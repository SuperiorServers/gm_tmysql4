#pragma once

#define GMOD_ALLOW_DEPRECATED

#define MODULE_VERSION 4.5

#define DATABASE_MT_NAME "Database"
#define PSTMT_MT_NAME "PreparedStatement"

#define LUA_GLOBAL -10002

#if defined(_WIN32) || defined(WIN32)

#include <winsock2.h>

#include <direct.h>
#define GetCurrentDir _getcwd

#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Secur32.lib")
#pragma comment(lib, "Shlwapi.lib")

#else

#include <unistd.h>
#define GetCurrentDir getcwd

#endif

#include <condition_variable>
#include <mysql.h>

#include "Lua/Interface.h"

class my_exception : public std::runtime_error {
public:
	my_exception(int errNo, const char* errMsg) : runtime_error(errMsg), errorNumber(errNo) { }
	const int errorNumber;
};

#define ThrowException(mysql) throw my_exception(mysql_errno(mysql), mysql_error(mysql))