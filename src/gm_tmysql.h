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

#define ASSIGN_WORKING_DIR( varname ) char buff[FILENAME_MAX]; GetCurrentDir(buff, FILENAME_MAX); std::string varname = std::to_string(*buff);

#include <mysql.h>

#include "Lua/Interface.h"
#include "database.h"