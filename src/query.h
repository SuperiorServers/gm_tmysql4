#ifndef TMYSQL4_QUERY
#define TMYSQL4_QUERY

#include "result.h"
#include "timer.h"

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
			delete* it;
	}

	Query* next;

	std::string&		GetQuery(void) { return m_strQuery; }
	void				AddResult(Result* result) { m_pResults.push_back(result); }
	void				TriggerCallback(lua_State* state);

private:

	std::string			m_strQuery;
	int					m_iCallback;
	int					m_iCallbackRef;
	bool				m_bUseNumbers;

	Results				m_pResults;

#ifdef ENABLE_QUERY_TIMERS
public:
	double				GetQueryTime(void) { return m_queryTimer.GetElapsedSeconds(); }
private:
	Timer				m_queryTimer;
#endif
};

#endif