#include "main.h"
#include "tmysql.h"

using namespace GarrysMod::Lua;

std::set<Database*> tmysql::m_databases;

bool tmysql::inShutdown = false;

int tmysql::iDatabaseMTID = 0;
int tmysql::iStatementMTID = 0;

int tmysql::lua_GetTable(lua_State* state)
{
	LUA->CreateTable();

	int i = 1;
	for (auto iter = m_databases.begin(); iter != m_databases.end(); iter++)
		LUA->PushNumber(i++),
		(*iter)->PushHandle(state),
		LUA->SetTable(-3);

	return 1;
}

int tmysql::lua_Create(lua_State* state)
{
	Database* mysqldb = createDatabase(state);
	if (!mysqldb) return 0;

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
	const char* host = LUA->CheckString(1);
	const char* user = LUA->CheckString(2);
	const char* pass = LUA->CheckString(3);
	const char* db = LUA->CheckString(4);
	unsigned int port = 3306;
	const char* unixSocket = "";
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

	Database* mysqldb = new Database(
		host,
		user,
		pass,
		db,
		port,
		unixSocket,
		flags,
		callbackfunc
	);

	if (mysqldb)
		RegisterDatabase(mysqldb);

	return mysqldb;
}
