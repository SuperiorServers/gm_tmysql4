#ifdef ENABLE_TIMER

#include "timer.h"

#ifndef _WIN32
	void QueryPerformanceCounter(LARGE_INTEGER* count)
	{
		LARGE_INTEGER nsec_count, nsec_per_tick;
		struct timespec ts1, ts2;

		if (clock_gettime(CLOCK_MONOTONIC, &ts1) != 0) {
			return;
		}

		nsec_count = ts1.tv_nsec + ts1.tv_sec * NSEC_PER_SEC;

		if (clock_getres(CLOCK_MONOTONIC, &ts2) != 0) {
			return;
		}

		nsec_per_tick = ts2.tv_nsec + ts2.tv_sec * NSEC_PER_SEC;

		*count = (LARGE_INTEGER) (nsec_count / nsec_per_tick);
	}
#endif

Timer::Timer()
{
#ifdef _WIN32
	QueryPerformanceFrequency(&frequency);
#endif
	QueryPerformanceCounter(&startTick);
}

double Timer::GetElapsedSeconds()
{
	LARGE_INTEGER currentTick;
	QueryPerformanceCounter(&currentTick);
#ifdef _WIN32
	return (double)(currentTick.QuadPart - startTick.QuadPart) / frequency.QuadPart;
#else
	return (double)(currentTick - startTick) / NSEC_PER_SEC;
#endif
}

#endif