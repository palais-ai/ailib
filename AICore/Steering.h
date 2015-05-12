#ifndef STEERING_H
#define STEERING_H

#include <vector>
#include <LinearMath/btVector3.h>

class Steering
{
public:
    static btVector3 search(const btVector3& position,
                            const btVector3& target,
                            const btVector3& currentVelocity,
                            btScalar maxVelocity);

    static btVector3 flee(const btVector3& position,
                          const btVector3& target,
                          const btVector3& currentVelocity,
                          btScalar maxVelocity);

    static btVector3 pursuit(const btVector3& position,
                             const btVector3& target,
                             const btVector3& currentVelocity,
                             const btVector3& targetVelocity,
                             btScalar lookaheadTime,
                             btScalar maxVelocity);

    static btVector3 evade(const btVector3& position,
                           const btVector3& target,
                           const btVector3& currentVelocity,
                           const btVector3& targetVelocity,
                           btScalar lookaheadTime,
                           btScalar maxVelocity);

    static btVector3 followPath(const btVector3& segmentStart,
                                const btVector3& segmentEnd,
                                const btVector3& position,
                                const btVector3& currentVelocity,
                                btScalar lookaheadTime,
                                btScalar maxVelocity);

    static btVector3 avoidObstacle();
};

#endif // STEERING_H
