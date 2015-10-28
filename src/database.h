#include <deque>
#include <atomic>
#include <mutex>
#include <thread>
#include <boost/asio.hpp>
#include "timer.h"

using namespace boost;

#define NUM_THREADS_DEFAULT 2
#define NUM_CON_DEFAULT NUM_THREADS_DEFAULT

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

	double				GetQueryTime(void) { return m_queryTimer.GetElapsedSeconds(); }

private:

	std::string			m_strQuery;
	int					m_iCallback;
	int					m_iCallbackRef;
	bool				m_bUseNumbers;

	Results				m_pResults;

	Timer				m_queryTimer;

public:
	Query*				next;
};

class Database
{
public:
	Database(const char* host, const char* user, const char* pass, const char* db, int port, const char* socket, int flags);
	~Database(void);

	bool			Initialize(std::string& error);
	void			Shutdown(void);
	std::size_t		RunShutdownWork(void);
	void			Release(void);

	const char*		GetDatabase(void) { return m_strDB; }
	bool			SetCharacterSet(const char* charset, std::string& error);
	char*			Escape(const char* query, unsigned int len);
	bool			Option(mysql_option option, const char* arg, std::string& error);
	const char*		GetServerInfo();
	const char*		GetHostInfo();
	int				GetServerVersion();
	void			QueueQuery(const char* query, int callback = -1, int callbackref = -1, bool usenumbers = false);

	int				GetTableIndex(void) { return m_iTableIndex; };

	Query*			GetCompletedQueries();

private:
	bool		Connect(MYSQL* mysql, std::string& error);

	void		QueueQuery(Query* query);

	void		DoExecute(Query* query);
	void		PushCompleted(Query* query);

	MYSQL* GetAvailableConnection()
	{
		std::lock_guard<std::recursive_mutex> guard(m_Mutex);
		MYSQL* result = m_vecAvailableConnections.front();
		m_vecAvailableConnections.pop_front();
		return result;
	}

	void ReturnConnection(MYSQL* mysql)
	{
		std::lock_guard<std::recursive_mutex> guard(m_Mutex);
		m_vecAvailableConnections.push_back(mysql);
	}

	MYSQL*	m_pEscapeConnection;

	waitfree_query_queue<Query> m_completedQueries;
	std::deque< MYSQL* > m_vecAvailableConnections;

	mutable std::recursive_mutex m_Mutex;

	std::vector<std::thread> thread_group;
	asio::io_service io_service;
	std::auto_ptr<asio::io_service::work> work;

	const char*			m_strHost;
	const char*			m_strUser;
	const char*			m_strPass;
	const char*			m_strDB;
	int					m_iPort;
	const char*			m_strSocket;
	int					m_iClientFlags;

	int					m_iTableIndex;
};
