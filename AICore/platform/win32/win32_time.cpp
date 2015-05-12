#include "../../HighResolutionTime.h"
#include <windows.h>

BEGIN_NS_AILIB

namespace HighResolutionTime {
    static LARGE_INTEGER initQPC()
    {
        LARGE_INTEGER retVal;
        QueryPerformanceFrequency(&retVal);
        return retVal;
    }

    static LARGE_INTEGER sFrequency = initQPC();

    Timestamp now()
    {
        LARGE_INTEGER retVal;
        QueryPerformanceCounter(&retVal);
        retVal.QuadPart *= 1000000000; // multiply first to prevent precision-loss
        retVal.QuadPart /= sFrequency.QuadPart;
        return retVal.QuadPart;
    }
}

END_NS_AILIB
