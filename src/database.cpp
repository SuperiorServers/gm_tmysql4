#include "main.h"
#include "statement.h"
#include "database.h"
#include "tmysql.h"

using namespace GarrysMod::Lua;

Database::Database(const char* host, const char* user, const char* pass, const char* db, unsigned int port, const char* socket, unsigned long flags, int callback)
	: m_iPort(port), m_iClientFlags(flags), m_iCallback(callback), m_bIsConnected(false), m_MySQL(NULL), m_bIsInGC(false), io_context(), io_work(io_context.get_executor())
{
	strncpy(m_strHost, host, 253);
	strncpy(m_strUser, user, 32);
	strncpy(m_strPass, pass, 32);
	strncpy(m_strDB, db, 64);
	strncpy(m_strSocket, socket, 107);
}

Database::~Database(void)
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

	io_thread = std::make_unique<std::thread>([&]() {io_context.run(); });

	m_bIsConnected = true;
	return true;
}

bool Database::Connect(std::string& error)
{
	const char* socket = (strlen(m_strSocket) == 0) ? nullptr : m_strSocket;
	unsigned int flags = m_iClientFlags | CLIENT_MULTI_RESULTS;

	my_bool tru = 1;
	std::string wkdir = get_working_dir() + "/garrysmod/lua/bin";

	if (mysql_options(m_MySQL, MYSQL_OPT_RECONNECT, &tru) > 0 ||
		mysql_options(m_MySQL, MYSQL_PLUGIN_DIR, wkdir.c_str()) > 0 ||
		mysql_real_connect(m_MySQL, m_strHost, m_strUser, m_strPass, m_strDB, m_iPort, socket, flags) != m_MySQL)
	{
		error.assign("[").append(std::to_string(mysql_errno(m_MySQL))).append("] ").append(mysql_error(m_MySQL));
		return false;
	}

	m_bIsPendingCallback = true;

	return true;
}

void Database::Release(lua_State* state)
{
	io_thread.release();

	if (m_MySQL != NULL)
	{
		mysql_close(m_MySQL);
		m_MySQL = NULL;
	}

	m_bIsConnected = false;

	for (auto [_, stmt] : m_preparedStatements)
		stmt->Release(state);

	NullifyReference(state);

	return;
}

char* Database::Escape(const char* query, unsigned int len, unsigned int* out_len)
{
	char* escaped = new char[len * 2 + 1];
	*out_len = mysql_real_escape_string(m_MySQL, escaped, query, len);
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
	asio::post(io_context, std::bind(&Database::RunQuery, this, newquery));
}

void Database::QueueStatement(PStatement* stmt, MYSQL_BIND* binds, int callback, int callbackref, bool usenumbers)
{
	DatabaseAction* action = new DatabaseAction(callback, callbackref, usenumbers);
	asio::post(io_context, std::bind(&Database::RunStatement, this, stmt, binds, action));
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

	if (m_iCallback >= 0)
	{
		LUA->GetField(LUA_GLOBAL, "debug");
		LUA->GetField(-1, "traceback");

		LUA->ReferencePush(m_iCallback);

		if (!LUA->IsType(-1, Type::Function))
		{
			LUA->Pop(3);
			return;
		}

		PushHandle(state);

		if (LUA->PCall(1, 0, -3))
			ErrorNoHalt(std::string("[tmysql callback error]\n").append(LUA->GetString(-1))),
			LUA->Pop(2);

		LUA->ReferenceFree(m_iCallback);
		m_iCallback = 0;
	}
}

void Database::PushHandle(lua_State* state)
{
	if (m_iLuaRef == 0)
	{
		LUA->PushUserType(this, tmysql::iDatabaseMTID);
		m_iLuaRef = LUA->ReferenceCreate();
	}

	LUA->ReferencePush(m_iLuaRef);
}

void Database::NullifyReference(lua_State* state)
{
	PushHandle(state);
	LUA->SetUserType(-1, NULL);
	LUA->Pop();

	LUA->ReferenceFree(m_iLuaRef);
}

void Database::Disconnect(lua_State* state)
{
	io_work.reset();
	while (!io_context.stopped());

	DispatchCompletedActions(state);

	Release(state);

	tmysql::DeregisterDatabase(this);

	delete this;
}

void Database::DispatchCompletedActions(lua_State* state)
{
	DatabaseAction* completed = GetCompletedActions();

	while (completed)
	{
		DatabaseAction* action = completed;

		if (!m_bIsInGC)
			action->TriggerCallback(state);

		completed = action->next;
		delete action;
	}
}

void Database::CreateStatement(const char* query, statementCreateWaiter* task)
{
	try {
		PStatement* stmt = new PStatement(m_MySQL, this, query);
		task->stmt = stmt;
	}
	catch (const my_exception& e)
	{
		task->errorNumber = e.errorNumber;
		task->errorMsg = e.what();
	}

	task->completed = true;
}

Database* getDatabaseFromStack(lua_State* state, bool throwNullError, bool throwDisconnectedError)
{
	LUA->CheckType(1, tmysql::iDatabaseMTID);
	Database* mysqldb = LUA->GetUserType<Database>(1, tmysql::iDatabaseMTID);

	if (throwNullError && mysqldb == nullptr)
		LUA->ThrowError("Tried to use a NULL tmysql database!");

	if (throwDisconnectedError && !mysqldb->IsConnected())
		LUA->ThrowError("Tried to use a disconnected tmysql database!");

	return mysqldb;
}

#pragma region Lua Exports
int Database::lua___gc(lua_State* state)
{
	Database* mysqldb = getDatabaseFromStack(state, false, false);

	if (mysqldb != nullptr)
	{
		mysqldb->m_bIsInGC = true;
		mysqldb->Disconnect(state);
	}

	return 0;
}

int Database::lua__tostring(lua_State* state)
{
	Database* mysqldb = getDatabaseFromStack(state, false, false);

	if (mysqldb != nullptr) {
		char buff[378] = { 0 };
		sprintf(buff, "[tmysql] %s:%s@:%s:%s", mysqldb->m_strUser, mysqldb->m_strDB, mysqldb->m_strHost, std::to_string(mysqldb->m_iPort).c_str());
		LUA->PushString(buff);
		return 1;
	}

	LUA->PushString("[NULL tmysql database]");
	return 1;
}

int Database::lua_IsValid(lua_State* state)
{
	Database* mysqldb = getDatabaseFromStack(state, false, false);

	LUA->PushBool(mysqldb != nullptr);

	return 1;
}

int Database::lua_Query(lua_State* state)
{
	Database* mysqldb = getDatabaseFromStack(state, true, true);

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
	Database* mysqldb = getDatabaseFromStack(state, true, true);

	const char* query = LUA->CheckString(2);

	// Use cached statement when possible
	auto pstmt = mysqldb->m_preparedStatements.find(query);
	if (pstmt != mysqldb->m_preparedStatements.end())
	{
		pstmt->second->PushHandle(state);
		return 1;
	}

	struct statementCreateWaiter task;

	// The module may be handling pending queries at the moment.
	// We can't intrude on mysql commands or we'll get errors.
	// So we will post this and wait out the existing queue instead.
	// It shouldn't be a big deal - if you're preparing statements while
	// people are playing the game, you're doing something wrong.
	asio::post(mysqldb->io_context, std::bind(&Database::CreateStatement, mysqldb, query, &task));
	while (!task.completed);

	if (task.stmt == nullptr) // errored
	{
		LUA->PushNil();
		LUA->PushString(std::string("[").append(std::to_string(task.errorNumber)).append("] ").append(task.errorMsg).c_str());
		return 2;
	}

	mysqldb->m_preparedStatements.emplace(task.stmt->GetQuery(), task.stmt);

	task.stmt->PushHandle(state);
	return 1;
}

int Database::lua_Escape(lua_State* state)
{
	Database* mysqldb = getDatabaseFromStack(state, true, true);

	LUA->CheckType(2, Type::String);

	unsigned int len;
	const char* query = LUA->GetString(2, &len);

	char* escaped = mysqldb->Escape(query, len, &len);
	LUA->PushString(escaped, len);

	delete[] escaped;
	return 1;
}

int Database::lua_SetOption(lua_State* state)
{
	Database* mysqldb = getDatabaseFromStack(state, true, false);

	std::string error;

	mysql_option option = static_cast<mysql_option>((int)LUA->CheckNumber(2));

	LUA->PushBool(mysqldb->Option(option, LUA->CheckString(3), error));
	LUA->PushString(error.c_str());
	return 2;
}

int Database::lua_GetServerInfo(lua_State* state)
{
	Database* mysqldb = getDatabaseFromStack(state, true, true);

	LUA->PushString(mysqldb->GetServerInfo());
	return 1;
}

int Database::lua_GetDatabase(lua_State* state)
{
	Database* mysqldb = getDatabaseFromStack(state, true, false);

	LUA->PushString(mysqldb->m_strDB);
	return 1;
}

int Database::lua_GetHostInfo(lua_State* state)
{
	Database* mysqldb = getDatabaseFromStack(state, true, true);

	LUA->PushString(mysqldb->GetHostInfo());
	return 1;
}

int Database::lua_GetServerVersion(lua_State* state)
{
	Database* mysqldb = getDatabaseFromStack(state, true, true);

	LUA->PushNumber(mysqldb->GetServerVersion());
	return 1;
}

int Database::lua_Connect(lua_State* state)
{
	Database* mysqldb = getDatabaseFromStack(state, true, false);

	if (mysqldb->IsConnected())
		LUA->ThrowError("Tried to use an already connected tmysql database!");

	std::string error;
	bool success = mysqldb->Initialize(error);

	LUA->PushBool(success);

	if (!success)
	{
		LUA->PushString(error.c_str());

		mysqldb->Release(state);
		delete mysqldb;

		return 2;
	}
	return 1;
}

int Database::lua_IsConnected(lua_State* state)
{
	Database* mysqldb = getDatabaseFromStack(state, true, false);

	LUA->PushBool(mysqldb->IsConnected());
	return 1;
}

int Database::lua_Disconnect(lua_State* state)
{
	Database* mysqldb = getDatabaseFromStack(state, true, true);

	mysqldb->Disconnect(state);

	return 0;
}

int Database::lua_SetCharacterSet(lua_State* state)
{
	Database* mysqldb = getDatabaseFromStack(state, true, true);

	const char* set = LUA->CheckString(2);

	std::string error;
	LUA->PushBool(mysqldb->SetCharacterSet(set, error));
	LUA->PushString(error.c_str());
	return 2;
}

int Database::lua_Poll(lua_State* state)
{
	// A disconnected database may possibly still have some pending action callbacks, so we don't want to error out on them
	Database* mysqldb = getDatabaseFromStack(state, true, false);

	if (mysqldb->IsPendingCallback())
		mysqldb->TriggerCallback(state);

	mysqldb->DispatchCompletedActions(state);
	return 0;
}
#pragma endregion
