#ifndef GRAPH_H
#define GRAPH_H

#pragma once

#include "ai_global.h"
#include <stdint.h>
#include <vector>

BEGIN_NS_AILIB

class Connection
{
public:
    static Connection makeConnection(uint16_t fromNode,
                                     uint16_t edgeIndex);

    uint16_t fromNode;
    uint16_t edgeIndex;
};

class Edge
{
public:
    typedef void user_type;

    static Edge makeEdge(uint16_t targetIndex,
                         real_type cost,
                         user_type* userData = NULL);

    real_type cost;
    uint16_t targetIndex;
};

template <typename USER_TYPE>
class UserDataEdge : public Edge
{
public:
    typedef USER_TYPE user_type;

    static UserDataEdge makeEdge(uint16_t targetIndex,
                                 real_type cost,
                                 user_type* userData = NULL)
    {
        UserDataEdge retVal;
        retVal.cost = cost;
        retVal.targetIndex = targetIndex;
        retVal.userData = userData;
        return retVal;
    }

    user_type* userData;
};

// Preallocated edge-list
template <size_t MAX_EDGES, typename EDGE_TYPE = Edge>
class BaseNode
{
    STATIC_ASSERT(MAX_EDGES <= 8)
public:
    typedef EDGE_TYPE edge_type;

    BaseNode() :
        mNumEdges(0)
    {
        ;
    }

    FORCE_INLINE const edge_type* beginSuccessors() const
    {
        return &mEdges[0];
    }

    FORCE_INLINE const edge_type* endSuccessors() const
    {
        return &mEdges[mNumEdges];
    }

    FORCE_INLINE void addEdge(const edge_type& edge)
    {
        AI_ASSERT(mNumEdges <= MAX_EDGES,
                  "This node can only have __MAX_EDGES__ edges.");

        mEdges[mNumEdges++] = edge;
    }

    FORCE_INLINE size_t getNumEdges() const
    {
        return mNumEdges;
    }

private:
    edge_type mEdges[MAX_EDGES];
    uint8_t mNumEdges;
};

// Dynamically allocated edge-list, default
template <typename EDGE_TYPE>
class BaseNode<0, EDGE_TYPE>
{
public:
    typedef EDGE_TYPE edge_type;
    typedef std::vector<edge_type> edge_collection;

    const edge_type* beginSuccessors() const
    {
        if(mEdges.size() == 0)
        {
            return NULL;
        }

        return &mEdges[0];
    }

    const edge_type* endSuccessors() const
    {
        if(mEdges.size() == 0)
        {
            return NULL;
        }

        return &mEdges[0] + mEdges.size();
    }

    FORCE_INLINE void addEdge(const edge_type& edge)
    {
        mEdges.push_back(edge);
    }

    FORCE_INLINE size_t getNumEdges() const
    {
        return mEdges.size();
    }
private:
    edge_collection mEdges;
};

template <typename NODE_TYPE, size_t MAX_EDGES = 0, typename EDGE_TYPE = Edge>
class Graph
{
public:
    typedef NODE_TYPE node_type;
    typedef EDGE_TYPE edge_type;
    typedef BaseNode<MAX_EDGES, EDGE_TYPE> connections_type;
    typedef std::vector<node_type> node_collection;
    typedef std::vector<connections_type> connection_collection;

    size_t addNode(const NODE_TYPE& node)
    {
        mNodes.push_back(node);
        mConnections.push_back(connections_type());

        return mNodes.size() - 1;
    }

    FORCE_INLINE void addEdge(size_t from,
                              size_t to,
                              real_type weight,
                              typename edge_type::user_type* userData = NULL)
    {
        mConnections[from].addEdge(EDGE_TYPE::makeEdge(to, weight, userData));
    }

    const node_type* getNodesBegin() const
    {
        if(mNodes.size() == 0)
        {
            return NULL;
        }
        return &mNodes[0];
    }

    const node_type* getNodesEnd() const
    {
        if(mNodes.size() == 0)
        {
            return NULL;
        }
        return &mNodes[0] + mNodes.size();
    }

    FORCE_INLINE const edge_type* getSuccessorsBegin(size_t idx) const
    {
        return mConnections[idx].beginSuccessors();
    }

    FORCE_INLINE const edge_type* getSuccessorsEnd(size_t idx) const
    {
        return mConnections[idx].endSuccessors();
    }

    FORCE_INLINE size_t getNumEdges(size_t idx) const
    {
        return mConnections[idx].getNumEdges();
    }

    FORCE_INLINE const node_type* getNode(size_t idx) const
    {
        return &mNodes[idx];
    }

    FORCE_INLINE node_type* getNode(size_t idx)
    {
        return &mNodes[idx];
    }

    FORCE_INLINE size_t getNumNodes() const
    {
        return mNodes.size();
    }
private:
    connection_collection mConnections;
    node_collection mNodes;
};

END_NS_AILIB

#endif // GRAPH_H
