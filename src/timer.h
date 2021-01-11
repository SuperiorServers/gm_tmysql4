#pragma once

#ifdef ENABLE_QUERY_TIMERS

#include <chrono>

#define StartQueryTimer(action) action->GetTimer()->Start()
#define EndQueryTimer(action) action->GetTimer()->Stop()
#define CreateQueryTimer() m_queryTimer = new Timer();
#define DeleteQueryTimer() delete m_queryTimer;

#define AddQueryTimerMembers() \
	public: \
		Timer* GetTimer() { return m_queryTimer; } \
	private: \
		Timer* m_queryTimer; \

#define PushQueryExecutionTime() \
	LUA->PushNumber(GetTimer()->GetElapsedSeconds()); \
	LUA->SetField(-2, "time") \

class Timer
{
public:
	double	GetElapsedSeconds() { return elapsedNano(endTime - startTime).count() / 1e9; }
	void	Start() { startTime = std::chrono::steady_clock::now(); }
	void	Stop() { endTime = std::chrono::steady_clock::now(); }

private:
	std::chrono::steady_clock::time_point startTime;
	std::chrono::steady_clock::time_point endTime;

	typedef std::chrono::duration<double, std::nano> elapsedNano;
};

#else

#define StartQueryTimer(action) 
#define EndQueryTimer(action)
#define CreateQueryTimer()
#define DeleteQueryTimer()
#define AddQueryTimerMembers()
#define PushQueryExecutionTime()

#endif