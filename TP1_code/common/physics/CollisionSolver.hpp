#pragma once
#include "CollisionManifold.hpp"
#include "CollisionUtils.hpp"
#include "RigidBody.hpp"
#include <algorithm>
#include <cmath>

namespace physics
{
    class CollisionSolver
    {
    public:
        static void Resolve(RigidBody & a, RigidBody & b, const CollisionManifold & manifold, bool applyPositionalCorrection = true)
        {
            if (!manifold.hasCollision)
                return;

            a.RegisterContact(-manifold.normal);
            b.RegisterContact(manifold.normal);
            
            if (applyPositionalCorrection)
                PositionalCorrection(a, b, manifold);
            ApplyImpulse(a, b, manifold);
        }
    private:
        static constexpr float kBounceVelocityThreshold = 0.75f;

        static float ComputeBodyImpulseDenominator(const RigidBody & body, const glm::vec3 & contactPoint, const glm::vec3 & direction)
        {
            if (body.inverseMass <= CollisionUtils::kEpsilon)
                return 0.0f;

            const glm::vec3 contactOffset = contactPoint - body.m_position;
            const glm::vec3 angularAxis = glm::cross(contactOffset, direction);
            const glm::vec3 angularTerm = glm::cross(body.GetInverseInertiaTensorWorld() * angularAxis, contactOffset);
            return body.inverseMass + CollisionUtils::Dot(angularTerm, direction);
        }

        static float ComputeImpulseDenominator(const RigidBody & a, const RigidBody & b, const glm::vec3 & contactPoint, const glm::vec3 & direction)
        {
            return ComputeBodyImpulseDenominator(a, contactPoint, direction)
                + ComputeBodyImpulseDenominator(b, contactPoint, direction);
        }

        static float CombineBounciness(const RigidBody & a, const RigidBody & b)
        {
            return CollisionUtils::ClampRange(std::max(a.bounciness, b.bounciness), 0.0f, 1.0f);
        }

        static float CombineStaticFriction(const RigidBody & a, const RigidBody & b)
        {
            return std::sqrt(std::max(0.0f, a.staticFriction) * std::max(0.0f, b.staticFriction));
        }

        static float CombineDynamicFriction(const RigidBody & a, const RigidBody & b)
        {
            return std::sqrt(std::max(0.0f, a.dynamicFriction) * std::max(0.0f, b.dynamicFriction));
        }

        static void ApplyImpulseAtContact(
            RigidBody & a,
            RigidBody & b,
            const CollisionManifold & manifold,
            const glm::vec3 & contactPoint,
            float restingNormalImpulseScale,
            float frictionImpulseScale
        )
        {
            const float normalMass = ComputeImpulseDenominator(a, b, contactPoint, manifold.normal);

            if (normalMass <= CollisionUtils::kEpsilon)
                return;

            const glm::vec3 relativeVelocity = b.GetVelocityAtWorldPoint(contactPoint) - a.GetVelocityAtWorldPoint(contactPoint);
            const float velAlongNormal = CollisionUtils::Dot(relativeVelocity, manifold.normal);

            if (velAlongNormal > 0.0f)
                return;

            float restitution = CombineBounciness(a, b);
            if (-velAlongNormal < kBounceVelocityThreshold)
                restitution = 0.0f;

            const float normalImpulseScale = restitution > CollisionUtils::kEpsilon ? 1.0f : restingNormalImpulseScale;
            const float normalImpulseScalar = (-(1.0f + restitution) * velAlongNormal / normalMass) * normalImpulseScale;
            if (normalImpulseScalar <= CollisionUtils::kEpsilon)
                return;

            const glm::vec3 impulse = manifold.normal * normalImpulseScalar;

            a.ApplyImpulseAtWorldPoint(-impulse, contactPoint);
            b.ApplyImpulseAtWorldPoint(impulse, contactPoint);

            const glm::vec3 postNormalRelativeVelocity = b.GetVelocityAtWorldPoint(contactPoint) - a.GetVelocityAtWorldPoint(contactPoint);
            glm::vec3 tangent = postNormalRelativeVelocity - (manifold.normal * CollisionUtils::Dot(postNormalRelativeVelocity, manifold.normal));
            const float tangentLengthSq = CollisionUtils::LengthSq(tangent);

            if (tangentLengthSq <= CollisionUtils::kEpsilon)
                return;

            tangent *= 1.0f / std::sqrt(tangentLengthSq);

            const float tangentMass = ComputeImpulseDenominator(a, b, contactPoint, tangent);
            if (tangentMass <= CollisionUtils::kEpsilon)
                return;

            const float tangentImpulseScalar = (-CollisionUtils::Dot(postNormalRelativeVelocity, tangent) / tangentMass) * frictionImpulseScale;
            const float staticFriction = CombineStaticFriction(a, b);
            const float dynamicFriction = CombineDynamicFriction(a, b);

            float frictionImpulseScalar = 0.0f;
            if (std::abs(tangentImpulseScalar) <= normalImpulseScalar * staticFriction)
                frictionImpulseScalar = tangentImpulseScalar;
            else
                frictionImpulseScalar = std::copysign(normalImpulseScalar * dynamicFriction, tangentImpulseScalar);

            const glm::vec3 frictionImpulse = tangent * frictionImpulseScalar;
            a.ApplyImpulseAtWorldPoint(-frictionImpulse, contactPoint);
            b.ApplyImpulseAtWorldPoint(frictionImpulse, contactPoint);
        }

        static void PositionalCorrection(RigidBody & a, RigidBody & b, const CollisionManifold & manifold)
        {
            const float invMassA = a.inverseMass;
            const float invMassB = b.inverseMass;
            const float invMassSum = invMassA + invMassB;

            if (invMassSum <= CollisionUtils::kEpsilon)
                return;
            constexpr float percent = 0.5f; // pourcentage de correction
            constexpr float slop = 0.005f; // tolérance;

            const float correctionMagnitude = std::max(manifold.penetration - slop, 0.0f) * percent / invMassSum;

            const glm::vec3 correction = manifold.normal * correctionMagnitude;
            
            a.m_position -= correction * invMassA;
            b.m_position += correction * invMassB;
        }
        static void ApplyImpulse(RigidBody & a, RigidBody & b, const CollisionManifold & manifold)
        {
            if (manifold.contacts.empty())
            {
                ApplyImpulseAtContact(a, b, manifold, (a.m_position + b.m_position) * 0.5f, 1.0f, 1.0f);
                return;
            }

            const float restingNormalImpulseScale = 1.0f / static_cast<float>(manifold.contacts.size());
            const float frictionImpulseScale = 1.0f / static_cast<float>(manifold.contacts.size());
            for (const ContactPoint & contact : manifold.contacts)
                ApplyImpulseAtContact(a, b, manifold, contact.position, restingNormalImpulseScale, frictionImpulseScale);
        }
    };
}