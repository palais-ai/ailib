#ifndef ASTAR_H
#define ASTAR_H

#pragma once

#include "ai_global.h"
#include "graph.h"
#include <cstring>
#include <algorithm>
#include <queue>

BEGIN_NS_AILIB

/**
 * @brief The AStar class implements the A* graph search algorithm using a preallocated node cache.
 * By preallocating we use O(n) memory but significantly increase the performance of the A* search.
 * Use the IDAStar class if preallocation is not feasible.
 */
template <typename GRAPH>
class AStar
{
private:
    /**
     * @brief AStarNode carries the bookkeeping information necessary for the A* algorithm.
     */
    class AStarNode
    {
    public:
        enum NodeState
        {
            NodeStateUnvisited = 0,
            NodeStateClosed,
            NodeStateOpen
        };

        real_type estTotalCost;
        real_type currentCost;
        AStarNode* parent;
        uint16_t connection;
        NodeState state;
    };

    /**
     * @brief The NodeComparator class is used for sorting the internal priority queue of the A*
     * algorithm.
     */
    class NodeComparator
    {
    public:
        FORCE_INLINE bool operator() (const AStarNode* lv, const AStarNode* rv) const
        {
            // Greather than ('>') because priority_queue orders from highest to lowest.
            // We require the opposite.
            return lv->estTotalCost > rv->estTotalCost;
        }
    };

    const GRAPH& mGraph;
    mutable std::vector<AStarNode> mNodeInfo; //< Cache structure
public:
    typedef typename GRAPH::node_type node_type;
    typedef typename GRAPH::edge_type edge_type;
    typedef std::vector<const node_type*> path_type;
    typedef std::vector<Connection> connections_type;
    typedef real_type(*Heuristic)(const node_type&,
                                  const node_type&);
    typedef bool(*Comparator)(const node_type&,
                              const node_type&);

    typedef std::priority_queue<AStarNode*,
                                std::vector<AStarNode*>,
                                NodeComparator> OpenList;

    FORCE_INLINE static bool equalsComparator(const node_type& lv,
                                              const node_type& rv)
    {
        return lv == rv;
    }

    // Using the zeroHeuristic results in A* simply processing all possible paths.
    FORCE_INLINE static real_type zeroHeuristic(const node_type&,
                                                const node_type&)
    {
        return 0;
    }

    AStar(const GRAPH& staticGraph) :
        mGraph(staticGraph),
        // Pre-allocate bookkeeping information for every node
        mNodeInfo(mGraph.getNumNodes())
    {
        ;
    }

    FORCE_INLINE path_type findPath(const node_type* const start,
                                    const node_type& goal,
                                    Comparator comparator = &equalsComparator,
                                    connections_type* /* out */ connections = NULL) const
    {
        return findPath(start, goal, &zeroHeuristic, comparator, connections);
    }

    /**
     * @brief findPath retrieves a shortest path between a __start__ and a __goal__ node,
     *        if a path exists.
     *
     * @param start A pointer to the start node on the search graph.
     * @param goal A reference to the goal node on the search graph.
     * @param heuristic Optional. The heuristic used to guide the search. Defaults to the zero
     *                  heuristic. Which means all paths will be searched. Use an appropiate
     *                  heuristic for your use case for optimal performance. The heuristic MUST
     *                  be admissible (never overestimates) in order for this algorithm to work.
     * @param comparator Optional. The node comparison function. Defaults to operator==()
     * @param connections Optional. Returns the sequence of connections of the shortest path.
     *                    Defaults to NULL.
     *
     * @return The path taken. A path is a sequence of nodes. Empty if no path can be found.
     *         Use the __connection__ output parameter to obtain the connection sequence.
     */
    path_type findPath(const node_type* const start,
                       const node_type& goal,
                       Heuristic heuristic = &zeroHeuristic,
                       Comparator comparator = &equalsComparator,
                       connections_type* /* out */ connections = NULL) const
    {
        AI_ASSERT(start, "Supplied a NULL start node.");

        // Make sure there is enough space in the node cache to handle all nodes.
        // This is necessary if the graph has changed its size in between construction
        // and this path query.
        mNodeInfo.reserve(mGraph.getNumNodes());

        OpenList open;

        // Zero-initialize the bookkeeping information
        std::memset(&mNodeInfo[0],
                    0,
                    mNodeInfo.size() * sizeof(AStarNode));

        const node_type* const firstNode = mGraph.getNodesBegin();
        const AStarNode* const firstNodeInfo = &mNodeInfo[0];

        const size_t startIdx = start - firstNode;
        AI_ASSERT(startIdx < mGraph.getNumNodes(),
                  "The nodes are not in continguous memory.");

        // Add the start node to the open list.
        AStarNode* startNode = &mNodeInfo[startIdx];
        startNode->estTotalCost = heuristic(*start, goal);
        startNode->state = AStarNode::NodeStateOpen;
        open.push(startNode);

        while(LIKELY(!open.empty()))
        {
            AStarNode* lowestCostNode = open.top();

            const size_t lowestCostIdx = lowestCostNode - firstNodeInfo;
            AI_ASSERT(lowestCostIdx < mGraph.getNumNodes(),
                   "The nodes are not in continguous memory.");

            const node_type* lowestCost = mGraph.getNode(lowestCostIdx);
            if(UNLIKELY(comparator(*lowestCost, goal)))
            {
                // We found a valid short path.
                // It's guaranteed to be the shortest, if our heuristic is underestimating.
                return buildPath(lowestCostNode, startIdx, connections);
            }

            lowestCostNode->state = AStarNode::NodeStateClosed;

            // The lowest cost node is going to be processed and removed from the open list.
            // We have to remove it before adding any children in case they have
            // a better cost value.
            open.pop();

            // Expand all child nodes of the current node.
            expand(lowestCostNode, goal, heuristic, lowestCostIdx, open);
        }

        // No solution found. Return an empty path.
        return path_type();
    }

private:
    void expand(AStarNode* node,
                const node_type& goal,
                Heuristic heuristic,
                const size_t index,
                OpenList& open) const
    {
        const edge_type* const end = mGraph.getSuccessorsEnd(index);
        const edge_type* const begin = mGraph.getSuccessorsBegin(index);
        for(const edge_type* it = begin; it != end; ++it)
        {
            const size_t targetIdx = it->targetIndex;
            AI_ASSERT(targetIdx < mGraph.getNumNodes(),
                      "The nodes are not in continguous memory.");

            real_type targetCost = node->currentCost + it->cost;
            real_type heuristicValue = 0.;

            AStarNode* targetNode = &mNodeInfo[targetIdx];
            if(targetNode->state == AStarNode::NodeStateUnvisited)
            {
                const node_type* target = mGraph.getNode(targetIdx);

                // We have to calculate the heuristic for unvisited nodes.
                heuristicValue = heuristic(*target, goal);

                targetNode->estTotalCost = targetCost + heuristicValue;
                targetNode->state = AStarNode::NodeStateOpen;
                open.push(targetNode);
            }
            else
            {
                if(LIKELY(targetNode->currentCost <= targetCost))
                {
                    // Continue if this node doesn't offer improvement
                    continue;
                }

                // Reuse the heuristic value
                heuristicValue = targetNode->estTotalCost - targetNode->currentCost;

                targetNode->estTotalCost = targetCost + heuristicValue;
                if(targetNode->state != AStarNode::NodeStateOpen)
                {
                    targetNode->state = AStarNode::NodeStateOpen;
                    open.push(targetNode);
                }
            }

            targetNode->parent = node;
            targetNode->connection = it - begin;
            AI_ASSERT(targetNode->connection < mGraph.getNumEdges(index),
                      "The nodes are not in continguous memory.");
            targetNode->currentCost = targetCost;
        }
    }

    path_type buildPath(const AStarNode* fromNode,
                        const size_t startIdx,
                        connections_type* /* out */ connections) const
    {
        const AStarNode* const firstNodeInfo = &mNodeInfo[0];
        const AStarNode* const startNode = &mNodeInfo[startIdx];
        const node_type* const start = mGraph.getNode(startIdx);

        path_type retVal;

        real_type cost = 0;
        const AStarNode* currentNode = fromNode;
        while(currentNode != startNode)
        {
            const size_t currentIdx = currentNode - firstNodeInfo;
            AI_ASSERT(currentIdx < mGraph.getNumNodes(),
                      "The nodes are not in continguous memory.");

            const node_type* current = mGraph.getNode(currentIdx);

            // Add this node to the path
            retVal.push_back(current);

            const size_t parentIdx = currentNode->parent - firstNodeInfo;
            AI_ASSERT(parentIdx < mGraph.getNumNodes(),
                      "The nodes are not in continguous memory.");

            // Find the path that was taken to calculate the path cost
            const edge_type* const connectionsBegin = mGraph.getSuccessorsBegin(parentIdx);
            AI_ASSERT(connectionsBegin, "Parent node has no children.");
            AI_ASSERT(currentNode->connection < mGraph.getNumEdges(parentIdx),
                      "Parent node doesn't have enough children.");

            cost += (connectionsBegin + currentNode->connection)->cost;

            // Record the actual edges taken
            if(connections)
            {
                connections->push_back(Connection::makeConnection(parentIdx,
                                                                  currentNode->connection));
            }

            // Move on to the next node in the chain.
            currentNode = currentNode->parent;
        }

        retVal.push_back(start);

        if(connections)
        {
            *connections = connections_type(connections->rbegin(), connections->rend());
        }

        // Reverse the path so it is in order from __start__ to __goal__
        return path_type(retVal.rbegin(), retVal.rend());
    }
};

END_NS_AILIB

#endif // ASTAR_H
