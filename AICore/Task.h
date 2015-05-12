#ifndef TASK_H
#define TASK_H

#pragma once

#include "ai_global.h"
#include "HighResolutionTime.h"

BEGIN_NS_AILIB

class Task;

enum Status
{
    StatusDormant = 0,
    StatusRunning,
    StatusWaiting,
    StatusTerminated
};

class TaskListener
{
public:
    virtual ~TaskListener();
    virtual void onStatusChanged(Task* task, Status from) = 0;
};

class Task
{
public:
    Task();
    virtual ~Task();

    virtual void run() = 0;

    void setListener(TaskListener* listener);
    Status getStatus() const;
    void setStatus(Status status);
    void addRuntime(const HighResolutionTime::Timestamp& runtime);
    void resetRuntime();
    HighResolutionTime::Timestamp getRuntime() const;
private:
    TaskListener* mListener;
    uint16_t mRuntime; //< in microseconds
    Status mStatus;
};

END_NS_AILIB

#endif // TASK_H
