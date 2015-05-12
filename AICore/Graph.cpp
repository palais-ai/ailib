#include "Graph.h"

BEGIN_NS_AILIB

Connection Connection::makeConnection(uint16_t fromNode,
                                      uint16_t edgeIndex)
{
    Connection retVal;
    retVal.fromNode  = fromNode;
    retVal.edgeIndex = edgeIndex;
    return retVal;
}

Edge Edge::makeEdge(uint16_t targetIndex,
                    real_type cost,
                    Edge::user_type* userData)
{
    UNUSED(userData);

    Edge retVal;
    retVal.cost = cost;
    retVal.targetIndex = targetIndex;
    return retVal;
}

END_NS_AILIB
