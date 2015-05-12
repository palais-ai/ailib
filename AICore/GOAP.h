#ifndef GOAP_H
#define GOAP_H

#pragma once

#include "ai_global.h"
#include "Graph.h"
#include <sparsehash/sparse_hash_map>

BEGIN_NS_AILIB

template <typename STATE>
class Action
{
public:
    typedef STATE state_type;

    virtual ~Action() {}

    virtual bool  isPreconditionFulfilled(const state_type& state) const = 0;
    virtual void  applyPostcondition(state_type& state) const = 0;
    virtual float getCost(const state_type& state) const = 0;
};

template <typename T>
struct Hash;

template <typename STATE, size_t MAX_ACTIONS = 0, typename HASH_FUN = Hash<STATE> >
class GOAPPlanner
{
public:
    typedef STATE state_type;
    typedef Action<state_type> action_type;
    typedef Graph<state_type, MAX_ACTIONS, UserDataEdge<action_type> > graph_type;
    typedef google::sparse_hash_map<STATE, uint16_t, HASH_FUN> hash_type;

    GOAPPlanner()
    {
        ;
    }

    void addAction(action_type* action)
    {
        mActions.push_back(action);
    }

    const graph_type& getGraph() const
    {
        return mGraph;
    }

    size_t buildGraph(const state_type& startState, const state_type& endState, uint16_t maxDepth)
    {
        UNUSED(endState);
        mHashTable.clear();
        mGraph = graph_type(); //< Clear before build.
        size_t startIdx = mGraph.addNode(startState);
        recursiveBuildGraph(startIdx, maxDepth, 0);
        return startIdx;
    }

private:
    void recursiveBuildGraph(size_t currentIdx,
                             uint16_t maxDepth,
                             uint16_t currentDepth)
    {
        const state_type currentState = *mGraph.getNode(currentIdx);

        typename std::vector<action_type*>::iterator it;
        for(it = mActions.begin(); it != mActions.end(); ++it)
        {
            action_type* action = *it;

            if(action->isPreconditionFulfilled(currentState))
            {
                state_type nextState = currentState;
                action->applyPostcondition(nextState);

                const size_t nextIdx = mGraph.addNode(nextState);
                mGraph.addEdge(currentIdx, nextIdx, action->getCost(currentState), action);

                // Check if this exact state has been processed before.
                typename hash_type::iterator it = mHashTable.find(nextState);
                if(it == mHashTable.end())
                {
                    // Store this world state to avoid re-processing it.
                    mHashTable[nextState] = currentDepth;
                }
                else
                {
                    if(currentDepth < (*it).second)
                    {
                        (*it).second = currentDepth;
                    }
                    else
                    {
                        // Avoid processing following world states.
                        continue;
                    }
                }

                if(currentDepth < maxDepth)
                {
                    recursiveBuildGraph(nextIdx, maxDepth, currentDepth + 1);
                }
            }
        }
    }

    graph_type mGraph;
    hash_type mHashTable;
    std::vector<action_type*> mActions;
};

END_NS_AILIB

#endif // GOAP_H
