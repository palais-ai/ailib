#include "Task.h"

BEGIN_NS_AILIB

TaskListener::~TaskListener()
{

}

Task::Task() :
    mListener(0),
    mRuntime(0),
    mStatus(StatusDormant)
{
    ;
}

Task::~Task()
{
    ;
}

void Task::setListener(TaskListener* listener)
{
    mListener = listener;
}

Status Task::getStatus() const
{
    return mStatus;
}

void Task::setStatus(Status status)
{
    Status before = mStatus;
    if(status != before)
    {
        mStatus = status;

        if(mListener)
        {
            mListener->onStatusChanged(this, before);
        }
    }
}

void Task::addRuntime(const HighResolutionTime::Timestamp& runtime)
{
    // TODO: We could improve this by changing the runtime scale towards longer computation times.

    // The runtime counter overflows after 2^16 microseconds (which is approx. 65 ms).
    // This is fine for our purposes, since the 65 ms represent actual computation time.
    // The only penalty for overflowing is that this task may get execution priority over other
    // tasks. We accept this penalty in order to save (up to) 6 bytes per task over the maximum
    // resolution (uint64_t).
    // Note that most longer running tasks will be blocking (eg. waiting for other systems' inputs).
    // This wait time is not counted towards runtime, which further alleviates this issue.
    mRuntime += runtime;
}

void Task::resetRuntime()
{
    mRuntime = 0;
}

HighResolutionTime::Timestamp Task::getRuntime() const
{
    return mRuntime;
}

END_NS_AILIB
