#ifdef ENABLE_QUERY_TIMERS

#ifndef TMYSQL4_TIMER
#define TMYSQL4_TIMER

#ifdef _WIN32
	#include <windows.h>
#else
	#include <time.h>
	#include <stdint.h>
	#define NSEC_PER_SEC (1000000000L)
	typedef int64_t LARGE_INTEGER;
#endif

class Timer
{
public:
	Timer();
	double GetElapsedSeconds();

private:
	LARGE_INTEGER frequency;
	LARGE_INTEGER startTick;
};

#endif

#endif