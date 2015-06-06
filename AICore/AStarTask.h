#ifndef ASTARTASK_H
#define ASTARTASK_H

#pragma once

#include "ai_global.h"
#include "Task.h"
#include "AStar.h"

BEGIN_NS_AILIB

template <typename GRAPH>
class AStarTaskListener
{
public:
    typedef AStar<GRAPH> AStarType;
    typedef typename AStarType::path_type path_type;
    typedef typename AStarType::connections_type connections_type;

    virtual void onAStarResult(AStarType* task,
                               const typename AStarType::path_type& path,
                               const connections_type* connections) = 0;
};

template <typename GRAPH, uint32_t STEPS_PER_RUN = 500>
class AStarTask : public AStar<GRAPH>, public Task
{
public:
    typedef AStar<GRAPH> AStarType;

    typedef typename AStarType::node_type node_type;
    typedef typename AStarType::edge_type edge_type;
    typedef typename AStarType::Heuristic Heuristic;
    typedef typename AStarType::Comparator Comparator;
    typedef typename AStarType::OpenList OpenList;
    typedef typename AStarType::path_type path_type;
    typedef typename AStarType::connections_type connections_type;

    AStarTask(AStarTaskListener<GRAPH>* listener,
              const GRAPH& graph,
              const node_type* const start,
              const node_type* const goal,
              Heuristic heuristic = &AStarType::zeroHeuristic,
              Comparator comparator = &AStarType::equalsComparator,
              connections_type* /* out */ connections = NULL) :
        AStar<GRAPH>(graph),
        mListener(listener),
        mStart(start),
        mGoal(goal),
        mHeuristic(heuristic),
        mComparator(comparator),
        mConnections(connections),
        mStartIdx(0)
    {
        mStartIdx = AStarType::initialise(mStart, *mGoal, mHeuristic, mOpen);
    }

    virtual void run()
    {
        if(!mListener)
        {
            setStatus(StatusTerminated);
            return;
        }
        uint32_t steps = 0;
        while(LIKELY(!mOpen.empty()))
        {
            if(AStarType::step(*mGoal, mHeuristic, mComparator, mOpen))
            {
                // We found a valid short path. Return the result.
                path_type path = AStarType::buildPath(mOpen.top(), mStartIdx, mConnections);
                setStatus(StatusTerminated);
                mListener->onAStarResult(this, path, mConnections);
                return;
            }
            steps++;

            if(steps >= STEPS_PER_RUN)
            {
                return;
            }
        }

        setStatus(StatusTerminated);
        // No solution found. Return an empty path.
        mListener->onAStarResult(this, path_type(), NULL);
    }

private:
    OpenList mOpen;
    AStarTaskListener<GRAPH>* mListener;
    const node_type* const mStart, *mGoal;
    Heuristic mHeuristic;
    Comparator mComparator;
    connections_type* mConnections;
    size_t mStartIdx;
};

END_NS_AILIB

#endif // ASTARTASK_H
