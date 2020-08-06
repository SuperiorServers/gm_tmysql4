#include "gm_tmysql.h"

using namespace GarrysMod::Lua;

#define DATABASE_NAME "Database"
#define DATABASE_ID 200

int iRefDatabases;

void PushDatabaseHandle(lua_State* state, Database* mysqldb);
void DisconnectDB(lua_State* state, Database* mysqldb);
void DispatchCompletedQueries(lua_State* state, Database* mysqldb);
void HandleConnectCallback(lua_State* state, Database* db);
void HandleQueryCallback(lua_State* state, Query* query);
void PopulateTableFromQuery(lua_State* state, Query* query);

bool in_shutdown = false;

// usage: host, user, pass, db, [port], [socket], [flags], [callback]
Database* makeDatabase(lua_State* state)
{
	std::string host = LUA->CheckString(1);
	std::string user = LUA->CheckString(2);
	std::string pass = LUA->CheckString(3);
	std::string db = LUA->CheckString(4);
	unsigned int port = 3306;
	std::string unixSocket = "";
	unsigned long flags = 0;

	if (LUA->IsType(5, Type::NUMBER))
		port = (unsigned int)LUA->GetNumber(5);

	if (LUA->IsType(6, GarrysMod::Lua::Type::STRING)) {
		unixSocket = LUA->GetString(6);
	}

	if (LUA->IsType(7, Type::NUMBER))
		flags  = (unsigned long)LUA->GetNumber(7);

	int callbackfunc = -1;
	if (LUA->GetType(8) == Type::FUNCTION)
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

void PushDatabaseHandle(lua_State* state, Database* mysqldb)
{
	UserData* userdata = (UserData*)LUA->NewUserdata(sizeof(UserData));
	userdata->data = mysqldb;
	userdata->type = DATABASE_ID;

	LUA->ReferencePush(iRefDatabases);
	LUA->PushNumber(mysqldb->GetTableIndex());
	LUA->Push(-3);
	LUA->SetTable(-3);
	LUA->Pop();

	LUA->CreateMetaTableType(DATABASE_NAME, DATABASE_ID);
	LUA->SetMetaTable(-2);
}


// tmysql
int tmysql_Create(lua_State* state)
{
	Database* mysqldb = makeDatabase(state);

	if (!mysqldb) return 0;

	UserData* userdata = (UserData*)LUA->NewUserdata(sizeof(UserData));
	userdata->data = mysqldb;
	userdata->type = DATABASE_ID;

	PushDatabaseHandle(state, mysqldb);

	return 1;
}

int tmysql_Connect(lua_State* state)
{
	Database* mysqldb = makeDatabase(state);

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
		HandleConnectCallback(state, mysqldb);

	PushDatabaseHandle(state, mysqldb);

	return 1;
}

int tmysql_GetTable(lua_State* state)
{
	LUA->ReferencePush(iRefDatabases);
	return 1;
}

// Database
int Database_Escape(lua_State* state)
{
	LUA->CheckType(1, DATABASE_ID);

	Database * mysqldb = *reinterpret_cast<Database **>(LUA->GetUserdata(1));

	if (!mysqldb) {
		LUA->ThrowError("Attempted to call Escape on a shutdown database");
		return 0;
	}

	if (!mysqldb->IsConnected()) {
		LUA->ThrowError("Attempted to call Escape on a disconnected database");
		return 0;
	}

	LUA->CheckType(2, Type::STRING);

	unsigned int len;
	const char* query = LUA->GetString(2, &len);

	char* escaped = mysqldb->Escape(query, len);
	LUA->PushString(escaped);

	delete[] escaped;
	return 1;
}

int Database_SetOption(lua_State* state)
{
	LUA->CheckType(1, DATABASE_ID);

	Database * mysqldb = *reinterpret_cast<Database **>(LUA->GetUserdata(1));

	if (!mysqldb) {
		LUA->ThrowError("Attempted to call Option on a shutdown database");
		return 0;
	}

	std::string error;

	mysql_option option = static_cast<mysql_option>((int)LUA->CheckNumber(2));

	LUA->PushBool(mysqldb->Option(option, LUA->CheckString(3), error));
	LUA->PushString(error.c_str());
	return 2;
}

int Database_GetServerInfo(lua_State* state)
{
	LUA->CheckType(1, DATABASE_ID);

	Database * mysqldb = *reinterpret_cast<Database **>(LUA->GetUserdata(1));

	if (!mysqldb) {
		LUA->ThrowError("Attempted to call GetServerInfo on a shutdown database");
		return 0;
	}

	if (!mysqldb->IsConnected()) {
		LUA->ThrowError("Attempted to call GetServerInfo on a disconnected database");
		return 0;
	}

	LUA->PushString(mysqldb->GetServerInfo());
	return 1;
}

int Database_GetHostInfo(lua_State* state)
{
	LUA->CheckType(1, DATABASE_ID);

	Database * mysqldb = *reinterpret_cast<Database **>(LUA->GetUserdata(1));

	if (!mysqldb) {
		LUA->ThrowError("Attempted to call GetHostInfo on a shutdown database");
		return 0;
	}

	if (!mysqldb->IsConnected()) {
		LUA->ThrowError("Attempted to call GetHostInfo on a disconnected database");
		return 0;
	}

	LUA->PushString(mysqldb->GetHostInfo());
	return 1;
}

int Database_GetServerVersion(lua_State* state)
{
	LUA->CheckType(1, DATABASE_ID);

	Database * mysqldb = *reinterpret_cast<Database **>(LUA->GetUserdata(1));

	if (!mysqldb) {
		LUA->ThrowError("Attempted to call GetServerVersion on a shutdown database");
		return 0;
	}

	if (!mysqldb->IsConnected()) {
		LUA->ThrowError("Attempted to call GetServerVersion on a disconnected database");
		return 0;
	}

	LUA->PushNumber(mysqldb->GetServerVersion());
	return 1;
}

int Database_Connect(lua_State* state)
{
	LUA->CheckType(1, DATABASE_ID);

	Database * mysqldb = *reinterpret_cast<Database **>(LUA->GetUserdata(1));

	if (!mysqldb) {
		LUA->ThrowError("Attempted to call Connect on a shutdown database");
		return 0;
	}

	if (mysqldb->IsConnected()) {
		LUA->ThrowError("Attempted to call Connect on an already connected database");
		return 0;
	}

	std::string error;
	bool success = mysqldb->Initialize(error);

	LUA->PushBool(success);

	if (!success)
	{
		LUA->PushString(error.c_str());
		delete mysqldb;
		return 2;
	}
	return 1;
}

int Database_IsConnected(lua_State* state)
{
	LUA->CheckType(1, DATABASE_ID);

	Database * mysqldb = *reinterpret_cast<Database **>(LUA->GetUserdata(1));

	if (!mysqldb) {
		LUA->ThrowError("Attempted to call IsConnected on a shutdown database");
		return 0;
	}

	LUA->PushBool(mysqldb->IsConnected());
	return 1;
}

int Database_Disconnect(lua_State* state)
{
	LUA->CheckType(1, DATABASE_ID);

	Database * mysqldb = *reinterpret_cast<Database **>(LUA->GetUserdata(1));

	if (!mysqldb) {
		LUA->ThrowError("Attempted to call Disconnect on a shutdown database");
		return 0;
	}

	LUA->ReferencePush(iRefDatabases);
	LUA->PushNumber(mysqldb->GetTableIndex());
	LUA->PushNil();
	LUA->SetTable(-3);

	DisconnectDB(state, mysqldb);

	return 0;
}

int Database_SetCharacterSet(lua_State* state)
{
	LUA->CheckType(1, DATABASE_ID);

	Database * mysqldb = *reinterpret_cast<Database **>(LUA->GetUserdata(1));

	if (!mysqldb) {
		LUA->ThrowError("Attempted to call SetCharacterSet on a shutdown database");
		return 0;
	}

	if (!mysqldb->IsConnected()) {
		LUA->ThrowError("Attempted to call SetCharacterSet on a disconnected database");
		return 0;
	}

	const char* set = LUA->CheckString(2);

	std::string error;
	LUA->PushBool(mysqldb->SetCharacterSet(set, error));
	LUA->PushString(error.c_str());
	return 2;
}

int Database_Query(lua_State* state)
{
	LUA->CheckType(1, DATABASE_ID);

	Database * mysqldb = *reinterpret_cast<Database **>(LUA->GetUserdata(1));

	if (!mysqldb) {
		LUA->ThrowError("Attempted to call Query on a shutdown database");
		return 0;
	}

	if (!mysqldb->IsConnected()) {
		LUA->ThrowError("Attempted to call Query on a disconnected database");
		return 0;
	}

	const char* query = LUA->CheckString(2);

	int callbackfunc = -1;
	if (LUA->GetType(3) == Type::FUNCTION)
	{
		LUA->Push(3);
		callbackfunc = LUA->ReferenceCreate();
	}

	int callbackref = -1;
	int callbackobj = LUA->GetType(4);
	if (callbackobj != Type::NIL)
	{
		LUA->Push(4);
		callbackref = LUA->ReferenceCreate();
	}

	mysqldb->QueueQuery(query, callbackfunc, callbackref, LUA->GetBool(5));
	return 0;
}

int Database_Poll(lua_State* state)
{
	LUA->CheckType(1, DATABASE_ID);

	Database * mysqldb = *reinterpret_cast<Database **>(LUA->GetUserdata(1));

	if (!mysqldb) {
		LUA->ThrowError("Attempted to call Poll on a shutdown database");
		return 0;
	}

	if (mysqldb->IsPendingCallback())
		HandleConnectCallback(state, mysqldb);

	DispatchCompletedQueries(state, mysqldb);
	return 0;
}

int Database_IsValid(lua_State* state)
{
	LUA->CheckType(1, DATABASE_ID);

	Database * mysqldb = *reinterpret_cast<Database **>(LUA->GetUserdata(1));

	LUA->PushBool(mysqldb != NULL);
	
	return 1;
}

/*
TMYSQL STUFFS
*/
void DisconnectDB(lua_State* state, Database* mysqldb)
{
	if (mysqldb)
	{
		mysqldb->Shutdown();
		DispatchCompletedQueries(state, mysqldb);

		while (mysqldb->RunShutdownWork())
			DispatchCompletedQueries(state, mysqldb);

		mysqldb->Release();
		delete mysqldb;
	}
}

void DispatchCompletedQueries(lua_State* state, Database* mysqldb)
{
	Query* completed = mysqldb->GetCompletedQueries();

	while (completed)
	{
		Query* query = completed;

		if (!in_shutdown && query->GetCallback() >= 0)
			HandleQueryCallback(state, query);

		completed = query->next;
		delete query;
	}
}

void HandleConnectCallback(lua_State* state, Database* db)
{
	db->MarkCallbackCalled();

	if (db->GetCallback() >= 0)
	{
		LUA->ReferencePush(db->GetCallback());

		if (!LUA->IsType(-1, Type::FUNCTION))
		{
			LUA->Pop();
			return;
		}

		// Push database handle
		PushDatabaseHandle(state, db);

		if (LUA->PCall(1, 0, 0))
		{
			LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
			LUA->GetField(-1, "ErrorNoHalt"); // could cache this function... but this really should not be called in the first place
			LUA->Push(-3); // This is the error message from PCall
			LUA->PushString("\n"); // add a newline since ErrorNoHalt does not do that itself
			LUA->Call(2, 0);
			LUA->Pop(2); // Pop twice since the PCall error is still on the stack
		}
	}
}

void HandleQueryCallback(lua_State* state, Query* query)
{
	LUA->ReferencePush(query->GetCallback());
	LUA->ReferenceFree(query->GetCallback());

	if (!LUA->IsType(-1, Type::FUNCTION))
	{
		LUA->Pop();
		LUA->ReferenceFree(query->GetCallbackRef());
		return;
	}

	int args = 1;
	if (query->GetCallbackRef() >= 0)
	{
		args = 2;
		LUA->ReferencePush(query->GetCallbackRef());
		LUA->ReferenceFree(query->GetCallbackRef());
	}

	LUA->CreateTable();
	PopulateTableFromQuery(state, query);

	// For some reason PCall crashes during shutdown??
	if (LUA->PCall(args, 0, 0) != 0)
	{
		LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		LUA->GetField(-1, "ErrorNoHalt"); // could cache this function... but this really should not be called in the first place
		LUA->Push(-3); // This is the error message from PCall
		LUA->PushString("\n"); // add a newline since ErrorNoHalt does not do that itself
		LUA->Call(2, 0);
		LUA->Pop(2); // Pop twice since the PCall error is still on the stack
	}
}

void PopulateTableFromResult(lua_State* state, MYSQL_RES* result, bool usenumbers)
{
	// no result to push, continue, this isn't fatal
	if (result == NULL)
		return;

	MYSQL_ROW row = mysql_fetch_row(result);
	MYSQL_FIELD *fields = mysql_fetch_fields(result);

	int rowid = 1;

	while (row)
	{
		unsigned int field_count = mysql_num_fields(result);
		unsigned long *lengths = mysql_fetch_lengths(result);

		// black magic warning: we use a temp and assign it so that we avoid consuming all the temp objects and causing horrible disasters
		LUA->CreateTable();
		int resultrow = LUA->ReferenceCreate();

		LUA->PushNumber(rowid++);
		LUA->ReferencePush(resultrow);
		LUA->ReferenceFree(resultrow);

		for (unsigned int i = 0; i < field_count; i++)
		{
			if (usenumbers == true)
				LUA->PushNumber(i + 1);

			if (row[i] == NULL)
				LUA->PushNil();
			else if (IS_NUM(fields[i].type) && fields[i].type != MYSQL_TYPE_LONGLONG)
				LUA->PushNumber(atof(row[i]));
			else
				LUA->PushString(row[i], lengths[i]);

			if (usenumbers == true)
				LUA->SetTable(-3);
			else
				LUA->SetField(-2, fields[i].name);
		}

		LUA->SetTable(-3);

		row = mysql_fetch_row(result);
	}
}

void PopulateTableFromQuery(lua_State* state, Query* query)
{
	Results results = query->GetResults();

	int resultid = 1;

	for (Results::iterator it = results.begin(); it != results.end(); ++it) {
		Result* result = *it;

		LUA->PushNumber(resultid++);

		LUA->CreateTable();
		{
			bool status = result->GetErrorID() == 0;
			LUA->PushBool(status);
			LUA->SetField(-2, "status");
			if (!status) {
				LUA->PushString(result->GetError().c_str());
				LUA->SetField(-2, "error");
				LUA->PushNumber(result->GetErrorID());
				LUA->SetField(-2, "errorid");
			}
			else {
				LUA->PushNumber(result->GetAffected());
				LUA->SetField(-2, "affected");
				LUA->PushNumber(result->GetLastID());
				LUA->SetField(-2, "lastid");
				LUA->CreateTable();
				PopulateTableFromResult(state, result->GetResult(), query->GetUseNumbers());
				LUA->SetField(-2, "data");
			}
#ifdef ENABLE_QUERY_TIMERS
			LUA->PushNumber(query->GetQueryTime());
			LUA->SetField(-2, "time");
#endif
		}

		LUA->SetTable(-3);
	}
}

GMOD_MODULE_OPEN()
{
	mysql_library_init(0, NULL, NULL);

	in_shutdown = false;

	LUA->CreateTable();
	iRefDatabases = LUA->ReferenceCreate();

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	{
		LUA->CreateTable();
		{
			LUA->PushNumber(4.2);
			LUA->SetField(-2, "Version");

			LUA->PushCFunction(tmysql_Create);
			LUA->SetField(-2, "Create");
			LUA->PushCFunction(tmysql_Connect);
			LUA->SetField(-2, "Connect");
			LUA->PushCFunction(tmysql_GetTable);
			LUA->SetField(-2, "GetTable");

			LUA->CreateTable();
			{
				LUA->PushNumber(mysql_get_client_version());
				LUA->SetField(-2, "MYSQL_VERSION");

				LUA->PushString(mysql_get_client_info());
				LUA->SetField(-2, "MYSQL_INFO");
			}
			LUA->SetField(-2, "info");

			LUA->CreateTable();
			{
				LUA->PushNumber(CLIENT_FOUND_ROWS);
				LUA->SetField(-2, "CLIENT_FOUND_ROWS");
				LUA->PushNumber(CLIENT_LONG_FLAG);
				LUA->SetField(-2, "CLIENT_LONG_FLAG");
				LUA->PushNumber(CLIENT_CONNECT_WITH_DB);
				LUA->SetField(-2, "CLIENT_CONNECT_WITH_DB");
				LUA->PushNumber(CLIENT_NO_SCHEMA);
				LUA->SetField(-2, "CLIENT_NO_SCHEMA");
				LUA->PushNumber(CLIENT_COMPRESS);
				LUA->SetField(-2, "CLIENT_COMPRESS");
				LUA->PushNumber(CLIENT_ODBC);
				LUA->SetField(-2, "CLIENT_ODBC");
				LUA->PushNumber(CLIENT_LOCAL_FILES);
				LUA->SetField(-2, "CLIENT_LOCAL_FILES");
				LUA->PushNumber(CLIENT_IGNORE_SPACE);
				LUA->SetField(-2, "CLIENT_IGNORE_SPACE");
				LUA->PushNumber(CLIENT_TRANSACTIONS);
				LUA->SetField(-2, "CLIENT_TRANSACTIONS");
				LUA->PushNumber(CLIENT_RESERVED);
				LUA->SetField(-2, "CLIENT_RESERVED");
				LUA->PushNumber(CLIENT_MULTI_STATEMENTS);
				LUA->SetField(-2, "CLIENT_MULTI_STATEMENTS");
				LUA->PushNumber(CLIENT_MULTI_RESULTS);
				LUA->SetField(-2, "CLIENT_MULTI_RESULTS");
				LUA->PushNumber(CLIENT_PS_MULTI_RESULTS);
				LUA->SetField(-2, "CLIENT_PS_MULTI_RESULTS");
			}
			LUA->SetField(-2, "flags");

			LUA->CreateTable();
			{
				LUA->PushNumber(MYSQL_OPT_CONNECT_TIMEOUT);
				LUA->SetField(-2, "MYSQL_OPT_CONNECT_TIMEOUT");
				LUA->PushNumber(MYSQL_OPT_COMPRESS);
				LUA->SetField(-2, "MYSQL_OPT_COMPRESS");
				LUA->PushNumber(MYSQL_OPT_NAMED_PIPE);
				LUA->SetField(-2, "MYSQL_OPT_NAMED_PIPE");
				LUA->PushNumber(MYSQL_INIT_COMMAND);
				LUA->SetField(-2, "MYSQL_INIT_COMMAND");
				LUA->PushNumber(MYSQL_READ_DEFAULT_FILE);
				LUA->SetField(-2, "MYSQL_READ_DEFAULT_FILE");
				LUA->PushNumber(MYSQL_READ_DEFAULT_GROUP);
				LUA->SetField(-2, "MYSQL_READ_DEFAULT_GROUP");
				LUA->PushNumber(MYSQL_SET_CHARSET_DIR);
				LUA->SetField(-2, "MYSQL_SET_CHARSET_DIR");
				LUA->PushNumber(MYSQL_SET_CHARSET_NAME);
				LUA->SetField(-2, "MYSQL_SET_CHARSET_NAME");
				LUA->PushNumber(MYSQL_OPT_LOCAL_INFILE);
				LUA->SetField(-2, "MYSQL_OPT_LOCAL_INFILE");
				LUA->PushNumber(MYSQL_OPT_PROTOCOL);
				LUA->SetField(-2, "MYSQL_OPT_PROTOCOL");
				LUA->PushNumber(MYSQL_SHARED_MEMORY_BASE_NAME);
				LUA->SetField(-2, "MYSQL_SHARED_MEMORY_BASE_NAME");
				LUA->PushNumber(MYSQL_OPT_READ_TIMEOUT);
				LUA->SetField(-2, "MYSQL_OPT_READ_TIMEOUT");
				LUA->PushNumber(MYSQL_OPT_WRITE_TIMEOUT);
				LUA->SetField(-2, "MYSQL_OPT_WRITE_TIMEOUT");
				LUA->PushNumber(MYSQL_OPT_USE_RESULT);
				LUA->SetField(-2, "MYSQL_OPT_USE_RESULT");
				LUA->PushNumber(MYSQL_OPT_USE_REMOTE_CONNECTION);
				LUA->SetField(-2, "MYSQL_OPT_USE_REMOTE_CONNECTION");
				LUA->PushNumber(MYSQL_OPT_USE_EMBEDDED_CONNECTION);
				LUA->SetField(-2, "MYSQL_OPT_USE_EMBEDDED_CONNECTION");
				LUA->PushNumber(MYSQL_OPT_GUESS_CONNECTION);
				LUA->SetField(-2, "MYSQL_OPT_GUESS_CONNECTION");
				LUA->PushNumber(MYSQL_SET_CLIENT_IP);
				LUA->SetField(-2, "MYSQL_SET_CLIENT_IP");
				LUA->PushNumber(MYSQL_SECURE_AUTH);
				LUA->SetField(-2, "MYSQL_SECURE_AUTH");
				LUA->PushNumber(MYSQL_REPORT_DATA_TRUNCATION);
				LUA->SetField(-2, "MYSQL_REPORT_DATA_TRUNCATION");
				LUA->PushNumber(MYSQL_OPT_RECONNECT);
				LUA->SetField(-2, "MYSQL_OPT_RECONNECT");
				LUA->PushNumber(MYSQL_OPT_SSL_VERIFY_SERVER_CERT);
				LUA->SetField(-2, "MYSQL_OPT_SSL_VERIFY_SERVER_CERT");
				LUA->PushNumber(MYSQL_PLUGIN_DIR);
				LUA->SetField(-2, "MYSQL_PLUGIN_DIR");
				LUA->PushNumber(MYSQL_DEFAULT_AUTH);
				LUA->SetField(-2, "MYSQL_DEFAULT_AUTH");
				LUA->PushNumber(MYSQL_OPT_BIND);
				LUA->SetField(-2, "MYSQL_OPT_BIND");
				LUA->PushNumber(MYSQL_OPT_SSL_KEY);
				LUA->SetField(-2, "MYSQL_OPT_SSL_KEY");
				LUA->PushNumber(MYSQL_OPT_SSL_CERT);
				LUA->SetField(-2, "MYSQL_OPT_SSL_CERT");
				LUA->PushNumber(MYSQL_OPT_SSL_CA);
				LUA->SetField(-2, "MYSQL_OPT_SSL_CA");
				LUA->PushNumber(MYSQL_OPT_SSL_CAPATH);
				LUA->SetField(-2, "MYSQL_OPT_SSL_CAPATH");
				LUA->PushNumber(MYSQL_OPT_SSL_CIPHER);
				LUA->SetField(-2, "MYSQL_OPT_SSL_CIPHER");
				LUA->PushNumber(MYSQL_OPT_SSL_CRL);
				LUA->SetField(-2, "MYSQL_OPT_SSL_CRL");
				LUA->PushNumber(MYSQL_OPT_SSL_CRLPATH);
				LUA->SetField(-2, "MYSQL_OPT_SSL_CRLPATH");
				LUA->PushNumber(MYSQL_OPT_CONNECT_ATTR_RESET);
				LUA->SetField(-2, "MYSQL_OPT_CONNECT_ATTR_RESET");
				LUA->PushNumber(MYSQL_OPT_CONNECT_ATTR_ADD);
				LUA->SetField(-2, "MYSQL_OPT_CONNECT_ATTR_ADD");
				LUA->PushNumber(MYSQL_OPT_CONNECT_ATTR_DELETE);
				LUA->SetField(-2, "MYSQL_OPT_CONNECT_ATTR_DELETE");
				LUA->PushNumber(MYSQL_SERVER_PUBLIC_KEY);
				LUA->SetField(-2, "MYSQL_SERVER_PUBLIC_KEY");
				LUA->PushNumber(MYSQL_ENABLE_CLEARTEXT_PLUGIN);
				LUA->SetField(-2, "MYSQL_ENABLE_CLEARTEXT_PLUGIN");
				LUA->PushNumber(MYSQL_OPT_CAN_HANDLE_EXPIRED_PASSWORDS);
				LUA->SetField(-2, "MYSQL_OPT_CAN_HANDLE_EXPIRED_PASSWORDS");
				LUA->PushNumber(MYSQL_OPT_SSL_ENFORCE);
				LUA->SetField(-2, "MYSQL_OPT_SSL_ENFORCE");
			}
			LUA->SetField(-2, "opts");
		}
		LUA->SetField(-2, "tmysql");
	}
	LUA->Pop();

	LUA->CreateMetaTableType(DATABASE_NAME, DATABASE_ID);
	{
		LUA->Push(-1);
		LUA->SetField(-2, "__index");

		LUA->PushCFunction(Database_IsValid);
		LUA->SetField(-2, "IsValid");
		LUA->PushCFunction(Database_Query);
		LUA->SetField(-2, "Query");
		LUA->PushCFunction(Database_Escape);
		LUA->SetField(-2, "Escape");
		LUA->PushCFunction(Database_SetOption);
		LUA->SetField(-2, "SetOption");
		LUA->PushCFunction(Database_GetServerInfo);
		LUA->SetField(-2, "GetServerInfo");
		LUA->PushCFunction(Database_GetHostInfo);
		LUA->SetField(-2, "GetHostInfo");
		LUA->PushCFunction(Database_GetServerVersion);
		LUA->SetField(-2, "GetServerVersion");
		LUA->PushCFunction(Database_Connect);
		LUA->SetField(-2, "Connect");
		LUA->PushCFunction(Database_IsConnected);
		LUA->SetField(-2, "IsConnected");
		LUA->PushCFunction(Database_Disconnect);
		LUA->SetField(-2, "Disconnect");
		LUA->PushCFunction(Database_SetCharacterSet);
		LUA->SetField(-2, "SetCharacterSet");
		LUA->PushCFunction(Database_Poll);
		LUA->SetField(-2, "Poll");
	}
	LUA->Pop(1);

	return 0;
}


GMOD_MODULE_CLOSE()
{
	in_shutdown = true;

	LUA->ReferencePush(iRefDatabases);
	LUA->PushNil();

	while (LUA->Next(-2))
	{
		LUA->Push(-2);

		if (LUA->IsType(-2, DATABASE_ID))
		{
			Database * mysqldb = *reinterpret_cast<Database **>(LUA->GetUserdata(-2));

			if (mysqldb)
				DisconnectDB(state, mysqldb);
		}

		LUA->Pop(2);
	}
	LUA->Pop();

	LUA->ReferenceFree(iRefDatabases);
	mysql_library_end();
	return 0;
}
