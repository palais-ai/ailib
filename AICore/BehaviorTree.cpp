#include "BehaviorTree.h"
#include <stdint.h>
#include <cstring>
#include <limits>
#include <algorithm>

BEGIN_NS_AILIB

BehaviorListener::~BehaviorListener()
{
    ;
}

void BehaviorListener::onReset(Behavior* behavior)
{
    UNUSED(behavior);
}

Behavior::Behavior() :
    mListener(NULL)
{
    ;
}

Behavior::~Behavior()
{
    ;
}

void Behavior::setListener(BehaviorListener* listener)
{
    mListener = listener;
}

void Behavior::terminate()
{
    setStatus(StatusTerminated);
}

void Behavior::notifySuccess()
{
    if(mListener)
    {
        mListener->onSuccess(this);
    }
    setStatus(StatusDormant);
}

void Behavior::notifyFailure()
{
    if(mListener)
    {
        mListener->onFailure(this);
    }
    setStatus(StatusDormant);
}

void Behavior::notifyReset()
{
    if(mListener)
    {
        mListener->onReset(this);
    }
}

void Behavior::setUserData(const hold_any& data)
{
    mUserData = data;
}

hold_any& Behavior::getUserData()
{
    return mUserData;
}

Composite::Composite(Scheduler& scheduler,
                     const Composite::BehaviorList& children) :
    mChildren(children),
    mScheduler(scheduler)
{
    for(Composite::BehaviorList::iterator it = mChildren.begin();
        it != mChildren.end(); ++it)
    {
        // Composites listen to their children's result codes.
        (*it)->setListener(this);
    }
}

Composite::~Composite()
{
    for(Composite::BehaviorList::iterator it = mChildren.begin();
        it != mChildren.end(); ++it)
    {
        // Reset children's listeners.
        (*it)->setListener(NULL);
    }
}

const Composite::BehaviorList& Composite::getChildren() const
{
    return mChildren;
}

Composite::BehaviorList& Composite::getChildren()
{
    return mChildren;
}

void Composite::setUserData(const hold_any& data)
{
    Behavior::setUserData(data);
    for(Composite::BehaviorList::iterator it = mChildren.begin();
        it != mChildren.end(); ++it)
    {
        // Pass on the data to all child nodes.
        (*it)->setUserData(getUserData());
    }
}

uint32_t Composite::indexOf(const Behavior* child) const
{
    BehaviorList::const_iterator it = std::find(getChildren().begin(),
                                                getChildren().end(),
                                                child);
    AI_ASSERT(it != getChildren().end(),
              "Tried to obtain index of inexistant child.");
    return it - getChildren().begin();
}

SequentialComposite::SequentialComposite(Scheduler& scheduler,
                                         const Composite::BehaviorList& children) :
    Composite(scheduler, children),
    mCurrentBehavior(0)
{
    ;
}

SequentialComposite::~SequentialComposite()
{
    ;
}

void SequentialComposite::run()
{
    // The composite has no children.
    if(getChildren().empty())
    {
        notifySuccess();
    }
    // Start by scheduling the first child for execution.
    else
    {
        mScheduler.enqueue(getChildren()[mCurrentBehavior]);
        setStatus(StatusWaiting);
    }
}

void SequentialComposite::terminate()
{
    terminateFromIndex(0);
    Behavior::terminate();
}

void SequentialComposite::onReset(Behavior* behavior)
{
    const uint32_t idx = indexOf(behavior);
    AI_ASSERT(idx <= mCurrentBehavior,
              "A behavior running after the current behavior should not be reseting this node.");

    // Terminate all behaviors following this one.
    terminateFromIndex(idx+1);
    notifyReset();
}

bool SequentialComposite::indexIsCurrent(uint32_t idx) const
{
    return idx == mCurrentBehavior;
}

bool SequentialComposite::currentIsLastBehavior() const
{
    AI_ASSERT(getChildren().size() <= static_cast<size_t>(std::numeric_limits<int32_t>::max()),
              "This node parents too many children for the counter variable - integer overflow.");
    return mCurrentBehavior + 1 == static_cast<int32_t>(getChildren().size());
}

void SequentialComposite::scheduleNextBehavior()
{
    mScheduler.enqueue(getChildren()[++mCurrentBehavior]);
}

void SequentialComposite::terminateFromIndex(uint32_t idx)
{
    if(idx < getChildren().size())
    {
        // Terminate all behaviors following (including) __idx__.
        for(uint32_t i = idx; i <= mCurrentBehavior; ++i)
        {
            getChildren()[i]->terminate();
        }

        mCurrentBehavior = idx == 0 ? 0 : idx - 1;
    }
}

Selector::Selector(Scheduler& scheduler,
                   const Composite::BehaviorList& children) :
    SequentialComposite(scheduler, children)
{
    ;
}

Selector::~Selector()
{
    ;
}

void Selector::onSuccess(Behavior* behavior)
{
    const uint32_t idx = indexOf(behavior);
    if(!indexIsCurrent(idx))
    {
        terminateFromIndex(idx+1);
    }
    notifySuccess();
}

void Selector::onFailure(Behavior* behavior)
{
    const uint32_t idx = indexOf(behavior);
    if(!indexIsCurrent(idx))
    {
        terminateFromIndex(idx+1);
    }

    // The successful behavior was the last one in the sequence.
    if(currentIsLastBehavior())
    {
        notifyFailure();
    }
    // Schedule the next behavior in the sequence.
    else
    {
        scheduleNextBehavior();
    }
}

Sequence::Sequence(Scheduler& scheduler,
                   const Composite::BehaviorList& children) :
    SequentialComposite(scheduler, children)
{
    ;
}

Sequence::~Sequence()
{
    ;
}

void Sequence::onSuccess(Behavior* behavior)
{
    const uint32_t idx = indexOf(behavior);
    if(!indexIsCurrent(idx))
    {
        terminateFromIndex(idx+1);
    }

    if(currentIsLastBehavior())
    {
        notifySuccess();
    }
    else
    {
        scheduleNextBehavior();
    }
}

void Sequence::onFailure(Behavior* behavior)
{
    const uint32_t idx = indexOf(behavior);
    if(!indexIsCurrent(idx))
    {
        terminateFromIndex(idx+1);
    }
    notifyFailure();
}

Parallel::Parallel(Scheduler& scheduler,
                   const Composite::BehaviorList& children) :
    Composite(scheduler, children)
{
    // The return code state array limits the maximum number of children for parallel tasks.
    AI_ASSERT(children.size() <= sMaxChildCount,
              "A parallel node can't run more than __sMaxChildCount__ parallel tasks.");
    resetCodes();
}

Parallel::~Parallel()
{
    ;
}

void Parallel::run()
{
    if(getChildren().empty())
    {
        notifySuccess();
        return;
    }

    // Schedule all the child behaviors at once.
    // Reversed to keep the execution sequence left-to-right using our sequential scheduler.
    for(std::vector<Behavior*>::const_reverse_iterator it = getChildren().rbegin();
        it != getChildren().rend(); ++it)
    {
        mScheduler.enqueue(*it);
    }

    setStatus(StatusWaiting);
}

void Parallel::terminate()
{
    resetCodes();

    // Terminate all child behaviors.
    for(std::vector<Behavior*>::const_iterator it = getChildren().begin();
        it != getChildren().end(); ++it)
    {
        (*it)->terminate();
    }

    Behavior::terminate();
}

void Parallel::onSuccess(Behavior* behavior)
{
    // Set the corresponding flags for the behavior.
    const uint32_t idx = indexOf(behavior);
    ReturnCode before = mCodes[idx];
    mCodes[idx] = ReturnCodeSuccess;

    if(allChildrenSucceeded())
    {
        // Signal successful execution only after all children have succeeded.
        notifySuccess();
    }
    else if(before == ReturnCodeFailure && !anyChildrenFailed())
    {
        // This behavior previously failed the parallel node.
        // Schedule the parallel node to run again to restart all terminated nodes and
        // tell the parent node that this node has an uncertain state (again).
        setStatus(StatusRunning);
        notifyReset();
    }
}

void Parallel::onFailure(Behavior* behavior)
{
    // Clear the corresponding flags for the behavior.
    const uint32_t idx = indexOf(behavior);
    const bool wasFailed = anyChildrenFailed();
    mCodes[idx] = ReturnCodeFailure;

    if(!wasFailed)
    {
        // End all remaining, active parallel tasks.
        for(std::vector<Behavior*>::const_iterator it = getChildren().begin();
            it != getChildren().end(); ++it)
        {
            if((*it)->getStatus() == StatusRunning ||
               (*it)->getStatus() == StatusWaiting)
            {
                (*it)->terminate();
            }
        }

        // Signal the failure of this node to the parent node.
        notifyFailure();
    }
}

void Parallel::onReset(Behavior* behavior)
{
    // Clear the corresponding flags for the behavior.
    const uint32_t idx = indexOf(behavior);
    const bool wasSuccess = allChildrenSucceeded();
    const ReturnCode before = mCodes[idx];
    mCodes[idx] = ReturnCodeNone;

    // Notify the parent node that this node has an uncertain state if
    // the __behavior__ was determining this node's return state.
    if((before == ReturnCodeSuccess && wasSuccess) ||
       (before == ReturnCodeFailure && !anyChildrenFailed()))
    {
        setStatus(StatusWaiting);
        notifyReset();
    }
}

void Parallel::resetCodes()
{
    std::memset(mCodes, ReturnCodeNone, sizeof(ReturnCode)*getChildren().size());
}

bool Parallel::allChildrenSucceeded() const
{
    for(uint32_t i = 0; i < getChildren().size(); ++i)
    {
        if(mCodes[i] != ReturnCodeSuccess)
        {
            return false;
        }
    }
    return true;
}

bool Parallel::anyChildrenFailed() const
{
    for(uint32_t i = 0; i < getChildren().size(); ++i)
    {
        if(mCodes[i] == ReturnCodeFailure)
        {
            return true;
        }
    }
    return false;
}

Decorator::Decorator(Scheduler& scheduler, Behavior* child) :
    mScheduler(scheduler),
    mChild(child)
{
    UNUSED(mScheduler);
    AI_ASSERT(mChild, "Child mustn't be NULL.");
    mChild->setListener(this);
}

Decorator::~Decorator()
{
    mChild->setListener(NULL);
}

void Decorator::terminate()
{
    terminateChild();
    Behavior::terminate();
}

void Decorator::onSuccess(Behavior* behavior)
{
    UNUSED(behavior);
    notifySuccess();
}

void Decorator::onFailure(Behavior* behavior)
{
    UNUSED(behavior);
    notifyFailure();
}

void Decorator::onReset(Behavior* behavior)
{
    UNUSED(behavior);
    notifyReset();
}

void Decorator::setUserData(const hold_any &data)
{
    Behavior::setUserData(data);
    mChild->setUserData(data);
}

const Behavior* Decorator::getChild() const
{
    return mChild;
}

void Decorator::scheduleBehavior()
{
    mScheduler.enqueue(mChild);
}

void Decorator::terminateChild()
{
    mChild->terminate();
}

Composite::BehaviorList make_shuffled(const Composite::BehaviorList& children)
{
    Composite::BehaviorList list = children;
    shuffle(list.begin(), list.end());
    return list;
}

RandomSelector::RandomSelector(Scheduler& scheduler,
                               const Composite::BehaviorList& children) :
    RandomComposite<Selector>::RandomComposite(scheduler, children)
{
    ;
}

RandomSelector::~RandomSelector()
{
    ;
}

RandomSequence::RandomSequence(Scheduler& scheduler,
                               const Composite::BehaviorList& children) :
    RandomComposite<Sequence>::RandomComposite(scheduler,children)
{
    ;
}

RandomSequence::~RandomSequence()
{
    ;
}

END_NS_AILIB
