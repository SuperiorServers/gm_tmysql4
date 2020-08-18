#include "main.h"
#include "tmysql.h"
#include "statement.h"
#include "database.h"

using namespace GarrysMod::Lua;

PStatement::PStatement(MYSQL* conn, Database* parent, const char* query)
{
	m_database = parent;
	m_stmt = mysql_stmt_init(conn);

	if (m_stmt == nullptr)
		ThrowException(conn);

	bool attr = 1;
	mysql_stmt_attr_set(m_stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &attr);

	if (mysql_stmt_prepare(m_stmt, query, strlen(query)) != 0)
		ThrowException(conn);

	m_numArgs = mysql_stmt_param_count(m_stmt);
}

PStatement::~PStatement(void)
{
	if (m_stmt != nullptr)
		mysql_stmt_close(m_stmt);
}

void PStatement::PushHandle(lua_State* state) {
	LUA->PushUserType(this, tmysql::iStatementMTID);
	LUA->PushMetaTable(tmysql::iStatementMTID);
	LUA->SetMetaTable(-2);
}

void PStatement::Execute(MYSQL_BIND* binds, DatabaseAction* action) {
	bool hasRetried = false;
	unsigned int errorno = NULL;

	retry:
	mysql_stmt_bind_param(m_stmt, binds);
	if (mysql_stmt_execute(m_stmt) != 0) {
		errorno = mysql_stmt_errno(m_stmt);

		if (!hasRetried && ((errorno == CR_SERVER_LOST) || (errorno == CR_SERVER_GONE_ERROR) || (errorno != CR_CONN_HOST_ERROR) || (errorno == 1053)/*ER_SERVER_SHUTDOWN*/ || (errorno != CR_CONNECTION_ERROR))) {
			hasRetried = true;
			goto retry;
		}
	}

	if (errorno != 0)
		action->AddResult(new Result(errorno, mysql_stmt_error(m_stmt)));
	else {
		int status = 0;
		while (status != -1) {
			mysql_stmt_store_result(m_stmt);
			action->AddResult(new Result(this));
			status = mysql_stmt_next_result(m_stmt);
		}
	}

	for (int l = 0; l < m_numArgs; l++)
		delete[] binds[l].buffer;

	delete[] binds;
}

int PStatement::lua___gc(lua_State* state)
{
	LUA->CheckType(1, tmysql::iStatementMTID);

	PStatement* pstmt = LUA->GetUserType<PStatement>(1, tmysql::iStatementMTID);
	delete pstmt;

	return 0;
}

int PStatement::lua_IsValid(lua_State* state)
{
	LUA->CheckType(1, tmysql::iStatementMTID);

	PStatement* pstmt = LUA->GetUserType<PStatement>(1, tmysql::iStatementMTID);
	LUA->PushBool(pstmt != nullptr);

	return 1;
}

int PStatement::lua_GetArgCount(lua_State* state)
{
	LUA->CheckType(1, tmysql::iStatementMTID);

	PStatement* pstmt = LUA->GetUserType<PStatement>(1, tmysql::iStatementMTID);
	LUA->PushNumber(mysql_stmt_param_count(pstmt->GetInternal()));

	return 1;
}

int PStatement::lua_Run(lua_State* state)
{
	LUA->CheckType(1, tmysql::iStatementMTID);

	PStatement* stmt = LUA->GetUserType<PStatement>(1, tmysql::iStatementMTID);

	if (stmt == nullptr) {
		LUA->ThrowError("Attempted to call Run on an invalidated prepared statement");
		return 0;
	}

	if (!stmt->m_database->IsConnected()) {
		LUA->ThrowError("Attempted to call Run with a disconnected database");
		return 0;
	}

	// We always need all parameters, everything afterward is optional
	if (LUA->Top() < stmt->m_numArgs + 1)
	{
		LUA->ThrowError("Attempting to call Run without enough arguments for prepared statement");
		return 0;
	}

	MYSQL_BIND* binds = new MYSQL_BIND[stmt->m_numArgs];
	memset(binds, 0, sizeof(MYSQL_BIND) * stmt->m_numArgs);

	for (int i = 0; i < stmt->m_numArgs; i++)
	{
		my_bool err;
		binds[i].error = &err;

		Type::Enum type = (Type::Enum)LUA->GetType(i + 2);
		switch (type)
		{
		case Type::Enum::String:
		{
			unsigned int len;
			auto str = LUA->GetString(i + 2, &len);

			binds[i].buffer_type = MYSQL_TYPE_STRING;
			binds[i].buffer_length = len;
			binds[i].buffer = (void*)str;
			break;
		}
		case Type::Enum::Bool:
		{
			binds[i].buffer_type = MYSQL_TYPE_BIT;
			binds[i].buffer = (void*)new bool(LUA->GetBool(i + 2));
			break;
		}
		case Type::Enum::Number:
		{
			binds[i].buffer_type = MYSQL_TYPE_DOUBLE;
			binds[i].buffer = (void*)new double(LUA->GetNumber(i + 2));
			break;
		}
		case Type::Enum::Nil:
		{
			binds[i].buffer_type = MYSQL_TYPE_NULL;
			break;
		}
		default:
		{
			for (int l = 0; l < i; l++)
				delete[] binds[l].buffer;

			delete[] binds;

			char err[256];
			sprintf(err, "Unexpected type '%s' in prepared parameter #%i: expected string, number, boolean, or nil", Type::Name[type], i + 1);
			LUA->ThrowError(err);

			return 0;
		}
		}
	}

	int callbackfunc = -1;
	if (LUA->GetType(stmt->m_numArgs + 2) == Type::Function)
	{
		LUA->Push(stmt->m_numArgs + 2);
		callbackfunc = LUA->ReferenceCreate();
	}

	int callbackref = -1;
	int callbackobj = LUA->GetType(stmt->m_numArgs + 3);
	if (callbackobj != Type::Nil)
	{
		LUA->Push(stmt->m_numArgs + 3);
		callbackref = LUA->ReferenceCreate();
	}

	stmt->m_database->QueueStatement(stmt, binds, callbackfunc, callbackref, LUA->GetBool(stmt->m_numArgs + 4));

	return 0;
}