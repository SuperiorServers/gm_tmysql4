#include "main.h"
#include "tmysql.h"
#include "databaseaction.h"

using namespace GarrysMod::Lua;

DatabaseAction::DatabaseAction(int callback = -1, int callbackref = -1, bool usenumbers = false) :
	m_iCallback(callback), m_iCallbackRef(callbackref), m_bUseNumbers(usenumbers)
{
	CreateQueryTimer();
}
DatabaseAction::~DatabaseAction(void)
{
	for (auto it : m_pResults)
		delete it;

	DeleteQueryTimer();
}

void DatabaseAction::TriggerCallback(lua_State* state)
{
	if (m_iCallback <= 0)
		return;

	LUA->ReferencePush(tmysql::iRefDebugTraceBack);

	LUA->ReferencePush(m_iCallback);
	LUA->ReferenceFree(m_iCallback);

	if (!LUA->IsType(-1, Type::Function))
	{
		LUA->Pop(2);

		if (m_iCallbackRef > 0)
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
	for (auto result : m_pResults)
	{
		LUA->PushNumber(resultid++);

		LUA->CreateTable();
		{
			result->PopulateLuaTable(state, m_bUseNumbers);

			PushQueryExecutionTime();
		}

		LUA->SetTable(-3);
	}

	// For some reason PCall crashes during shutdown??
	if (LUA->PCall(args, 0, -args - 2) != 0)
		ErrorNoHalt(std::string("[tmysql callback error]\n").append(LUA->GetString(-1))),
		LUA->Pop(1);
}