#ifdef ENABLE_QUERY_TIMERS

#include "timer.h"

using namespace std::chrono;

Timer::~Timer()
{
}

void Timer::Start()
{
	startTime = steady_clock::now();
	return;
}

void Timer::Stop()
{
	endTime = steady_clock::now();
	return;
}

double Timer::GetElapsedSeconds()
{
	duration<double, std::nano> elapsed = endTime - startTime;
	return elapsed.count() / 1e9;
}

#endif