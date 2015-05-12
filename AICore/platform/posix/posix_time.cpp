#include "HighResolutionTime.h"
#include <sys/time.h>
#include <time.h>

BEGIN_NS_AILIB

static LARGE_INTEGER frequency = 0;
QueryPerformanceFrequency(&frequency);

namespace HighResolutionTime {
    Timestamp now()
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t) ts.tv_sec * (uint64_t) 1000000000 + (uint64_t) ts.tv_nsec;
    }
}

END_NS_AILIB
