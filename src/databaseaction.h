#pragma once

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

	AddQueryTimerMembers();
};