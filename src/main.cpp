#include "main.h"
#include "tmysql.h"
#include "statement.h"

GMOD_MODULE_OPEN()
{
	mysql_library_init(0, NULL, NULL);

	tmysql::inShutdown = false;

	LUA->CreateTable();
	tmysql::iRefDatabases = LUA->ReferenceCreate();

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	{
		LUA->CreateTable();
		{
			LUA->PushNumber(MODULE_VERSION);
			LUA->SetField(-2, "Version");

			LUA->PushCFunction(tmysql::lua_Create);
			LUA->SetField(-2, "Create");
			LUA->PushCFunction(tmysql::lua_Connect);
			LUA->SetField(-2, "Connect");
			LUA->PushCFunction(tmysql::lua_GetTable);
			LUA->SetField(-2, "GetTable");

			LUA->CreateTable();
			{
				LUA->PushNumber(mysql_get_client_version());
				LUA->SetField(-2, "MYSQL_VERSION");

				LUA->PushString(mysql_get_client_info());
				LUA->SetField(-2, "MYSQL_INFO");
			}
			LUA->SetField(-2, "info");

#pragma region MySQL Flags
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
#pragma endregion

#pragma region MySQL Opts
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
#pragma endregion

		}
		LUA->SetField(-2, "tmysql");
	}
	LUA->Pop();

	tmysql::iDatabaseMTID = LUA->CreateMetaTable(DATABASE_MT_NAME);
	{
		LUA->Push(-1);
		LUA->SetField(-2, "__index");
		LUA->PushCFunction(Database::lua___gc);
		LUA->SetField(-2, "__gc");
		LUA->PushCFunction(Database::lua__tostring);
		LUA->SetField(-2, "__tostring");
		LUA->PushCFunction(Database::lua_IsValid);
		LUA->SetField(-2, "IsValid");
		LUA->PushCFunction(Database::lua_Query);
		LUA->SetField(-2, "Query");
		LUA->PushCFunction(Database::lua_Prepare);
		LUA->SetField(-2, "Prepare");
		LUA->PushCFunction(Database::lua_Escape);
		LUA->SetField(-2, "Escape");
		LUA->PushCFunction(Database::lua_SetOption);
		LUA->SetField(-2, "SetOption");
		LUA->PushCFunction(Database::lua_GetServerInfo);
		LUA->SetField(-2, "GetServerInfo");
		LUA->PushCFunction(Database::lua_GetDatabase);
		LUA->SetField(-2, "GetDatabase");
		LUA->PushCFunction(Database::lua_GetHostInfo);
		LUA->SetField(-2, "GetHostInfo");
		LUA->PushCFunction(Database::lua_GetServerVersion);
		LUA->SetField(-2, "GetServerVersion");
		LUA->PushCFunction(Database::lua_Connect);
		LUA->SetField(-2, "Connect");
		LUA->PushCFunction(Database::lua_IsConnected);
		LUA->SetField(-2, "IsConnected");
		LUA->PushCFunction(Database::lua_Disconnect);
		LUA->SetField(-2, "Disconnect");
		LUA->PushCFunction(Database::lua_SetCharacterSet);
		LUA->SetField(-2, "SetCharacterSet");
		LUA->PushCFunction(Database::lua_Poll);
		LUA->SetField(-2, "Poll");
	}
	LUA->Pop();

	tmysql::iStatementMTID = LUA->CreateMetaTable(PSTMT_MT_NAME);
	{
		LUA->Push(-1);
		LUA->SetField(-2, "__index");

		LUA->PushCFunction(PStatement::lua___gc);
		LUA->SetField(-2, "__gc");

		LUA->PushCFunction(PStatement::lua__tostring);
		LUA->SetField(-2, "__tostring");

		LUA->PushCFunction(PStatement::lua_Run);
		LUA->SetField(-2, "__call");

		LUA->PushCFunction(PStatement::lua_IsValid);
		LUA->SetField(-2, "IsValid");

		LUA->PushCFunction(PStatement::lua_GetArgCount);
		LUA->SetField(-2, "GetArgCount");

		LUA->PushCFunction(PStatement::lua_Run);
		LUA->SetField(-2, "Run");
	}
	LUA->Pop();

	return 0;
}


GMOD_MODULE_CLOSE()
{
	tmysql::inShutdown = true;

	LUA->ReferencePush(tmysql::iRefDatabases);
	LUA->PushNil();

	while (LUA->Next(-2))
	{
		LUA->Push(-2);

		Database* mysqldb = LUA->GetUserType<Database>(-2, tmysql::iDatabaseMTID);

		if (mysqldb != nullptr)
			mysqldb->Disconnect(state);

		LUA->Pop(2);
	}
	LUA->Pop();

	LUA->ReferenceFree(tmysql::iRefDatabases);
	mysql_library_end();

	return 0;
}