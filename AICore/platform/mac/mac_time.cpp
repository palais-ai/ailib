#include "HighResolutionTime.h"
#include <mach/mach_time.h>

BEGIN_NS_AILIB

namespace HighResolutionTime {

    static mach_timebase_info_data_t initInfo()
    {
        mach_timebase_info_data_t info;
        mach_timebase_info(&info);
        return info;
    }

    static mach_timebase_info_data_t sInfo = initInfo();

    Timestamp now()
    {
        return mach_absolute_time() * sInfo.numer / sInfo.denom;
    }
}

END_NS_AILIB
