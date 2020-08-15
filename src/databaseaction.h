#ifndef TMYSQL4_DBACTION
#define TMYSQL4_DBACTION

#include "result.h"
#include "timer.h"

class DatabaseAction
{
public:
	DatabaseAction(int callback = -1, int callbackref = -1, bool usenumbers = false) :
		m_iCallback(callback), m_iCallbackRef(callbackref), m_bUseNumbers(usenumbers)
	{
	}
	~DatabaseAction(void)
	{
		for (Results::iterator it = m_pResults.begin(); it != m_pResults.end(); ++it)
			delete* it;
	}

	DatabaseAction* next;

	void				AddResult(Result* result) { m_pResults.push_back(result); }
	void				TriggerCallback(lua_State* state);

private:

	int					m_iCallback;
	int					m_iCallbackRef;
	bool				m_bUseNumbers;

	Results				m_pResults;

#ifdef ENABLE_QUERY_TIMERS
public:
	Timer				GetTimer() { return m_queryTimer; }
private:
	Timer				m_queryTimer;
#endif
};

#endif