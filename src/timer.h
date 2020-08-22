#ifdef ENABLE_QUERY_TIMERS

#ifndef TMYSQL4_TIMER
#define TMYSQL4_TIMER

#include <chrono>

class Timer
{
public:
	~Timer();
	double	GetElapsedSeconds();
	void	Start();
	void	Stop();

private:
	std::chrono::steady_clock::time_point startTime;
	std::chrono::steady_clock::time_point endTime;
};

#endif

#endif