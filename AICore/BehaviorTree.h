#ifndef BEHAVIORTREE_H
#define BEHAVIORTREE_H

#pragma once

#include "ai_global.h"
#include "Scheduler.h"
#include "Any.h"
#include <vector>
#include <cstdlib>

BEGIN_NS_AILIB

class Behavior;

class BehaviorListener
{
public:
    virtual ~BehaviorListener();
    virtual void onSuccess(Behavior* behavior) = 0;
    virtual void onFailure(Behavior* behavior) = 0;
    virtual void onReset(Behavior* behavior);
};

class Behavior : public Task
{
public:
    Behavior();
    virtual ~Behavior();

    void setListener(BehaviorListener* listener);

    // __terminate__ should be called by parent nodes that
    // forcefully remove this behavior from the scheduler.
    virtual void terminate();

    void notifySuccess();
    void notifyFailure();
    void notifyReset();

    // Virtual to allow subclasses to decide whether or not the user data should be passed on
    // to child behaviors.
    virtual void setUserData(const hold_any& data);
    hold_any& getUserData();
private:
    BehaviorListener* mListener;
    hold_any mUserData;
};

class Composite : public Behavior, public BehaviorListener
{
public:
    typedef std::vector<Behavior*> BehaviorList;

    Composite(Scheduler& scheduler, const BehaviorList& children);
    virtual ~Composite();

    const BehaviorList& getChildren() const;
    BehaviorList& getChildren();

    // Cascades the user data to all children.
    virtual void setUserData(const hold_any& data);
private:
    BehaviorList mChildren;
protected:
    uint32_t indexOf(const Behavior* child) const;

    Scheduler& mScheduler;
};

class SequentialComposite : public Composite
{
public:
    SequentialComposite(Scheduler& scheduler,
                        const Composite::BehaviorList& children);

    virtual ~SequentialComposite();

    virtual void run();
    virtual void terminate();
    virtual void onReset(Behavior* behavior);
protected:
    bool indexIsCurrent(uint32_t idx) const;
    bool currentIsLastBehavior() const;
    void scheduleNextBehavior();
    void terminateFromIndex(uint32_t idx);
private:
    uint16_t mCurrentBehavior;
};

class Selector : public SequentialComposite
{
public:
    Selector(Scheduler& scheduler,
             const Composite::BehaviorList& children);
    virtual ~Selector();

    virtual void onSuccess(Behavior* behavior);
    virtual void onFailure(Behavior* behavior);
};

class Sequence : public SequentialComposite
{
public:
    Sequence(Scheduler& scheduler,
             const Composite::BehaviorList& children);
    virtual ~Sequence();

    virtual void onSuccess(Behavior* behavior);
    virtual void onFailure(Behavior* behavior);
};

class Parallel : public Composite
{
public:
    Parallel(Scheduler& scheduler,
             const Composite::BehaviorList& children);
    virtual ~Parallel();

    virtual void run();
    virtual void terminate();
    virtual void onSuccess(Behavior* behavior);
    virtual void onFailure(Behavior* behavior);
    virtual void onReset(Behavior* behavior);
private:
    void resetCodes();
    bool allChildrenSucceeded() const;
    bool anyChildrenFailed() const;

    enum ReturnCode
    {
        ReturnCodeNone = 0,
        ReturnCodeSuccess,
        ReturnCodeFailure
    };
    static const int32_t sMaxChildCount = 8;
    ReturnCode mCodes[sMaxChildCount];
};

class Decorator : public Behavior, public BehaviorListener
{
public:
    Decorator(Scheduler& scheduler, Behavior* child);
    virtual ~Decorator();

    virtual void terminate();
    virtual void onSuccess(Behavior* behavior);
    virtual void onFailure(Behavior* behavior);
    virtual void onReset(Behavior* behavior);
    virtual void setUserData(const hold_any &data);

    const Behavior* getChild() const;
protected:
    void scheduleBehavior();
    void terminateChild();
private:
    Scheduler& mScheduler;
    Behavior* const mChild;
};

// Requires a random access iterator.
template <typename ITER>
void shuffle(ITER start, ITER end)
{
    AI_ASSERT(end - start <= std::numeric_limits<int32_t>::max(),
              "Too many children - integer overflow.");

    // Fisher-Yates shuffle, used by the random composites below.
    for(int32_t i = end - start - 1; i > 0; --i)
    {
        // The rand() use can be made deterministic by calling srand() with the same seed on startup.
        const uint32_t j = rand() % i;
        typedef typename std::iterator_traits<ITER>::value_type ValueType;
        ValueType tmp = *(start + i);
        *(start + i) = *(start + j);
        *(start + j) = tmp;
    }
}

Composite::BehaviorList make_shuffled(const Composite::BehaviorList& children);

template <typename T>
class RandomComposite : public T
{
public:
    RandomComposite(Scheduler& scheduler,
                    const Composite::BehaviorList& children) :
        T(scheduler, make_shuffled(children))
    {
        ;
    }

    virtual ~RandomComposite() {}

    virtual void terminate()
    {
        T::terminate();
        // Re-shuffle on termination. (After terminating the children)
        shuffle(Composite::getChildren().begin(), Composite::getChildren().end());
    }
};

class RandomSelector : public RandomComposite<Selector>
{
public:
    RandomSelector(Scheduler& scheduler,
                   const Composite::BehaviorList& children);
    virtual ~RandomSelector();
};

class RandomSequence : public RandomComposite<Sequence>
{
public:
    RandomSequence(Scheduler& scheduler,
                   const Composite::BehaviorList& children);
    virtual ~RandomSequence();
};

END_NS_AILIB

#endif // BEHAVIORTREE_H
