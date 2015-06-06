#ifndef STEERING_H
#define STEERING_H

#include "ai_global.h"
#include <vector>
#include <LinearMath/btVector3.h>

BEGIN_NS_AILIB

class Steering
{
public:
    class RaycastResult
    {
    public:
        bool hasHit;
        real_type distance;
    };

    typedef RaycastResult(*RaycastFunction)(const btVector3& origin, const btVector3& direction);

    static btVector3 search(const btVector3& position,
                            const btVector3& target,
                            btScalar maxVelocity);

    static btVector3 flee(const btVector3& position,
                          const btVector3& target,
                          btScalar maxVelocity);

    static btVector3 pursuit(const btVector3& position,
                             const btVector3& target,
                             const btVector3& targetVelocity,
                             btScalar lookaheadTime,
                             btScalar maxVelocity);

    static btVector3 evade(const btVector3& position,
                           const btVector3& target,
                           const btVector3& targetVelocity,
                           btScalar lookaheadTime,
                           btScalar maxVelocity);

    static btVector3 avoidObstacle(const btVector3& position,
                                   const btVector3& currentVelocity,
                                   btScalar maxVelocity,
                                   btScalar lookaheadTime,
                                   RaycastFunction fun);
};

END_NS_AILIB

#endif // STEERING_H
