#include "Steering.h"

BEGIN_NS_AILIB

btVector3 Steering::search(const btVector3& position,
                           const btVector3& target,
                           btScalar maxVelocity)
{
    return (target - position).normalized() * maxVelocity;
}

btVector3 Steering::flee(const btVector3& position,
                         const btVector3& target,
                         btScalar maxVelocity)
{
    return (position - target).normalized() * maxVelocity;
}

btVector3 Steering::pursuit(const btVector3& position,
                            const btVector3& target,
                            const btVector3& targetVelocity,
                            btScalar lookaheadTime,
                            btScalar maxVelocity)
{
    return search(position, target + targetVelocity * lookaheadTime, maxVelocity);
}

btVector3 Steering::evade(const btVector3& position,
                          const btVector3& target,
                          const btVector3& targetVelocity,
                          btScalar lookaheadTime,
                          btScalar maxVelocity)
{
    return flee(position, target + targetVelocity * lookaheadTime, maxVelocity);
}

btVector3 Steering::avoidObstacle(const btVector3& position,
                                  const btVector3& currentVelocity,
                                  btScalar maxVelocity,
                                  btScalar lookaheadTime,
                                  RaycastFunction fun)
{
    RaycastResult res = fun(position, position + currentVelocity * lookaheadTime);

    if(res.hasHit)
    {
        btVector3 hitPoint = position + currentVelocity * res.distance;
        btScalar currentSpeed = currentVelocity.length();
        if(position.distance(hitPoint) / currentSpeed < lookaheadTime)
        {
            btVector3 searchPoint(hitPoint.x() + lookaheadTime, hitPoint.y(), hitPoint.z());
            return search(position, searchPoint, maxVelocity);
        }
    }

    return btVector3(0,0,0);
}

END_NS_AILIB
