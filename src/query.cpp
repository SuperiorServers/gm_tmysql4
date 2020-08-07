#include "main.h"
#include "query.h"

using namespace GarrysMod::Lua;

void Query::TriggerCallback(lua_State* state)
{
	if (m_iCallback < 0)
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
	if (m_iCallbackRef >= 0)
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
			LUA->PushNumber(GetQueryTime());
			LUA->SetField(-2, "time");
#endif
		}

		LUA->SetTable(-3);
	}

	// For some reason PCall crashes during shutdown??
	if (LUA->PCall(args, 0, 0) != 0)
	{
		LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		LUA->GetField(-1, "ErrorNoHalt"); // could cache this function... but this really should not be called in the first place
		LUA->Push(-3); // This is the error message from PCall
		LUA->PushString("\n"); // add a newline since ErrorNoHalt does not do that itself
		LUA->Call(2, 0);
		LUA->Pop(2); // Pop twice since the PCall error is still on the stack
	}
}