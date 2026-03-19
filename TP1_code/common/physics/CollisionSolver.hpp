#pragma once
#include "CollisionManifold.hpp"
#include "CollisionUtils.hpp"
#include "RigidBody.hpp"
#include <cmath>

namespace physics
{
    class CollisionSolver
    {
    public:
        static void Resolve(RigidBody & a, RigidBody & b, const CollisionManifold & manifold)
        {
            if (!manifold.hasCollision)
                return;
            
            PositionalCorrection(a, b, manifold);
            ApplyImpulse(a, b, manifold);
        }
    private:
        static void PositionalCorrection(RigidBody & a, RigidBody & b, const CollisionManifold & manifold)
        {
            const float invMassA = a.inverseMass;
            const float invMassB = b.inverseMass;
            const float invMassSum = invMassA + invMassB;

            if (invMassSum <= CollisionUtils::kEpsilon)
                return;
            constexpr float percent = 0.8f; // pourcentage de correction
            constexpr float slop = 0.001f; // tolérance;

            const float correctionMagnitude = std::max(manifold.penetration - slop, 0.0f) * percent / invMassSum;

            const glm::vec3 correction = manifold.normal * correctionMagnitude;
            
            a.m_position -= correction * invMassA;
            b.m_position += correction * invMassB;
        }
        static void ApplyImpulse(RigidBody & a, RigidBody & b, const CollisionManifold & manifold)
        {
            const float invMassA = a.inverseMass;
            const float invMassB = b.inverseMass;
            const float invMassSum = invMassA + invMassB;

            if (invMassSum <= CollisionUtils::kEpsilon)
                return;
            
            const glm::vec3 relativeVelocity = b.m_linearVelocity - a.m_linearVelocity;
            const float velAlongNormal = CollisionUtils::Dot(relativeVelocity, manifold.normal);
            
            if (velAlongNormal > 0.0f)
                return;
            
            const float restitution = std::min(a.restitution, b.restitution);

            const float impulseScalar = -(1.0f + restitution) * velAlongNormal / invMassSum;
            const glm::vec3 impulse = manifold.normal * impulseScalar;

            a.m_linearVelocity -= impulse * invMassA;
            b.m_linearVelocity += impulse * invMassB;
        }
    };
}