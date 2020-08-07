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

inline std::string get_working_dir() { char buff[FILENAME_MAX]; GetCurrentDir(buff, FILENAME_MAX); return std::string(buff); }

// From the boost atomic examples	
template<typename T>
class waitfree_query_queue {
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

	waitfree_query_queue() : head_(0) {}

	T * pop_all_reverse(void)
	{
		return head_.exchange(0, std::memory_order_acquire);
	}
private:
	std::atomic<T *> head_;
};

typedef waitfree_query_queue<Query> QueryQueue;

class Database
{
public:
	Database(std::string host, std::string user, std::string pass, std::string db, unsigned int port, std::string socket, unsigned long flags, int callback);
	~Database(void);

	bool			Initialize(std::string& error);
	bool			IsConnected(void) { return m_bIsConnected; };
	bool			IsPendingCallback() { return m_bIsPendingCallback; }
	void			TriggerCallback(lua_State* state);
	void			PushHandle(lua_State* state);

	void			Disconnect(lua_State* state);

	// Lua metamethods
	static int		lua_IsValid(lua_State* state);
	static int		lua_Query(lua_State* state);
	static int		lua_Escape(lua_State* state);
	static int		lua_SetOption(lua_State* state);
	static int		lua_GetServerInfo(lua_State* state);
	static int		lua_GetHostInfo(lua_State* state);
	static int		lua_GetServerVersion(lua_State* state);
	static int		lua_Connect(lua_State* state);
	static int		lua_IsConnected(lua_State* state);
	static int		lua_Disconnect(lua_State* state);
	static int		lua_SetCharacterSet(lua_State* state);
	static int		lua_Poll(lua_State* state);

private:
	MYSQL*			m_MySQL;

	std::string		m_strHost;
	std::string		m_strUser;
	std::string		m_strPass;
	std::string		m_strDB;
	unsigned int	m_iPort;
	std::string		m_strSocket;
	unsigned long	m_iClientFlags;

	int				m_iTableIndex;
	int				GetTableIndex(void) { return m_iTableIndex; };

	bool			m_bIsConnected;

	bool			m_bIsPendingCallback;
	int				m_iCallback;
	int				GetCallback() { return m_iCallback; }

	bool			Connect(std::string& error);
	void			Shutdown(void);
	std::size_t		RunShutdownWork(void);
	void			Release(void);

	void			RunQuery(Query* query);

	Query*			GetCompletedQueries() { return m_completedQueries.pop_all(); }
	void			PushCompleted(Query* query) { m_completedQueries.push(query); }
	void			DispatchCompletedQueries(lua_State* state);

	bool			SetCharacterSet(const char* charset, std::string& error);
	char*			Escape(const char* query, unsigned int len);
	bool			Option(mysql_option option, const char* arg, std::string& error);
	const char*		GetServerInfo();
	const char*		GetHostInfo();
	int				GetServerVersion();

	void			QueueQuery(const char* query, int callback = -1, int callbackref = -1, bool usenumbers = false);
	QueryQueue		m_completedQueries;

	std::vector<std::thread> thread_group;
	asio::io_service io_service;
	std::auto_ptr<asio::io_service::work> work;
};

#endif