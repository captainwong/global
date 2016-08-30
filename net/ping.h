#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <chrono>
#include <istream>
#include <iostream>
#include <ostream>
#include "icmp_header.hpp"
#include "ipv4_header.hpp"

using boost::asio::ip::icmp;
using boost::asio::deadline_timer;
namespace posix_time = boost::posix_time;

class pinger
{
public:
	pinger(boost::asio::io_service& io_service, const char* destination, unsigned short max_sequence_number = 0)
		: resolver_(io_service), socket_(io_service, icmp::v4()),
		timer_(io_service), sequence_number_(0), num_replies_(0), max_sequence_number_(max_sequence_number)
	{
		icmp::resolver::query query(icmp::v4(), destination, "");
		destination_ = *resolver_.resolve(query);

		start_send();
		start_receive();
	}

	auto get_average() const {
		auto cnt = total_time_.count();
		if (max_sequence_number_ == 0) {
			return cnt;
		} else {
			return cnt / max_sequence_number_;
		}
	}

	bool quiting() const { return quiting_; }

private:
	void start_send()
	{
		std::string body("\"Hello!\" from Asio ping.");

		if (max_sequence_number_ != 0 && sequence_number_ >= max_sequence_number_) {
			quiting_ = true;
			timer_.cancel();
			socket_.close();
			socket_.get_io_service().stop();
			return;
		}

		// Create an ICMP header for an echo request.
		icmp_header echo_request;
		echo_request.type(icmp_header::echo_request);
		echo_request.code(0);
		echo_request.identifier(get_identifier());
		echo_request.sequence_number(++sequence_number_);
		compute_checksum(echo_request, body.begin(), body.end());

		// Encode the request packet.
		boost::asio::streambuf request_buffer;
		std::ostream os(&request_buffer);
		os << echo_request << body;

		// Send the request.
		time_sent_ = posix_time::microsec_clock::universal_time();
		socket_.send_to(request_buffer.data(), destination_);

		// Wait up to five seconds for a reply.
		num_replies_ = 0;
		timer_.expires_at(time_sent_ + posix_time::seconds(3));
		timer_.async_wait(boost::bind(&pinger::handle_timeout, this));
	}

	void handle_timeout()
	{
		if (num_replies_ == 0) {
			total_time_ += std::chrono::milliseconds(5000);
			JLOGA("Request timed out");
		}

		if (!quiting_) {
			// Requests must be sent no less than one second apart.
			timer_.expires_at(time_sent_ + posix_time::seconds(1));
			timer_.async_wait(boost::bind(&pinger::start_send, this));
		}
	}

	void start_receive()
	{
		// Discard any data already in the buffer.
		reply_buffer_.consume(reply_buffer_.size());

		// Wait for a reply. We prepare the buffer to receive up to 64KB.
		socket_.async_receive(reply_buffer_.prepare(65536),
							  boost::bind(&pinger::handle_receive, this, _2));
	}

	void handle_receive(std::size_t length)
	{
		// The actual number of bytes received is committed to the buffer so that we
		// can extract it using a std::istream object.
		reply_buffer_.commit(length);

		// Decode the reply packet.
		std::istream is(&reply_buffer_);
		ipv4_header ipv4_hdr;
		icmp_header icmp_hdr;
		is >> ipv4_hdr >> icmp_hdr;

		// We can receive all ICMP packets received by the host, so we need to
		// filter out only the echo replies that match the our identifier and
		// expected sequence number.
		if (is && icmp_hdr.type() == icmp_header::echo_reply
			&& icmp_hdr.identifier() == get_identifier()
			&& icmp_hdr.sequence_number() == sequence_number_) {
			// If this is the first reply, interrupt the five second timeout.
			if (num_replies_++ == 0)
				timer_.cancel();

			// Print out some information about the reply packet.
			posix_time::ptime now = posix_time::microsec_clock::universal_time();
			auto ms = (now - time_sent_).total_milliseconds();
			std::stringstream ss;
			ss << length - ipv4_hdr.header_length()
				<< " bytes from " << ipv4_hdr.source_address()
				<< ": icmp_seq=" << icmp_hdr.sequence_number()
				<< ", ttl=" << ipv4_hdr.time_to_live()
				<< ", time=" << ms << " ms";
			JLOGA(ss.str().c_str());
			total_time_ += std::chrono::milliseconds(ms);
			auto cnt = total_time_.count();
			ss.str(""); ss.clear();
			ss << "total_time_ " << cnt;
			JLOGA(ss.str().c_str());
		}

		if (max_sequence_number_ != 0 && sequence_number_ < max_sequence_number_)
			start_receive();
		else {
			quiting_ = true;
			timer_.cancel();
			socket_.close();
			socket_.get_io_service().stop();
		}
	}

	static unsigned short get_identifier()
	{
#if defined(BOOST_WINDOWS)
		return static_cast<unsigned short>(::GetCurrentProcessId());
#else
		return static_cast<unsigned short>(::getpid());
#endif
	}

	icmp::resolver resolver_;
	icmp::endpoint destination_;
	icmp::socket socket_;
	deadline_timer timer_;
	unsigned short sequence_number_;
	unsigned short max_sequence_number_;
	posix_time::ptime time_sent_;
	boost::asio::streambuf reply_buffer_;
	std::size_t num_replies_;
	bool quiting_ = false;
	std::chrono::milliseconds total_time_ = {};
};
