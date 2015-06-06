#ifndef IDASTAR_H
#define IDASTAR_H

#pragma once

#include "ai_global.h"
#include "graph.h"
#include <limits>

BEGIN_NS_AILIB

/**
 * @brief The IDAStar class implements the Iterative Deepening A* algorithm. The algorithm requires
 * a constant amount of memory and is thus more suitable for extremely large search spaces than
 * the A* algorithm. However, reducing the memory requirements comes at the cost of performance,
 * because nodes may be expanded multiple times in one search.
 */
template <typename GRAPH>
class IDAStar
{
private:
    const GRAPH& mGraph;
public:
    typedef typename GRAPH::node_type node_type;
    typedef typename GRAPH::edge_type edge_type;
    typedef std::vector<const node_type*> path_type;
    typedef std::vector<Connection> connections_type;
    typedef real_type(*Heuristic)(const node_type&,
                                  const node_type&);
private:
    class HeuristicComparator
    {
        const GRAPH& mGraph;
        const node_type& mGoal;
        Heuristic mHeuristic;
    public:
        HeuristicComparator(const GRAPH& graph, const node_type& goal, Heuristic heuristic) :
            mGraph(graph),
            mGoal(goal),
            mHeuristic(heuristic)
        {
            ;
        }

        FORCE_INLINE bool operator()(const edge_type* lv, const edge_type* rv) const
        {
            const node_type* left  = mGraph.getNode(lv->targetIndex);
            const node_type* right = mGraph.getNode(rv->targetIndex);
            return mHeuristic(*left, mGoal) > mHeuristic(*right, mGoal); // Sort from worst to best.
        }
    };
public:

    IDAStar(const GRAPH& graph) :
        mGraph(graph)
    {
        ;
    }

    path_type findPath(const node_type* const start,
                       const node_type* const goal,
                       Heuristic heuristic,
                       const int32_t maxDepth,
                       connections_type* /* out */ connections = NULL) const
    {
        AI_ASSERT(start, "Supplied a NULL start node.");
        AI_ASSERT(goal, "Supplied a NULL goal node.");
        AI_ASSERT(maxDepth >= 0, "Maximum search depth must be a positive value.");

        if(maxDepth == 0)
        {
            // Do nothing if search depth is 0.
            if(connections)
            {
                *connections = connections_type();
            }
            return path_type();
        }

        path_type nodeStack(maxDepth);
        connections_type edgeStack(maxDepth);
        std::vector< std::vector<const edge_type*> > childrenStack(maxDepth);
        std::vector<real_type> costStack(maxDepth);
        real_type nextEstimate = heuristic(*start, *goal);

        while(true)
        {
            real_type estimate = nextEstimate;
            nextEstimate = std::numeric_limits<real_type>::max();
            int32_t depth = 0;
            pushNode(nodeStack, edgeStack, childrenStack, costStack, goal, heuristic, depth, start);

            while(depth >= 0)
            {
                if(childrenStack[depth].empty())
                {
                    // POP current top element, we don't actually need to do anything with the
                    // stacks here, as they will be overwritten when necessary.
                    depth--;
                }
                else
                {
                    const edge_type* bestCandidate = childrenStack[depth].back();
                    childrenStack[depth].pop_back();

                    const node_type* nextNode = mGraph.getNode(bestCandidate->targetIndex);

                    const real_type costUntilNow = costStack[depth]*costStack[depth];
                    const real_type heuristicValue = heuristic(*nextNode, *goal);

                    real_type currentCost = costUntilNow + heuristicValue;
                    if(currentCost <= estimate)
                    {
                        if(nextNode == goal)
                        {
                            if(connections)
                            {
                                *connections = connections_type(edgeStack.begin(),
                                                                edgeStack.begin() + depth + 1);
                            }
                            nodeStack.resize(depth);
                            return nodeStack;
                        }

                        if(depth + 1 < maxDepth)
                        {
                            depth++;
                            pushNode(nodeStack,
                                     edgeStack,
                                     childrenStack,
                                     costStack,
                                     goal,
                                     heuristic,
                                     depth,
                                     nextNode,
                                     bestCandidate);
                        }
                    }
                    else
                    {
                        nextEstimate = std::min(nextEstimate, currentCost);
                    }
                }
            }
        }
    }
private:
    void pushNode(path_type& nodeStack,
                  connections_type& edgeStack,
                  std::vector< std::vector<const edge_type*> >& childrenStack,
                  std::vector<real_type>& costStack,
                  const node_type* goal,
                  Heuristic heuristic,
                  uint32_t depth,
                  const node_type* node,
                  const edge_type* edge = NULL) const
    {
        nodeStack[depth] = node;

        const node_type* const firstNode = mGraph.getNodesBegin();
        const size_t index = node - firstNode;
        if(depth > 0)
        {
            AI_ASSERT(edge != NULL,
                      "Must specify the edge taken to get to the next node.");
            edgeStack[depth] = Connection::makeConnection(index, edge->targetIndex);
            costStack[depth] = costStack[depth-1] + edge->cost;
        }
        else
        {
            costStack[depth] = 0;
        }

        const edge_type* const begin = mGraph.getSuccessorsBegin(index);
        const edge_type* const end = mGraph.getSuccessorsEnd(index);
        const size_t numEdges = end - begin;
        childrenStack[depth] = std::vector<const edge_type*>(numEdges);

        for(size_t i = 0; i < numEdges; ++i)
        {
            childrenStack[depth][i] = (begin + i);
        }

        std::sort(childrenStack[depth].begin(),
                  childrenStack[depth].end(),
                  HeuristicComparator(mGraph, *goal, heuristic));
    }
};

END_NS_AILIB

#endif // IDASTAR_H
