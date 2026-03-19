#pragma once
#include "Collider.hpp"
#include "glm/glm.hpp"
#include "AABB.hpp"
#include "CollisionUtils.hpp"
#include <cmath>

namespace physics
{
    class SphereCollider : public Collider
    {
    private:
    public:
        glm::vec3 m_localCenter{0.0f, 0.0f, 0.0f};
        float m_radius = 0.5f;
        
        SphereCollider() : Collider()
        {

        }

        SphereCollider(glm::vec3 localCenter, float radius) : Collider()
        {
            m_localCenter = localCenter;
            m_radius = radius;
        }

        const ColliderType getType() const override
        {
            return ColliderType::Sphere;
        }

        const AABB computeAABB(const Transform * world) override
        {
            const glm::vec3 center = getWorldCenter(*world);
            const float r = getWorldRadius(*world);

            AABB aabb;
            aabb.min = center - glm::vec3(r, r, r);
            aabb.max = center + glm::vec3(r, r , r);
            return aabb;
        }

        const glm::vec3 getWorldCenter(const Transform & world) const
        {
            return world.getWorldPos(m_localCenter);
        }

        float getWorldRadius(const Transform & world) const
        {
            return m_radius;
        }

        glm::mat3 computeLocalInverseInertiaTensor(float mass) const override
        {
            constexpr float epsilon = 1e-6f;

            if (mass <= epsilon || m_radius <= epsilon)
                return glm::mat3(0.0f);

            const float inertia = 0.4f * mass * m_radius * m_radius;
            if (inertia <= epsilon)
                return glm::mat3(0.0f);

            const float inverseInertia = 1.0f / inertia;
            return glm::mat3(
                inverseInertia, 0.0f, 0.0f,
                0.0f, inverseInertia, 0.0f,
                0.0f, 0.0f, inverseInertia
            );
        }
    };
}