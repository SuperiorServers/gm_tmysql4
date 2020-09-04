#ifndef TMYSQL4_DATABASE
#define TMYSQL4_DATABASE

#define ASIO_STANDALONE 
#define ASIO_HAS_STD_ADDRESSOF
#define ASIO_HAS_STD_ARRAY
#define ASIO_HAS_CSTDINT
#define ASIO_HAS_STD_SHARED_PTR
#define ASIO_HAS_STD_TYPE_TRAITS

#include <deque>
#include <atomic>
#include <mutex>
#include <thread>
#include <string>
#include <functional>
#include "asio.hpp"
#include <errmsg.h>

#include "query.h"
#include <unordered_set>

class PStatement;

inline std::string get_working_dir() { char buff[FILENAME_MAX]; GetCurrentDir(buff, FILENAME_MAX); return std::string(buff); }

// From the boost atomic examples	
template<typename T>
class waitfree_action_queue {
public:
	void push(T * data)
	{
		T * stale_head = head_.load(std::memory_order_relaxed);
		do {
			data->next = stale_head;
		} while (!head_.compare_exchange_weak(stale_head, data, std::memory_order_release, std::memory_order_relaxed));
	}

	T * pop_all(void)
	{
		T * last = pop_all_reverse(), *first = 0;
		while (last) {
			T * tmp = last;
			last = last->next;
			tmp->next = first;
			first = tmp;
		}
		return first;
	}

	waitfree_action_queue() : head_(0) {}

	T * pop_all_reverse(void)
	{
		return head_.exchange(0, std::memory_order_acquire);
	}
private:
	std::atomic<T *> head_;
};

typedef waitfree_action_queue<DatabaseAction> ActionQueue;

class Database
{
public:
	Database(const char* host, const char* user, const char* pass, const char* db, unsigned int port, const char* socket, unsigned long flags, int callback);
	~Database(void);

	bool			Initialize(std::string& error);
	bool			IsConnected(void) { return m_bIsConnected; };
	bool			IsPendingCallback() { return m_bIsPendingCallback; }
	void			TriggerCallback(lua_State* state);
	void			PushHandle(lua_State* state);
	void			NullifyReference(lua_State* state);
	void			DeregisterPStatement(PStatement* stmt) { m_preparedStatements.erase(stmt); }

	PStatement*		CreateStatement(lua_State* state);
	void			QueueStatement(PStatement* stmt, MYSQL_BIND* binds, int callback = -1, int callbackref = -1, bool usenumbers = false);

	void			Disconnect(lua_State* state);

	// Lua metamethods
	static int		lua___gc(lua_State* state);
	static int		lua__tostring(lua_State* state);
	static int		lua_IsValid(lua_State* state);
	static int		lua_Query(lua_State* state);
	static int		lua_Prepare(lua_State* state);
	static int		lua_Escape(lua_State* state);
	static int		lua_SetOption(lua_State* state);
	static int		lua_GetServerInfo(lua_State* state);
	static int		lua_GetDatabase(lua_State* state);
	static int		lua_GetHostInfo(lua_State* state);
	static int		lua_GetServerVersion(lua_State* state);
	static int		lua_Connect(lua_State* state);
	static int		lua_IsConnected(lua_State* state);
	static int		lua_Disconnect(lua_State* state);
	static int		lua_SetCharacterSet(lua_State* state);
	static int		lua_Poll(lua_State* state);

private:
	MYSQL*			m_MySQL;

	char			m_strHost[253] = { 0 };
	char			m_strUser[32] = { 0 };
	char			m_strPass[32] = { 0 };
	char			m_strDB[64] = { 0 };
	unsigned int	m_iPort;
	char			m_strSocket[107] = { 0 };
	unsigned long	m_iClientFlags;

	bool			m_bIsConnected;
	bool			m_bIsInGC;

	int				m_iLuaRef = 0;
	bool			m_bIsPendingCallback;
	int				m_iCallback;

	bool			Connect(std::string& error);
	void			Release(lua_State* state);

	void			RunQuery(Query* query);
	void			RunStatement(PStatement* stmt, MYSQL_BIND* binds, DatabaseAction* action);

	DatabaseAction* GetCompletedActions() { return m_completedActions.pop_all(); }
	void			DispatchCompletedActions(lua_State* state);

	bool			SetCharacterSet(const char* charset, std::string& error);
	char*			Escape(const char* query, unsigned int len, unsigned int* out_len);
	bool			Option(mysql_option option, const char* arg, std::string& error);
	const char*		GetServerInfo();
	const char*		GetHostInfo();
	int				GetServerVersion();

	void			QueueQuery(const char* query, int callback = -1, int callbackref = -1, bool usenumbers = false);

	std::unordered_set<PStatement*> m_preparedStatements;

	ActionQueue		m_completedActions;

	std::unique_ptr<std::thread> io_thread;
	asio::io_context io_context;
	asio::executor_work_guard<asio::io_context::executor_type> io_work;
};

#endif
