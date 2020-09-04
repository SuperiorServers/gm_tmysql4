#ifndef TMYSQL4_STATEMENT
#define TMYSQL4_STATEMENT

class Database;
class DatabaseAction;

class PStatement {
public:
	PStatement(MYSQL* conn, Database* parent, const char* query);
	~PStatement(void);

	void		PushHandle(lua_State* state);
	void		Execute(MYSQL_BIND* binds, DatabaseAction* action);
	void		Release(lua_State* state);

	MYSQL_STMT* GetInternal() { return m_stmt; }

	static int	lua___gc(lua_State* state);
	static int	lua_IsValid(lua_State* state);
	static int	lua_GetArgCount(lua_State* state);
	static int	lua_Run(lua_State* state);
	static int	lua__tostring(lua_State* state);
private:
	Database*	m_database;
	MYSQL_STMT* m_stmt;

	int			m_numArgs;
	int			m_iLuaRef = 0;
};

#endif