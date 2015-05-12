#ifndef SCHEDULER_H
#define SCHEDULER_H

#pragma once

#include "ai_global.h"
#include "Task.h"
#include <set>

BEGIN_NS_AILIB

class TaskComparator
{
public:
    FORCE_INLINE bool operator() (const Task* lv, const Task* rv) const
    {
        if(lv->getRuntime() == rv->getRuntime())
        {
            return lv > rv;
        }

        return lv->getRuntime() < rv->getRuntime();
    }
};

class SchedulerListener
{
public:
    virtual ~SchedulerListener() {}
    virtual void   onTaskAdded(Task* task)
    {
        UNUSED(task)
    }

    virtual void onTaskRemoved(Task* task)
    {
        UNUSED(task)
    }

    virtual void onBeginRunTask(Task* task)
    {
        UNUSED(task)
    }
};

// TODO: Add a SchedulingPolicy template parameter.
class Scheduler : private TaskListener
{
public:
    typedef std::set<Task*, TaskComparator> TaskList;

    Scheduler();

    void setListener(SchedulerListener* listener);
    void clear();
    void enqueue(Task* task);
    void dequeue(Task* task);

    // @returns Number of microseconds spent computing during this call.
    HighResolutionTime::Timestamp update(HighResolutionTime::Timestamp maxRuntime, float dt);
    virtual void onStatusChanged(Task* task, Status from);
private:
    void removeWaiting(Task* task);
    void removeRunning(Task* task);
    void removeFrom(TaskList& list, Task* task);

    TaskList mTasks;
    TaskList mWaiting;
    SchedulerListener* mListener;
};

END_NS_AILIB

#endif // SCHEDULER_H
