#pragma once

#include "Collider.hpp"
#include "../gameobject/Transform.hpp"
#include "glm/glm.hpp"
#include "AABB.hpp"
#include "CollisionUtils.hpp"

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
    };
}