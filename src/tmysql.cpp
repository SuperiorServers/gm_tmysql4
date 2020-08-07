#include "main.h"
#include "tmysql.h"

using namespace GarrysMod::Lua;

int tmysql::iRefDatabases = 0;
bool tmysql::inShutdown = false;

int tmysql::lua_GetTable(lua_State* state)
{
	LUA->ReferencePush(iRefDatabases);
	return 1;
}

int tmysql::lua_Create(lua_State* state)
{
	Database* mysqldb = createDatabase(state);

	if (!mysqldb) return 0;

	UserData* userdata = (UserData*)LUA->NewUserdata(sizeof(UserData));
	userdata->data = mysqldb;
	userdata->type = DATABASE_MT_ID;

	mysqldb->PushHandle(state);

	return 1;
}

int tmysql::lua_Connect(lua_State* state)
{
	Database* mysqldb = createDatabase(state);

	if (!mysqldb) return 0;

	std::string error;
	if (!mysqldb->Initialize(error))
	{
		LUA->PushBool(false);
		LUA->PushString(error.c_str());
		delete mysqldb;
		return 2;
	}

	if (mysqldb->IsPendingCallback())
		mysqldb->TriggerCallback(state);

	mysqldb->PushHandle(state);

	return 1;
}

Database* tmysql::createDatabase(lua_State* state)
{
	std::string host = LUA->CheckString(1);
	std::string user = LUA->CheckString(2);
	std::string pass = LUA->CheckString(3);
	std::string db = LUA->CheckString(4);
	unsigned int port = 3306;
	std::string unixSocket = "";
	unsigned long flags = 0;

	if (LUA->IsType(5, Type::Number))
		port = (unsigned int)LUA->GetNumber(5);

	if (LUA->IsType(6, GarrysMod::Lua::Type::String)) {
		unixSocket = LUA->GetString(6);
	}

	if (LUA->IsType(7, Type::Number))
		flags = (unsigned long)LUA->GetNumber(7);

	int callbackfunc = -1;
	if (LUA->GetType(8) == Type::Function)
	{
		LUA->Push(8);
		callbackfunc = LUA->ReferenceCreate();
	}

	return new Database(
		host,
		user,
		pass,
		db,
		port,
		unixSocket,
		flags,
		callbackfunc
	);
}