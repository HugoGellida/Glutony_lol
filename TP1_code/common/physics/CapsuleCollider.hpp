#pragma once

#include "Collider.hpp"
#include "../gameobject/Transform.hpp"
#include "glm/glm.hpp"
#include "AABB.hpp"
#include "CollisionUtils.hpp"
#include <cmath>

namespace physics
{
    class CapsuleCollider : public Collider
    {
    private:
        glm::vec3 m_localCenter;
        float m_radius;
        float m_halfHeight;
        glm::vec3 m_localAxis;
    public:
        CapsuleCollider() : Collider()
        {}
        CapsuleCollider(
            const glm::vec3 & localCenter,
            float radius,
            float halfHeight,
            const glm::vec3 & localAxis = glm::vec3(0.0f, 1.0f, 0.0f)
        ) : Collider()
        {
            m_localCenter = localCenter;
            m_radius = radius;
            m_halfHeight = halfHeight;
            m_localAxis = CollisionUtils::NormalizeSafe(localAxis);
        }


        const ColliderType getType() const override
        {
            return ColliderType::Capsule;
        }

        float getWorldRadius(const Transform & world) const
        {
            return m_radius;
        }

        glm::vec3 getWorldCenter(const Transform & world) const
        {
            return world.getWorldPos(m_localCenter);
        }

        glm::vec3 getWorldAxis(const Transform & world) const
        {
            return CollisionUtils::NormalizeSafe(world.getWorldNormal(m_localAxis));
        }

        float getHalfHeight() const
        {
            return m_halfHeight;
        }

        const AABB computeAABB(const Transform * world) override
        {
            glm::vec3 a;
            glm::vec3 b;
            getWorldSegment(*world, a, b);

            const float r = getWorldRadius(*world);

            AABB aabb;
            aabb.min = CollisionUtils::min(a, b) - glm::vec3(r, r, r);
            aabb.max = CollisionUtils::max(a, b) + glm::vec3(r, r, r);
            return aabb;
        }

        void getWorldSegment(const Transform & world, glm::vec3 & a, glm::vec3 & b) const
        {
            const glm::vec3 center = getWorldCenter(world);
            const glm::vec3 axis = getWorldAxis(world);
            const glm::vec3 offset = axis * m_halfHeight;

            a = center - offset;
            b = center + offset;
        }

        glm::mat3 computeLocalInverseInertiaTensor(float mass) const override
        {
            if (mass <= CollisionUtils::kEpsilon || m_radius <= CollisionUtils::kEpsilon)
                return glm::mat3(0.0f);

            const float cylinderHeight = std::max(0.0f, m_halfHeight * 2.0f);
            const float sphereVolume = (4.0f / 3.0f) * static_cast<float>(M_PI) * m_radius * m_radius * m_radius;
            const float cylinderVolume = static_cast<float>(M_PI) * m_radius * m_radius * cylinderHeight;
            const float totalVolume = sphereVolume + cylinderVolume;

            if (totalVolume <= CollisionUtils::kEpsilon)
                return glm::mat3(0.0f);

            const float cylinderMass = mass * (cylinderVolume / totalVolume);
            const float sphereMass = mass * (sphereVolume / totalVolume);
            const float sphereCenterOffset = m_halfHeight + (3.0f * m_radius / 8.0f);

            const float cylinderAxisInertia = 0.5f * cylinderMass * m_radius * m_radius;
            const float cylinderRadialInertia = (cylinderMass / 12.0f) * (3.0f * m_radius * m_radius + cylinderHeight * cylinderHeight);

            const float hemisphereMass = sphereMass * 0.5f;
            const float hemisphereCenterInertia = 0.4f * hemisphereMass * m_radius * m_radius;
            const float hemisphereRadialInertia = hemisphereCenterInertia + hemisphereMass * sphereCenterOffset * sphereCenterOffset;

            const float axisInertia = cylinderAxisInertia + (2.0f * hemisphereCenterInertia);
            const float radialInertia = cylinderRadialInertia + (2.0f * hemisphereRadialInertia);

            const glm::vec3 localAxis = CollisionUtils::NormalizeSafe(m_localAxis, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::vec3 helperAxis = std::abs(localAxis.y) < 0.999f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
            glm::vec3 tangent = CollisionUtils::NormalizeSafe(glm::cross(helperAxis, localAxis), glm::vec3(1.0f, 0.0f, 0.0f));
            glm::vec3 bitangent = CollisionUtils::NormalizeSafe(glm::cross(localAxis, tangent), glm::vec3(0.0f, 0.0f, 1.0f));

            glm::mat3 basis;
            basis[0] = tangent;
            basis[1] = localAxis;
            basis[2] = bitangent;

            glm::mat3 inverseDiagonal(
                radialInertia > CollisionUtils::kEpsilon ? 1.0f / radialInertia : 0.0f, 0.0f, 0.0f,
                0.0f, axisInertia > CollisionUtils::kEpsilon ? 1.0f / axisInertia : 0.0f, 0.0f,
                0.0f, 0.0f, radialInertia > CollisionUtils::kEpsilon ? 1.0f / radialInertia : 0.0f
            );

            return basis * inverseDiagonal * glm::transpose(basis);
        }
    };
}