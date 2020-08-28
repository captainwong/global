#include "Server.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/thread.h>
#include <thread>
#include <mutex>
#include <algorithm>
#include <signal.h>
#include <inttypes.h>
#include "../log2.h"


namespace jlib {
namespace net {
namespace server {


struct OneTimeIniter {
	OneTimeIniter() {
#ifdef _WIN32
		WSADATA wsa_data;
		WSAStartup(0x0201, &wsa_data);
		if (0 != evthread_use_windows_threads()) {
			JLOG_CRTC("failed to init libevent with thread by calling evthread_use_windows_threads");
			exit(-1);
		}
#else 
		if (0 != evthread_use_pthreads()) {
			JLOG_CRTC("failed to init libevent with thread by calling evthread_use_pthreads");
			exit(-1);
		}
		signal(SIGPIPE, SIG_IGN);
#endif

	}
};

static std::string eventToString(short evs) {
	std::string s;
#define check_event_append_to_s(e) if(evs & e){s += #e " ";}
	check_event_append_to_s(BEV_EVENT_READING);
	check_event_append_to_s(BEV_EVENT_WRITING);
	check_event_append_to_s(BEV_EVENT_EOF);
	check_event_append_to_s(BEV_EVENT_ERROR);
	check_event_append_to_s(BEV_EVENT_TIMEOUT);
	check_event_append_to_s(BEV_EVENT_CONNECTED);
#undef check_event_append_to_s
	return s;
}

struct BaseClientPrivateData {
	int thread_id = 0;
	void* bev = nullptr;
	void* timer = nullptr;
	std::chrono::steady_clock::time_point lastTimeComm = {};
};


BaseClient::BaseClient(int fd, void* bev)
	: fd(fd)
	, privateData(new BaseClientPrivateData())
{
	((BaseClientPrivateData*)privateData)->bev = bev;
}

BaseClient::~BaseClient()
{
	delete (BaseClientPrivateData*)privateData;
}

BaseClient* BaseClient::createDefaultClient(int fd, void* bev)
{
	BaseClient* client = new BaseClient(fd, bev);
	return client;
}

void BaseClient::send(const void* data, size_t len)
{
	if (!((BaseClientPrivateData*)privateData)->bev) {
		JLOG_CRTC("BaseClient::send bev is nullptr, #{}", fd);
		return;
	}

	auto output = bufferevent_get_output((bufferevent*)((BaseClientPrivateData*)privateData)->bev);
	if (!output) {
		JLOG_INFO("BaseClient::send bev output nullptr, #{}", fd);
		return;
	}

	evbuffer_lock(output);
	evbuffer_add(output, data, len);
	evbuffer_unlock(output);
}

void BaseClient::shutdown(int what)
{
	if (fd != 0) {
		::shutdown(fd, what);
		//fd = 0;
	}
}

void BaseClient::updateLastTimeComm()
{
	((BaseClientPrivateData*)privateData)->lastTimeComm = std::chrono::steady_clock::now();
}

struct Server::PrivateImpl
{
	struct WorkerThreadContext {
		std::string name = {};
		int thread_id = 0;
		event_base* base = nullptr;
		std::thread thread = {};

		static void dummy_timercb_avoid_worker_exit(evutil_socket_t, short, void*)
		{}

		explicit WorkerThreadContext(const std::string& name, int thread_id)
			: name(name)
			, thread_id(thread_id)
		{
			thread = std::thread(&WorkerThreadContext::worker, this);
		}

		void worker() {
			JLOG_INFO("{} WorkerThread #{} started", name.data(), thread_id);
			base = event_base_new();
			timeval tv = { 1, 0 };
			event_add(event_new(base, -1, EV_PERSIST, dummy_timercb_avoid_worker_exit, nullptr), &tv);
			event_base_dispatch(base);
			JLOG_INFO("{} WorkerThread #{} exited", name.data(), thread_id);
		}

		static void readcb(struct bufferevent* bev, void* user_data)
		{
			char buff[4096];
			auto input = bufferevent_get_input(bev);
			Server* server = (Server*)user_data;
			if (server->userData_ && server->onMsg_) {
				int fd = (int)bufferevent_getfd(bev);
				BaseClient* client = nullptr;
				{
					std::lock_guard<std::mutex> lg(server->mutex);
					auto iter = server->clients.find(fd);
					if (iter != server->clients.end()) {
						client = iter->second;
					}
				}
				if (client) {
					while (1) {
						int len = (int)evbuffer_copyout(input, buff, std::min(sizeof(buff), evbuffer_get_length(input)));
						if (len > 0) {
							size_t ate = server->onMsg_(buff, len, client, server->userData_);
							if (ate > 0) {
								evbuffer_drain(input, ate);
								continue;
							}
						}
						break;
					}
				} else {
					bufferevent_free(bev);
				}
			} else {
				evbuffer_drain(input, evbuffer_get_length(input));
			}
		}

		static void eventcb(struct bufferevent* bev, short events, void* user_data)
		{
			Server* server = (Server*)user_data;
			//printf("eventcb events=%d %s\n", events, eventToString(events).data());

			std::string msg;
			if (events & (BEV_EVENT_EOF)) {
				msg = ("Connection closed");
			} else if (events & BEV_EVENT_ERROR) {
				msg = ("Got an error on the connection: ");
				msg += strerror(errno);
			}

			if (server->userData_ && server->onConn_) {
				int fd = (int)bufferevent_getfd(bev);
				BaseClient* client = nullptr;
				{
					std::lock_guard<std::mutex> lg(server->mutex);
					auto iter = server->clients.find(fd);
					if (iter != server->clients.end()) {
						client = iter->second;
					} else {
						JLOG_CRTC("eventcb cannot find client by fd #{}", (int)fd);
					}
				}
				if (client) {
					if (((BaseClientPrivateData*)client->privateData)->timer) {
						event_del((event*)((BaseClientPrivateData*)client->privateData)->timer);
					}
					server->onConn_(false, msg, client, server->userData_);
					{
						std::lock_guard<std::mutex> lg(server->mutex);
						server->clients.erase(fd);
						delete client;
					}
				}
			}
			bufferevent_free(bev);
		}
	};
	typedef WorkerThreadContext* WorkerThreadContextPtr;

	PrivateImpl(void* user_data)
		: user_data(user_data)
	{}

	event_base* base = nullptr;
	void* user_data = nullptr;
	std::thread thread = {};
	timeval tv = {};
	WorkerThreadContextPtr* workerThreadContexts = {};
	int curWorkerId = 0;

	static void accpet_error_cb(evconnlistener* listener, void* context)
	{
		auto base = evconnlistener_get_base(listener);
		int err = EVUTIL_SOCKET_ERROR();
		JLOG_CRTC("accpet_error_cb:{}:{}", err, evutil_socket_error_to_string(err));
		event_base_loopexit(base, nullptr);
	}

	static void timercb(evutil_socket_t fd, short, void* user_data)
	{
		auto server = (Server*)user_data;
		std::lock_guard<std::mutex> lg(server->mutex);
		auto iter = server->clients.find((int)fd);
		if (iter != server->clients.end()) {
			auto client = iter->second;
			auto now = std::chrono::steady_clock::now();
			auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - ((BaseClientPrivateData*)client->privateData)->lastTimeComm);
			if (diff.count() > server->maxIdleTime_) {
				JLOG_INFO("{} client #{} timeout={}s > {}s, shutting down", server->name_, client->fd, diff.count(), server->maxIdleTime_);
				((BaseClientPrivateData*)client->privateData)->timer = nullptr;
				client->shutdown();
			} else {
				((BaseClientPrivateData*)client->privateData)->timer = event_new(server->impl->base, fd, 0, timercb, server);
				event_add((event*)((BaseClientPrivateData*)client->privateData)->timer, &server->impl->tv);
			}
		} else {
			JLOG_CRTC("timercb cannot find client by fd #{}", (int)fd);
		}
	}

	static void accept_cb(evconnlistener* listener, evutil_socket_t fd, sockaddr* addr, int socklen, void* user_data)
	{
		char str[INET_ADDRSTRLEN] = { 0 };
		auto sin = (sockaddr_in*)addr;
		inet_ntop(AF_INET, &sin->sin_addr, str, INET_ADDRSTRLEN);

		Server* server = (Server*)user_data;
		auto ctx = server->impl->workerThreadContexts[server->impl->curWorkerId];

		auto bev = bufferevent_socket_new(ctx->base, fd, BEV_OPT_CLOSE_ON_FREE);
		if (!bev) {
			JLOG_CRTC("Error constructing bufferevent!");
			exit(-1);
		}

		assert(server->newClient_);
		auto client = server->newClient_((int)fd, bev);
		((BaseClientPrivateData*)client->privateData)->thread_id = server->impl->curWorkerId;
		client->ip = str;
		client->port = sin->sin_port;
		client->updateLastTimeComm();
		((BaseClientPrivateData*)client->privateData)->timer = event_new(ctx->base, fd, 0, timercb, /*new TimerContext({ server, client })*/ server);
		event_add((event*)((BaseClientPrivateData*)client->privateData)->timer, &server->impl->tv);

		{
			std::lock_guard<std::mutex> lg(server->mutex);
			server->clients[(int)fd] = client;
		}

		bufferevent_setcb(bev, WorkerThreadContext::readcb, nullptr, WorkerThreadContext::eventcb, server);
		bufferevent_enable(bev, EV_WRITE | EV_READ);

		if (server->userData_ && server->onConn_) {
			server->onConn_(true, "", client, server->userData_);
		}

		server->impl->curWorkerId = (server->impl->curWorkerId + 1) % server->threadNum_;
	}

};

Server::Server()
{
	static OneTimeIniter initLibEvent;
}

Server::~Server()
{
	stop();
}

bool Server::start(uint16_t port, std::string& msg)
{
	do {
		stop();

		std::lock_guard<std::mutex> lg(mutex);

		impl = new PrivateImpl(this);
		impl->base = event_base_new();
		if (!impl->base) {
			msg = "init libevent failed";
			break;
		}

		sockaddr_in sin = { 0 };
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
		sin.sin_port = htons(port);

		auto listener = evconnlistener_new_bind(impl->base,
												PrivateImpl::accept_cb,
												this,
												LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
												-1, // backlog, -1 for default
												(const sockaddr*)(&sin),
												sizeof(sin));
		if (!listener) {
			JLOG_CRTC("{} create listener failed", name_.data());
			exit(-1);
		}
		evconnlistener_set_error_cb(listener, PrivateImpl::accpet_error_cb);

		// init common timeout 
		impl->tv.tv_sec = maxIdleTime_;
		impl->tv.tv_usec = 0;
		const struct timeval* tv_out = event_base_init_common_timeout(impl->base, &impl->tv);
		memcpy(&impl->tv, tv_out, sizeof(struct timeval));

		impl->thread = std::thread([this]() {
			JLOG_INFO("{} listen thread started", name_.data());
			event_base_dispatch(this->impl->base);
			JLOG_INFO("{} listen thread exited", name_.data());
		});

		impl->workerThreadContexts = new PrivateImpl::WorkerThreadContextPtr[threadNum_];
		for (int i = 0; i < threadNum_; i++) {
			impl->workerThreadContexts[i] = (new PrivateImpl::WorkerThreadContext(name_, i));
		}

		started_ = true;
		return true;
	} while (0);

	stop();
	return false;
}

void Server::stop()
{
	std::lock_guard<std::mutex> lg(mutex);
	if (!impl) { return; }

	if (impl->base) {
		event_base_loopexit(impl->base, nullptr);
	}

	if (impl->thread.joinable()) {
		impl->thread.join();
	}

	if (impl->base) {
		event_base_free(impl->base);
		impl->base = nullptr;
	}

	for (int i = 0; i < threadNum_; i++) {
		event_base_loopexit(impl->workerThreadContexts[i]->base, nullptr);
	}

	for (int i = 0; i < threadNum_; i++) {
		impl->workerThreadContexts[i]->thread.join();
		event_base_free(impl->workerThreadContexts[i]->base);
		delete impl->workerThreadContexts[i];
	}

	delete impl->workerThreadContexts;
	delete impl;
	impl = nullptr;

	for (auto client : clients) {
		delete client.second;
	}
	clients.clear();

	started_ = false;
}

}
}
}
