#include "main.h"
#include "databaseaction.h"

using namespace GarrysMod::Lua;

DatabaseAction::DatabaseAction(int callback = -1, int callbackref = -1, bool usenumbers = false) :
	m_iCallback(callback), m_iCallbackRef(callbackref), m_bUseNumbers(usenumbers)
{
#ifdef ENABLE_QUERY_TIMERS
	m_queryTimer = new Timer();
#endif
}
DatabaseAction::~DatabaseAction(void)
{
	for (Results::iterator it = m_pResults.begin(); it != m_pResults.end(); ++it)
		delete* it;
#ifdef ENABLE_QUERY_TIMERS
	delete m_queryTimer;
#endif
}

void DatabaseAction::TriggerCallback(lua_State* state)
{
	if (m_iCallback <= 0)
		return;

	LUA->ReferencePush(m_iCallback);
	LUA->ReferenceFree(m_iCallback);

	if (!LUA->IsType(-1, Type::Function))
	{
		LUA->Pop();
		LUA->ReferenceFree(m_iCallbackRef);
		return;
	}

	int args = 1;
	if (m_iCallbackRef > 0)
	{
		args = 2;
		LUA->ReferencePush(m_iCallbackRef);
		LUA->ReferenceFree(m_iCallbackRef);
	}

	LUA->CreateTable();

	int resultid = 1;
	for (Results::iterator it = m_pResults.begin(); it != m_pResults.end(); ++it) {
		Result* result = *it;

		LUA->PushNumber(resultid++);

		LUA->CreateTable();
		{
			result->PopulateLuaTable(state, m_bUseNumbers);

#ifdef ENABLE_QUERY_TIMERS
			LUA->PushNumber(GetTimer()->GetElapsedSeconds());
			LUA->SetField(-2, "time");
#endif
		}

		LUA->SetTable(-3);
	}

	// For some reason PCall crashes during shutdown??
	if (LUA->PCall(args, 0, 0) != 0)
	{
		LUA->GetField(LUA_GLOBAL, "ErrorNoHalt");
		LUA->Push(-3);
		LUA->PushString("\n");
		LUA->Call(2, 0);
		LUA->Pop();
	}
}