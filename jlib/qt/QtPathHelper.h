#pragma once

#include "qt_global.h"
#include <QDir>
#include <QCoreApplication>
#include <QString>
#include <QDateTime>
#include <QStandardPaths>

JLIBQT_NAMESPACE_BEGIN

static const auto JLIBQT_FILE_TIME_FORMAT = "yyyy-MM-dd_hh-mm-ss";

struct PathHelper
{
	static inline bool mkpath(const QString& path) {
		QDir dir; return dir.exists(path) ? true : dir.mkpath(path);
	}

	static inline QString currentTimeString() {
		return QDateTime::currentDateTime().toString(JLIBQT_FILE_TIME_FORMAT);
	}

	virtual ~PathHelper() {}

	virtual QString program()	const { return programPath_; }
	virtual QString bin()		const { return program() + "/bin"; }
	virtual QString data()		const { return program() + "/data"; }
	virtual QString log()		const { return program() + "/log"; }
	virtual QString dumps()		const { return program() + "/dumps"; }
	virtual QString config()	const { return data() + "/config"; }
	virtual QString db()		const { return data() + "/db"; }
	
protected:
	// disable constructor
	explicit PathHelper() {}
	QString programPath_ = {};
};

struct AutoSwithToBin {
	AutoSwithToBin(PathHelper* helper) : helper(helper) {}
	AutoSwithToBin(PathHelper* helper, const QString& path) : helper(helper) {
		QDir().setCurrent(path);
	}
	~AutoSwithToBin() {
		if (helper) { QDir().setCurrent(helper->bin()); }
	}

	PathHelper* helper = nullptr;
};

/*
* @brief ·�������࣬���ݱ����ڰ�װĿ¼�������dll��bin�ļ���
* @note ���½ṹ��Ϊ��
* @note |-- program-install-dir
* @note |   |-- bin
* @note |   |    |-- program.exe
* @note |   |    |-- *.dlls
* @note |   |-- dumps
* @note |   |-- log
* @note |   |-- data
* @note |       |-- config
* @note |       |-- db
*/
struct PathHelperLocal : PathHelper
{
	PathHelperLocal() : PathHelper() {
		struct Helper {
			Helper() {
				QDir dir(QCoreApplication::applicationDirPath());
				dir.cdUp(); path = dir.absolutePath();
			}
			QString path = {};
		};
		static Helper helper;
		programPath_ = helper.path;
	}
};


/*
* @brief ·�������࣬���ݱ����ڰ�װĿ¼�������dllҲ�ڰ�װĿ¼
* @note ���½ṹ��Ϊ��
* @note |-- program-install-dir
* @note |   |-- dumps
* @note |   |-- log
* @note |   |-- data
* @note |   |   |-- config
* @note |   |   |-- db
* @note |   |-- program.exe
* @note |   |-- *.dlls
*/
struct PathHelperLocalWithoutBin : PathHelper
{
	PathHelperLocalWithoutBin() : PathHelper() {
		struct Helper {
			Helper() {
				QDir dir(QCoreApplication::applicationDirPath());
				path = dir.absolutePath();
			}
			QString path = {};
		};
		static Helper helper;
		programPath_ = helper.path;
	}

	virtual QString bin() const override { return program(); }
};

/*
* @brief ·�������࣬���ݱ�����������дĿ¼���� C:/Users/[USER]/AppData/Roaming
* @note �ڵ��ô���֮ǰ�ȵ��� QCoreApplication::setOrganizationName("your-organization-name");
* @note ���½ṹ��Ϊ��
* @note |-- program-install-dir
* @note |   |-- bin
* @note |   |    |-- program.exe
* @note |   |    |-- *.dlls
* @note
* @note |-- writable-dir
* @note |   |-- your-organization-name
* @note |   |    |-- program-name
* @note |   |        |-- log
* @note |   |        |-- dumps
* @note |   |        |-- config
* @note |   |        |-- db
*/
struct PathHelperDataSeperated : PathHelperLocal
{
	PathHelperDataSeperated() : PathHelperLocal() {}

	virtual QString data()		const override {
		return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	}
	virtual QString log()		const override { return data() + "/log"; }
	virtual QString dumps()		const override { return data() + "/dumps"; }
	virtual QString config()	const override { return data() + "/config"; }
	virtual QString db()		const override { return data() + "/db"; }
};

/*
* @brief ·�������࣬���ݱ�����������дĿ¼���� C:/Users/[USER]/AppData/Roaming
* @note �ڵ��ô���֮ǰ�ȵ��� QCoreApplication::setOrganizationName("your-organization-name");
* @note ���½ṹ��Ϊ��
* @note |-- program-install-dir
* @note |   |-- program.exe
* @note |   |-- *.dlls
* @note
* @note |-- writable-dir
* @note |   |-- your-organization-name
* @note |   |    |-- program-name
* @note |   |        |-- log
* @note |   |        |-- dumps
* @note |   |        |-- config
* @note |   |        |-- db
*/
struct PathHelperDataSeperatedWithoutBin : PathHelperLocalWithoutBin
{
	PathHelperDataSeperatedWithoutBin() : PathHelperLocalWithoutBin() {}

	virtual QString data()		const override {
		return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	}
	virtual QString log()		const override { return data() + "/log"; }
	virtual QString dumps()		const override { return data() + "/dumps"; }
	virtual QString config()	const override { return data() + "/config"; }
	virtual QString db()		const override { return data() + "/db"; }
};


JLIBQT_NAMESPACE_END
