#include "main.h"
#include "tmysql.h"
#include "statement.h"
#include "database.h"

using namespace GarrysMod::Lua;

static void deleteBinds(MYSQL_BIND* binds, int count)
{
	for (int l = 0; l < count; l++)
		switch (binds[l].buffer_type)
		{
		case MYSQL_TYPE_NULL:
			break;
		case MYSQL_TYPE_STRING:
			delete[] binds[l].buffer;
			break;
		default:
			delete binds[l].buffer;
			break;
		}

	delete[] binds;
}

PStatement::PStatement(MYSQL* conn, Database* parent, const char* query) :
	m_database(parent),
	m_query(query),
	m_stmt(0),
	m_numArgs(0)
{
	Prepare(conn);
}

PStatement::~PStatement(void)
{
	if (m_stmt != nullptr)
		mysql_stmt_close(m_stmt);
}

void PStatement::Prepare(MYSQL* conn)
{
	if (m_stmt != nullptr)
		mysql_stmt_close(m_stmt);

	m_stmt = mysql_stmt_init(conn);

	if (m_stmt == nullptr)
		ThrowException(conn);

	bool attr = 1;
	mysql_stmt_attr_set(m_stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &attr);

	if (mysql_stmt_prepare(m_stmt, m_query.c_str(), m_query.length()) != 0)
		ThrowException(conn);

	m_numArgs = mysql_stmt_param_count(m_stmt);
}

void PStatement::PushHandle(lua_State* state) {
	if (m_iLuaRef == 0)
		LUA->PushUserType(this, tmysql::iStatementMTID),
		m_iLuaRef = LUA->ReferenceCreate();

	LUA->ReferencePush(m_iLuaRef);
}

void PStatement::Release(lua_State* state)
{
	PushHandle(state);
	LUA->SetUserType(-1, NULL);
	LUA->Pop();

	LUA->ReferenceFree(m_iLuaRef);

	delete this;
}

void PStatement::Execute(MYSQL_BIND* binds, DatabaseAction* action) {
	bool hasRetried = false;
	bool hasRetriedStatement = false;
	unsigned int errorno;

	for (int i = 0; i < m_numArgs; i++)
		if (binds[i].buffer_type == MYSQL_TYPE_STRING)
			if (strlen(static_cast<char*>(binds[i].buffer)) != binds[i].buffer_length)
				binds[i].buffer_type = MYSQL_TYPE_BLOB;

	StartQueryTimer(action);

retry:
	errorno = 0;
	mysql_stmt_bind_param(m_stmt, binds);
	if (mysql_stmt_execute(m_stmt) != 0) {
		errorno = mysql_stmt_errno(m_stmt);

		if (!hasRetried && (errorno == CR_SERVER_LOST || errorno == CR_SERVER_GONE_ERROR || errorno == 1053 /* Server shutdown in progress */)) {
			std::string err;
			hasRetried = true;

			if (m_database->Connect(err, true))
				goto retry;
		}

		if (!hasRetriedStatement && errorno == 1243) { // Unknown prepared statement handler - often happens once after reconnecting?
			Prepare(m_database->GetMySQLHandle());
			hasRetriedStatement = true;
			goto retry;
		}
	}

	EndQueryTimer(action);

	if (errorno != 0)
		action->AddResult(new Result (errorno, mysql_stmt_error(m_stmt)));
	else {
		int status = 0;
		while (status != -1) {
			mysql_stmt_store_result(m_stmt);
			action->AddResult(new Result(this));
			status = mysql_stmt_next_result(m_stmt);
		}
	}

	deleteBinds(binds, m_numArgs);
}

PStatement* getPStatementFromStack(lua_State* state, bool throwNullError)
{
	LUA->CheckType(1, tmysql::iStatementMTID);
	PStatement* pstmt = LUA->GetUserType<PStatement>(1, tmysql::iStatementMTID);

	if (throwNullError && pstmt == nullptr)
		LUA->ThrowError("Tried to use a NULL tmysql prepared statement!");

	return pstmt;
}

int PStatement::lua___gc(lua_State* state)
{
	PStatement* pstmt = getPStatementFromStack(state, false);

	if (pstmt != nullptr)
		pstmt->m_database->DeregisterStatement(pstmt->GetQuery()),
		pstmt->Release(state);

	return 0;
}

int PStatement::lua__tostring(lua_State* state)
{
	PStatement* pstmt = getPStatementFromStack(state, false);

	if (pstmt != nullptr)
		LUA->PushString("[tmysql prepared statement]");
	else
		LUA->PushString("[NULL tmysql prepared statement]");

	return 1;
}

int PStatement::lua_IsValid(lua_State* state)
{
	PStatement* pstmt = getPStatementFromStack(state, false);

	LUA->PushBool(pstmt != nullptr);
	return 1;
}

int PStatement::lua_GetArgCount(lua_State* state)
{
	PStatement* pstmt = getPStatementFromStack(state, true);

	LUA->PushNumber(mysql_stmt_param_count(pstmt->GetInternal()));
	return 1;
}

int PStatement::lua_Run(lua_State* state)
{
	PStatement* pstmt = getPStatementFromStack(state, true);

	// We always need all parameters, everything afterward is optional
	if (LUA->Top() < pstmt->m_numArgs + 1)
		LUA->ThrowError("Attempting to call Run without enough arguments for prepared statement");

	MYSQL_BIND* binds = new MYSQL_BIND[pstmt->m_numArgs];
	memset(binds, 0, sizeof(MYSQL_BIND) * pstmt->m_numArgs);

	for (int i = 0; i < pstmt->m_numArgs; i++)
	{
		my_bool err;
		binds[i].error = &err;

		int type = LUA->GetType(i + 2);
		switch (type)
		{
		case Type::Enum::String:
		{
			unsigned int len;
			const char* str = LUA->GetString(i + 2, &len);

			binds[i].buffer_type = MYSQL_TYPE_STRING;
			binds[i].buffer_length = len;
			binds[i].buffer = new char[len];

			memcpy(binds[i].buffer, str, len);

			break;
		}
		case Type::Enum::Bool:
		{
			binds[i].buffer_type = MYSQL_TYPE_BIT;
			binds[i].buffer = new bool(LUA->GetBool(i + 2));
			break;
		}
		case Type::Enum::Number:
		{
			binds[i].buffer_type = MYSQL_TYPE_DOUBLE;
			binds[i].buffer = new double(LUA->GetNumber(i + 2));
			break;
		}
		case Type::Enum::Nil:
		{
			binds[i].buffer_type = MYSQL_TYPE_NULL;
			break;
		}
		default:
		{
			deleteBinds(binds, i);

			char err[256];
			sprintf(err, "Unexpected type '%s' in prepared parameter #%i: expected string, number, boolean, or nil", LUA->GetTypeName(type), i + 1);
			LUA->ThrowError(err);
		}
		}
	}

	int callbackfunc = -1;
	if (LUA->GetType(pstmt->m_numArgs + 2) == Type::Function)
	{
		LUA->Push(pstmt->m_numArgs + 2);
		callbackfunc = LUA->ReferenceCreate();
	}

	int callbackref = -1;
	int callbackobj = LUA->GetType(pstmt->m_numArgs + 3);
	if (callbackobj != Type::Nil)
	{
		LUA->Push(pstmt->m_numArgs + 3);
		callbackref = LUA->ReferenceCreate();
	}

	pstmt->m_database->QueueStatement(pstmt, binds, callbackfunc, callbackref, LUA->GetBool(pstmt->m_numArgs + 4));

	return 0;
}
