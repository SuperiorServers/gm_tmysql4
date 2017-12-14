#include "gm_tmysql.h"
#include <errmsg.h>

using namespace GarrysMod::Lua;

unsigned int database_index = 1;

Database::Database(std::string host, std::string user, std::string pass, std::string db, unsigned int port, std::string socket, unsigned long flags, int callback)
	: m_strHost(host), m_strUser(user), m_strPass(pass), m_strDB(db), m_iPort(port), m_strSocket(socket), m_iClientFlags(flags), m_iCallback(callback), m_bIsConnected(false), m_MySQL(NULL), m_iTableIndex(database_index++)
{
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
	const char* socket = (m_strSocket.length() == 0) ? nullptr : m_strSocket.c_str();
	unsigned int flags = m_iClientFlags | CLIENT_MULTI_RESULTS;

	my_bool tru = 1;
	if (mysql_options(m_MySQL, MYSQL_OPT_RECONNECT, &tru) > 0)
	{
		error.assign(mysql_error(m_MySQL));
		return false;
	}

	if (mysql_real_connect(m_MySQL, m_strHost.c_str(), m_strUser.c_str(), m_strPass.c_str(), m_strDB.c_str(), m_iPort, socket, flags) != m_MySQL)
	{
		error.assign(mysql_error(m_MySQL));
		return false;
	}

	MarkCallbackPending();

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
	io_service.post(std::bind(&Database::DoExecute, this, newquery));
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
	const char* strquery = query->GetQuery().c_str();
	size_t len = query->GetQueryLength();

	bool hasRetried = false;

	retry:

	unsigned int errorno = NULL;
	if (mysql_real_query(m_MySQL, strquery, len) != 0) {

		errorno = mysql_errno(m_MySQL);

		if (!hasRetried && ((errorno == CR_SERVER_LOST) || (errorno == CR_SERVER_GONE_ERROR) || (errorno != CR_CONN_HOST_ERROR) || (errorno == 1053)/*ER_SERVER_SHUTDOWN*/ || (errorno != CR_CONNECTION_ERROR))) {
			hasRetried = true;
			goto retry;
		}
	}
	
	int status = 0;
	while (status != -1) {
		MYSQL_RES* pResult = mysql_store_result(m_MySQL);

		Result* result = new Result();
		{
			result->SetResult(pResult);
			result->SetErrorID(errorno);
			result->SetError(mysql_error(m_MySQL));
			result->SetAffected((double)mysql_affected_rows(m_MySQL));
			result->SetLastID((double)mysql_insert_id(m_MySQL));
		}
		query->AddResult(result);
		status = mysql_next_result(m_MySQL);
	}

	PushCompleted(query);
}