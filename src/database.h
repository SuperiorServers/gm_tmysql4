#include <deque>
#include <atomic>
#include <mutex>
#include <thread>
#include <string>
#include <boost/asio.hpp>
#include "timer.h"

using namespace boost;

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

class Result
{
public:
	Result()
	{
	}

	~Result(void)
	{
		mysql_free_result(m_pResult);
	}

	void				SetErrorID(int error) { m_iError = error; }
	const int			GetErrorID(void) { return m_iError; }

	void				SetError(const char* error) { m_strError.assign(error); }
	const std::string&	GetError(void) { return m_strError; }

	void				SetLastID(double ID) { m_iLastID = ID; }
	double				GetLastID(void) { return m_iLastID; }

	void				SetAffected(double ID) { m_iAffected = ID; }
	double				GetAffected(void) { return m_iAffected; }

	void				SetResult(MYSQL_RES* result) { m_pResult = result; }
	MYSQL_RES*			GetResult() { return m_pResult; }

private:
	std::string			m_strError;
	int					m_iError;
	double				m_iLastID;
	double				m_iAffected;
	MYSQL_RES*			m_pResult;
};

typedef std::vector<Result*> Results;

class Query
{
public:
	Query(const char* query, int callback = -1, int callbackref = -1, bool usenumbers = false) :
		m_strQuery(query), m_iCallback(callback), m_iCallbackRef(callbackref), m_bUseNumbers(usenumbers)
	{
	}

	~Query(void)
	{
		for (Results::iterator it = m_pResults.begin(); it != m_pResults.end(); ++it)
			delete *it;
	}

	const std::string&	GetQuery(void) { return m_strQuery; }
	size_t				GetQueryLength(void) { return m_strQuery.length(); }

	int					GetCallback(void) { return m_iCallback; }
	int					GetCallbackRef(void) { return m_iCallbackRef; }

	bool				GetUseNumbers(void) { return m_bUseNumbers; }

	void				AddResult(Result* result) { m_pResults.push_back(result); }
	Results				GetResults(void) { return m_pResults; }

#ifdef ENABLE_QUERY_TIMERS
	double				GetQueryTime(void) { return m_queryTimer.GetElapsedSeconds(); }
#endif

private:

	std::string			m_strQuery;
	int					m_iCallback;
	int					m_iCallbackRef;
	bool				m_bUseNumbers;

	Results				m_pResults;

#ifdef ENABLE_QUERY_TIMERS
	Timer				m_queryTimer;
#endif

public:
	Query*				next;
};

class Database
{
public:
	Database(std::string host, std::string user, std::string pass, std::string db, unsigned int port, std::string socket, unsigned long flags, int callback);
	~Database(void);

	bool			Initialize(std::string& error);
	void			Shutdown(void);
	std::size_t		RunShutdownWork(void);
	void			Release(void);

	int				GetCallback() { return m_iCallback; }
	const char*		GetDatabase(void) { return m_strDB.c_str(); }
	bool			SetCharacterSet(const char* charset, std::string& error);
	char*			Escape(const char* query, unsigned int len);
	bool			Option(mysql_option option, const char* arg, std::string& error);
	const char*		GetServerInfo();
	const char*		GetHostInfo();
	int				GetServerVersion();
	void			QueueQuery(const char* query, int callback = -1, int callbackref = -1, bool usenumbers = false);

	int				GetTableIndex(void) { return m_iTableIndex; };

	bool			IsConnected(void) { return m_bIsConnected; };

	bool			IsPendingCallback() { return m_bIsPendingCallback; }
	void			MarkCallbackCalled() { m_bIsPendingCallback = false; }

	Query*			GetCompletedQueries();

private:
	bool		Connect(std::string& error);

	void		DoExecute(Query* query);
	void		PushCompleted(Query* query);

	void		MarkCallbackPending() { m_bIsPendingCallback = true; }

	MYSQL*		m_MySQL;

	waitfree_query_queue<Query> m_completedQueries;

	std::vector<std::thread> thread_group;
	asio::io_service io_service;
	std::auto_ptr<asio::io_service::work> work;

	bool				m_bIsConnected;
	bool				m_bIsPendingCallback;

	std::string			m_strHost;
	std::string			m_strUser;
	std::string			m_strPass;
	std::string			m_strDB;
	unsigned int		m_iPort;
	std::string			m_strSocket;
	unsigned long		m_iClientFlags;
	int					m_iCallback;

	int					m_iTableIndex;
};
