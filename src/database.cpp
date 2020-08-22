#include "main.h"
#include "statement.h"
#include "database.h"
#include "tmysql.h"

using namespace GarrysMod::Lua;

Database::Database(const char* host, const char* user, const char* pass, const char* db, unsigned int port, const char* socket, unsigned long flags, int callback)
	:  m_iPort(port), m_iClientFlags(flags), m_iCallback(callback), m_bIsConnected(false), m_MySQL(NULL)
{
		strncpy(m_strHost, host, 253);
		strncpy(m_strUser, user, 32);
		strncpy(m_strPass, pass, 32);
		strncpy(m_strDB, db, 64);
		strncpy(m_strSocket, socket, 107);
	work.reset(new asio::io_service::work(io_service));
}

Database::~Database( void )
{
}

bool Database::Initialize(std::string& error)
{
	m_MySQL = mysql_init(nullptr);

	if (m_MySQL == NULL)
	{
		error.assign("Out of memory!");
		return false;
	}

	if (!Connect(error))
		return false;

	thread_group.push_back(std::thread( [&]() { io_service.run(); } ));

	m_bIsConnected = true;
	return true;
}

bool Database::Connect(std::string& error)
{
	const char* socket = (strlen(m_strSocket) == 0) ? nullptr : m_strSocket;
	unsigned int flags = m_iClientFlags | CLIENT_MULTI_RESULTS;

	my_bool tru = 1;
	if (mysql_options(m_MySQL, MYSQL_OPT_RECONNECT, &tru) > 0)
	{
		error.assign(mysql_error(m_MySQL));
		return false;
	}

	std::string wkdir = get_working_dir() + "/garrysmod/lua/bin";
	if (mysql_options(m_MySQL, MYSQL_PLUGIN_DIR, wkdir.c_str()) > 0)
	{
		error.assign(mysql_error(m_MySQL));
		return false;
	}

	if (mysql_real_connect(m_MySQL, m_strHost, m_strUser, m_strPass, m_strDB, m_iPort, socket, flags) != m_MySQL)
	{
		error.assign(mysql_error(m_MySQL));
		return false;
	}

	m_bIsPendingCallback = true;

	return true;
}

void Database::Shutdown(void)
{
	work.reset();

	for (auto iter = thread_group.begin(); iter != thread_group.end(); ++iter)
		iter->join();

	assert(io_service.stopped());
}

std::size_t Database::RunShutdownWork(void)
{
	io_service.reset();
	return io_service.run();
}

void Database::Release(void)
{
	assert(io_service.stopped());

	if (m_MySQL != NULL)
	{
		mysql_close(m_MySQL);
		m_MySQL = NULL;
	}

	m_bIsConnected = false;
}

char* Database::Escape(const char* query, unsigned int len)
{
	char* escaped = new char[len * 2 + 1];
	mysql_real_escape_string(m_MySQL, escaped, query, len);
	return escaped;
}

bool Database::Option(mysql_option option, const char* arg, std::string& error)
{
	if (mysql_options(m_MySQL, option, arg) > 0)
	{
		error.assign(mysql_error(m_MySQL));
		return false;
	}

	return true;
}

const char* Database::GetServerInfo()
{
	return mysql_get_server_info(m_MySQL);
}

const char* Database::GetHostInfo()
{
	return mysql_get_host_info(m_MySQL);
}

int Database::GetServerVersion()
{
	return mysql_get_server_version(m_MySQL);
}

bool Database::SetCharacterSet(const char* charset, std::string& error)
{
	if (mysql_set_character_set(m_MySQL, charset) > 0)
	{
		error.assign(mysql_error(m_MySQL));
		return false;
	}

	return true;
}

void Database::QueueQuery(const char* query, int callback, int callbackref, bool usenumbers)
{
	Query* newquery = new Query(query, callback, callbackref, usenumbers);
	io_service.post(std::bind(&Database::RunQuery, this, newquery));
}

void Database::QueueStatement(PStatement* stmt, MYSQL_BIND* binds, int callback, int callbackref, bool usenumbers)
{
	DatabaseAction* action = new DatabaseAction(callback, callbackref, usenumbers);
	io_service.post(std::bind(&Database::RunStatement, this, stmt, binds, action));
}

void Database::RunQuery(Query* query)
{
	std::string strquery = query->GetQuery();
	size_t len = strquery.length();

	bool hasRetried = false;
	unsigned int errorno = NULL;
#ifdef ENABLE_QUERY_TIMERS
	query->GetTimer()->Start();
#endif
	retry:
	if (mysql_real_query(m_MySQL, strquery.c_str(), len) != 0) {

		errorno = mysql_errno(m_MySQL);

		if (!hasRetried && ((errorno == CR_SERVER_LOST) || (errorno == CR_SERVER_GONE_ERROR) || (errorno != CR_CONN_HOST_ERROR) || (errorno == 1053)/*ER_SERVER_SHUTDOWN*/ || (errorno != CR_CONNECTION_ERROR))) {
			hasRetried = true;
			goto retry;
		}
	}
#ifdef ENABLE_QUERY_TIMERS
	query->GetTimer()->Stop();
#endif
	if (errorno != 0)
		query->AddResult(new Result(errorno, mysql_error(m_MySQL)));
	else {
		int status = 0;
		while (status != -1) {
			query->AddResult(new Result(m_MySQL));

			status = mysql_next_result(m_MySQL);
		}
	}

	m_completedActions.push(query);
}

void Database::RunStatement(PStatement* stmt, MYSQL_BIND* binds, DatabaseAction* action)
{
	stmt->Execute(binds, action);

	m_completedActions.push(action);
}

void Database::TriggerCallback(lua_State* state)
{
	m_bIsPendingCallback = false;

	if (GetCallback() >= 0)
	{
		LUA->ReferencePush(GetCallback());

		if (!LUA->IsType(-1, Type::Function))
		{
			LUA->Pop();
			return;
		}

		PushHandle(state);

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

void Database::PushHandle(lua_State* state)
{
	LUA->PushUserType(this, tmysql::iDatabaseMTID);
	LUA->PushMetaTable(tmysql::iDatabaseMTID);
	LUA->SetMetaTable(-2);
}

void Database::Disconnect(lua_State* state)
{
	Shutdown();

	DispatchCompletedActions(state);

	while (RunShutdownWork())
		DispatchCompletedActions(state);

	Release();

	delete this;
}

void Database::DispatchCompletedActions(lua_State* state)
{
	DatabaseAction* completed = GetCompletedActions();

	while (completed)
	{
		DatabaseAction* action = completed;

		if (!tmysql::inShutdown)
			action->TriggerCallback(state);

		completed = action->next;
		delete action;
	}
}

PStatement* Database::CreateStatement(lua_State* state)
{
	const char* query = LUA->CheckString(2);

	PStatement* stmt = new PStatement(m_MySQL, this, query);

	return stmt;
}


#pragma region Lua Exports
int Database::lua___gc(lua_State* state)
{
	LUA->CheckType(1, tmysql::iDatabaseMTID);
	Database* mysqldb = LUA->GetUserType<Database>(1, tmysql::iDatabaseMTID);

	if (mysqldb != nullptr)
		mysqldb->Disconnect(state);

	return 0;

}

int Database::lua__tostring(lua_State* state)
{
	LUA->CheckType(1, tmysql::iDatabaseMTID);

	Database* mysqldb = LUA->GetUserType<Database>(1, tmysql::iDatabaseMTID);
	
	if (mysqldb != nullptr) {
		char buff[100] = { 0 };
		sprintf_s(buff, 100, "[Tmysql database: %s]", mysqldb->m_strDB);
		LUA->PushString(buff);
		return 1;
	}
	
	return 0;
}

int Database::lua_IsValid(lua_State* state)
{
	LUA->CheckType(1, tmysql::iDatabaseMTID);

	Database* mysqldb = LUA->GetUserType<Database>(1, tmysql::iDatabaseMTID);

	LUA->PushBool(mysqldb != NULL);

	return 1;
}

int Database::lua_Query(lua_State* state)
{
	LUA->CheckType(1, tmysql::iDatabaseMTID);

	Database* mysqldb = LUA->GetUserType<Database>(1, tmysql::iDatabaseMTID);

	if (mysqldb == nullptr) {
		LUA->ThrowError("Attempted to call Query on a shutdown database");
		return 0;
	}

	if (!mysqldb->IsConnected()) {
		LUA->ThrowError("Attempted to call Query on a disconnected database");
		return 0;
	}

	const char* query = LUA->CheckString(2);

	int callbackfunc = -1;
	if (LUA->GetType(3) == Type::Function)
	{
		LUA->Push(3);
		callbackfunc = LUA->ReferenceCreate();
	}

	int callbackref = -1;
	int callbackobj = LUA->GetType(4);
	if (callbackobj != Type::Nil)
	{
		LUA->Push(4);
		callbackref = LUA->ReferenceCreate();
	}

	mysqldb->QueueQuery(query, callbackfunc, callbackref, LUA->GetBool(5));
	return 0;
}

int Database::lua_Prepare(lua_State* state)
{
	LUA->CheckType(1, tmysql::iDatabaseMTID);

	Database* mysqldb = LUA->GetUserType<Database>(1, tmysql::iDatabaseMTID);

	if (mysqldb == nullptr) {
		LUA->PushNil();
		LUA->PushString("Attempted to call Query on a shutdown database");
		return 2;
	}

	if (!mysqldb->IsConnected()) {
		LUA->PushNil();
		LUA->PushString("Attempted to call Query on a disconnected database");
		return 2;
	}

	PStatement* stmt;

	try {
		stmt = mysqldb->CreateStatement(state);
	}
	catch (const my_exception& error)
	{
		LUA->PushNil();
		LUA->PushString(error.what());
		return 2;
	}

	stmt->PushHandle(state);
	return 1;
}

int Database::lua_Escape(lua_State* state)
{
	LUA->CheckType(1, tmysql::iDatabaseMTID);

	Database* mysqldb = LUA->GetUserType<Database>(1, tmysql::iDatabaseMTID);

	if (mysqldb == nullptr) {
		LUA->ThrowError("Attempted to call Escape on a shutdown database");
		return 0;
	}

	if (!mysqldb->IsConnected()) {
		LUA->ThrowError("Attempted to call Escape on a disconnected database");
		return 0;
	}

	LUA->CheckType(2, Type::String);

	unsigned int len;
	const char* query = LUA->GetString(2, &len);

	char* escaped = mysqldb->Escape(query, len);
	LUA->PushString(escaped);

	delete[] escaped;
	return 1;
}

int Database::lua_SetOption(lua_State* state)
{
	LUA->CheckType(1, tmysql::iDatabaseMTID);

	Database* mysqldb = LUA->GetUserType<Database>(1, tmysql::iDatabaseMTID);

	if (mysqldb == nullptr) {
		LUA->ThrowError("Attempted to call Option on a shutdown database");
		return 0;
	}

	std::string error;

	mysql_option option = static_cast<mysql_option>((int)LUA->CheckNumber(2));

	LUA->PushBool(mysqldb->Option(option, LUA->CheckString(3), error));
	LUA->PushString(error.c_str());
	return 2;
}

int Database::lua_GetServerInfo(lua_State* state)
{
	LUA->CheckType(1, tmysql::iDatabaseMTID);

	Database* mysqldb = LUA->GetUserType<Database>(1, tmysql::iDatabaseMTID);

	if (mysqldb == nullptr) {
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

int Database::lua_GetDatabase(lua_State* state)
{
	LUA->CheckType(1, tmysql::iDatabaseMTID);

	Database* mysqldb = LUA->GetUserType<Database>(1, tmysql::iDatabaseMTID);
	
	if (mysqldb != nullptr) {
		LUA->PushString(mysqldb->m_strDB);
		return 1;
	}
	
	return 0;
}

int Database::lua_GetHostInfo(lua_State* state)
{
	LUA->CheckType(1, tmysql::iDatabaseMTID);

	Database* mysqldb = LUA->GetUserType<Database>(1, tmysql::iDatabaseMTID);

	if (mysqldb == nullptr) {
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

int Database::lua_GetServerVersion(lua_State* state)
{
	LUA->CheckType(1, tmysql::iDatabaseMTID);

	Database* mysqldb = LUA->GetUserType<Database>(1, tmysql::iDatabaseMTID);

	if (mysqldb == nullptr) {
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

int Database::lua_Connect(lua_State* state)
{
	LUA->CheckType(1, tmysql::iDatabaseMTID);

	Database* mysqldb = LUA->GetUserType<Database>(1, tmysql::iDatabaseMTID);

	if (mysqldb == nullptr) {
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

int Database::lua_IsConnected(lua_State* state)
{
	LUA->CheckType(1, tmysql::iDatabaseMTID);

	Database* mysqldb = LUA->GetUserType<Database>(1, tmysql::iDatabaseMTID);

	if (mysqldb == nullptr) {
		LUA->ThrowError("Attempted to call IsConnected on a shutdown database");
		return 0;
	}

	LUA->PushBool(mysqldb->IsConnected());
	return 1;
}

int Database::lua_Disconnect(lua_State* state)
{
	LUA->CheckType(1, tmysql::iDatabaseMTID);

	Database* mysqldb = LUA->GetUserType<Database>(1, tmysql::iDatabaseMTID);

	if (mysqldb == nullptr) {
		LUA->ThrowError("Attempted to call Disconnect on a shutdown database");
		return 0;
	}

	mysqldb->Disconnect(state);

	return 0;
}

int Database::lua_SetCharacterSet(lua_State* state)
{
	LUA->CheckType(1, tmysql::iDatabaseMTID);

	Database* mysqldb = LUA->GetUserType<Database>(1, tmysql::iDatabaseMTID);

	if (mysqldb == nullptr) {
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

int Database::lua_Poll(lua_State* state)
{
	LUA->CheckType(1, tmysql::iDatabaseMTID);

	Database* mysqldb = LUA->GetUserType<Database>(1, tmysql::iDatabaseMTID);

	if (mysqldb == nullptr) {
		LUA->ThrowError("Attempted to call Poll on a shutdown database");
		return 0;
	}

	if (mysqldb->IsPendingCallback())
		mysqldb->TriggerCallback(state);

	mysqldb->DispatchCompletedActions(state);
	return 0;
}
#pragma endregion
