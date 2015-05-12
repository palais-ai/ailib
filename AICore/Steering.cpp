#include "Steering.h"

btVector3 search(const btVector3& position,
                 const btVector3& target,
                 const btVector3& currentVelocity,
                 btScalar maxVelocity)
{
    return (target - position).normalized() * maxVelocity - currentVelocity;
}

btVector3 flee(const btVector3& position,
               const btVector3& target,
               const btVector3& currentVelocity,
               btScalar maxVelocity)
{
    return (position - target).normalized() * maxVelocity - currentVelocity;
}

btVector3 pursuit(const btVector3& position,
                  const btVector3& target,
                  const btVector3& currentVelocity,
                  const btVector3& targetVelocity,
                  btScalar lookaheadTime,
                  btScalar maxVelocity)
{
    return search(position, target + targetVelocity * lookaheadTime, currentVelocity, maxVelocity);
}

btVector3 evade(const btVector3& position,
                const btVector3& target,
                const btVector3& currentVelocity,
                const btVector3& targetVelocity,
                btScalar lookaheadTime,
                btScalar maxVelocity)
{
    return flee(position, target + targetVelocity * lookaheadTime, currentVelocity, maxVelocity);
}



btVector3 followPath(const btVector3& segmentStart,
                     const btVector3& segmentEnd,
                     const btVector3& position,
                     const btVector3& currentVelocity,
                     btScalar lookaheadTime,
                     btScalar maxVelocity)
{
    return btVector3();
}

btVector3 avoidObstacle()
{
    return btVector3();
}
