#include "gm_tmysql.h"

Database::Database(const char* host, const char* user, const char* pass, const char* db, int port, const char* socket, int flags) :
m_strHost(host), m_strUser(user), m_strPass(pass), m_strDB(db), m_iPort(port), m_strSocket(socket), m_iClientFlags(flags), m_pEscapeConnection(NULL)
{
	work.reset(new asio::io_service::work(io_service));
}

Database::~Database( void )
{
}

bool Database::Initialize(std::string& error)
{
	static my_bool bTrue = true;

	for (int i = NUM_CON_DEFAULT+1; i; i--)
	{
		MYSQL* mysql = mysql_init(NULL);

		if (!Connect(mysql, error))
			return false;

		if (i > NUM_CON_DEFAULT)
			m_pEscapeConnection = mysql;
		else
			m_vecAvailableConnections.push_back(mysql);
	}

	for (unsigned int i = 0; i < NUM_THREADS_DEFAULT; ++i)
	{
		thread_group.push_back(std::thread(
			[&]()
		{
			io_service.run();
		}));
	}

	return true;
}

bool Database::Connect(MYSQL* mysql, std::string& error)
{
	if (!mysql_real_connect(mysql, m_strHost, m_strUser, m_strPass, m_strDB, m_iPort, m_strSocket, m_iClientFlags))
	{
		error.assign(mysql_error(mysql));
		return false;
	}

	return true;
}
void Database::Shutdown(void)
{
	work.reset();

	for (auto iter = thread_group.begin(); iter != thread_group.end(); ++iter)
	{
		iter->join();
	}

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

	for (auto iter = m_vecAvailableConnections.begin(); iter != m_vecAvailableConnections.end(); ++iter)
	{
		mysql_close(*iter);
	}

	m_vecAvailableConnections.clear();

	if (m_pEscapeConnection != NULL)
	{
		mysql_close(m_pEscapeConnection);
		m_pEscapeConnection = NULL;
	}
}

char* Database::Escape(const char* query)
{
	size_t len = strlen(query);
	char* escaped = new char[len * 2 + 1];

	mysql_real_escape_string(m_pEscapeConnection, escaped, query, len);
	return escaped;
}

bool Database::SetCharacterSet(const char* charset, std::string& error)
{
	if (mysql_set_character_set(m_pEscapeConnection, charset) > 0)
	{
		error.assign(mysql_error(m_pEscapeConnection));
		return false;
	}

	for (auto iter = m_vecAvailableConnections.begin(); iter != m_vecAvailableConnections.end(); ++iter)
	{
		if (mysql_set_character_set(*iter, charset) > 0)
		{
			error.assign(mysql_error(*iter));
			return false;
		}
	}

	return true;
}

void Database::QueueQuery(const char* query, int callback, int callbackref, bool usenumbers)
{
	Query* newquery = new Query(query, callback, callbackref, usenumbers);
	QueueQuery(newquery);
}

void Database::QueueQuery(Query* query)
{
	io_service.post(std::bind(&Database::DoExecute, this, query));
}

Query* Database::GetCompletedQueries()
{
	return m_completedQueries.pop_all();
}

void Database::PushCompleted(Query* query)
{
	m_completedQueries.push(query);
}

void Database::DoExecute(Query* query)
{
	MYSQL* pMYSQL = GetAvailableConnection();

	const char* strquery = query->GetQuery().c_str();
	size_t len = query->GetQueryLength();

	int err = mysql_ping(pMYSQL);

	if (err > 0)
	{
		// Attempt reconnect if connection is lost, once chance, one opportunity per query call
        std::lock_guard<std::recursive_mutex> guard(m_AvailableMutex);
		
		mysql_close( pMYSQL );
		pMYSQL = mysql_init( NULL );

		mysql_real_connect(pMYSQL, m_strHost, m_strUser, m_strPass, m_strDB, m_iPort, m_strSocket, m_iClientFlags);
	}

	mysql_real_query(pMYSQL, strquery, len);

	int status;
	do {
		MYSQL_RES* pResult = mysql_store_result(pMYSQL);
		unsigned int errorno = mysql_errno(pMYSQL);

		Result* result = new Result();
		{
			result->SetResult(pResult);
			result->SetErrorID(errorno);
			result->SetError(mysql_error(pMYSQL));
			result->SetAffected((double)mysql_affected_rows(pMYSQL));
			result->SetLastID((double)mysql_insert_id(pMYSQL));
		}
		query->AddResult(result);
		status = mysql_next_result(pMYSQL);
	} while (status != -1);

	PushCompleted(query);
	ReturnConnection(pMYSQL);
}