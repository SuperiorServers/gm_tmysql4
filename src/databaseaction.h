#ifndef TMYSQL4_DBACTION
#define TMYSQL4_DBACTION

#include "result.h"
#include "timer.h"

class DatabaseAction
{
public:
	DatabaseAction(int callback, int callbackref, bool usenumbers);
	~DatabaseAction(void);

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
	Timer*				GetTimer() { return m_queryTimer; }
private:
	Timer*				m_queryTimer;
#endif
};

#endif