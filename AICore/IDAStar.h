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
    typedef std::vector<const node_type*> path_type;
    typedef std::vector<Connection> connections_type;
    typedef real_type(*Heuristic)(const node_type&,
                                  const node_type&);

    IDAStar(const GRAPH& graph) :
        mGraph(graph)
    {
        ;
    }

    path_type findPath(const node_type* const start,
                       const node_type* const goal,
                       Heuristic heuristic,
                       const uint32_t maxDepth,
                       connections_type* /* out */ connections = NULL) const
    {
        AI_ASSERT(start, "Supplied a NULL start node.");
        AI_ASSERT(goal, "Supplied a NULL goal node.");

        if(maxDepth == 0)
        {
            // Do nothing if search depth is 0.
            if(connections)
            {
                *connections = connections_type();
            }
            return path_type;
        }

        path_type retVal;
        connections_type tmpConnections;
        real_type estimate = heuristic(*start, *goal);
        while(true)
        {
            real_type newEstimate;
            DepthSearchResult res = depthSearch(start,
                                                goal,
                                                retVal,
                                                0, //< current cost
                                                estimate,
                                                heuristic,
                                                1, // < depth
                                                maxDepth,
                                                newEstimate,
                                                tmpConnections);

            if(res == DepthSearchResultFound)
            {
                if(connections)
                {
                    *connections = tmpConnections;
                }
                return retVal;
            }
            else if(res == DepthSearchResultUnreachable)
            {
                if(connections)
                {
                    *connections = connections_type();
                }
                return path_type();
            }

            estimate = newEstimate;
        }
    }
private:
    enum DepthSearchResult
    {
        DepthSearchResultFound = 0,
        DepthSearchResultUnreachable,
        DepthSearchResultCost
    };

    DepthSearchResult depthSearch(const node_type* node,
                                  const node_type* const goal,
                                  path_type& currentPath,
                                  real_type currentCost,
                                  real_type currentEstimate,
                                  Heuristic heuristic,
                                  const uint32_t depth,
                                  const uint32_t maxDepth,
                                  real_type& /* out */ nextEstimate,
                                  connections_type* /* out */ connections) const
    {
        real_type estimate = currentCost + heuristic(*node, *goal);
        if(estimate > currentEstimate)
        {
            nextEstimate = estimate;
            return DepthSearchResultCost;
        }

        if(node == goal)
        {
            currentPath.resize(depth);
            connections->resize(depth - 1);
            currentPath[depth - 1] = node;
            return DepthSearchResultFound;
        }

        if(depth == maxDepth)
        {
            return DepthSearchResultUnreachable;
        }

        const node_type* const firstNode = mGraph.getNodesBegin();
        const size_t nodeIdx = node - firstNode;
        AI_ASSERT(nodeIdx < mGraph.getNumNodes(),
                  "The nodes are not in continguous memory.");

        real_type min = std::numeric_limits<real_type>::max();
        const Edge* const end = mGraph.getSuccessorsEnd(nodeIdx);
        for(const Edge* it = mGraph.getSuccessorsBegin(nodeIdx); it != end; ++it)
        {
            real_type tmp = std::numeric_limits<real_type>::max();
            DepthSearchResult res = depthSearch(mGraph.getNode(it->targetIndex),
                                                goal,
                                                currentPath,
                                                currentCost + it->cost,
                                                currentEstimate,
                                                heuristic,
                                                depth + 1,
                                                maxDepth,
                                                tmp,
                                                connections);

            if(res == DepthSearchResultFound)
            {
                (*connections)[depth - 1] = Connection::makeConnection(nodeIdx, it - begin);
                return DepthSearchResultFound;
            }
            else if(tmp < min)
            {
                min = tmp;
            }
        }

        if(min == std::numeric_limits<real_type>::max())
        {
            return DepthSearchResultUnreachable;
        }
        else
        {
            nextEstimate = min;
            return DepthSearchResultCost;
        }
    }
};

END_NS_AILIB

#endif // IDASTAR_H
