#ifndef TMYSQL4_TMYSQL
#define TMYSQL4_TMYSQL

#include "database.h"

class tmysql {
public:
	static int		iRefDatabases;
	static bool		inShutdown;

	static int		lua_Create(lua_State* state);
	static int		lua_Connect(lua_State* state);
	static int		lua_GetTable(lua_State* state);

private:
	static Database* createDatabase(lua_State* state);
};

#endif