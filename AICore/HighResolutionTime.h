#ifndef HIGHRESOLUTIONTIME_H
#define HIGHRESOLUTIONTIME_H

#pragma once

#include "ai_global.h"
#include <stdint.h>

BEGIN_NS_AILIB

namespace HighResolutionTime {
    typedef int64_t Timestamp; // in microseconds

    double milliseconds(Timestamp t);
    double seconds(Timestamp t);
    Timestamp milliseconds(double t);
    Timestamp seconds(double t);
    Timestamp now();
}

END_NS_AILIB

#endif // HIGHRESOLUTIONTIME_H
