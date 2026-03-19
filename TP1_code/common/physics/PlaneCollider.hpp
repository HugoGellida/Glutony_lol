#pragma once

#include "Collider.hpp"
#include "../gameobject/Transform.hpp"
#include "glm/glm.hpp"
#include "AABB.hpp"
#include "CollisionUtils.hpp"

namespace physics
{
    class PlaneCollider : public Collider
    {
    private:
        glm::vec3 m_localOrigin;
        glm::vec3 m_localNormal;
    public:
        PlaneCollider(glm::vec3 localOrigin, glm::vec3 localNormal) : Collider()
        {
            m_localOrigin = localOrigin;
            m_localNormal = localNormal;
        }
        
        const ColliderType getType() const override
        {
            return ColliderType::Plane;
        }

        const AABB computeAABB(const Transform * world) override
        {
            // for infinite plan but should use smaller AABB !
            constexpr float kHuge = 1e6f;

            AABB aabb;
            aabb.min = glm::vec3(-kHuge, -kHuge, -kHuge);
            aabb.max = glm::vec3(kHuge, kHuge, kHuge);
            return aabb;
        }
        
        glm::vec3 getWorldOrigin(const Transform & world) const
        {
            return world.getWorldPos(m_localOrigin);
        }

        glm::vec3 getWorldNormal(const Transform & world) const
        {
            return world.getWorldNormal(m_localNormal);
        }

        float SignedDistance(const glm::vec3 & worldPoint, const Transform & world) const
        {
            const glm::vec3 planeOrigin = getWorldOrigin(world);
            const glm::vec3 planeNormal = getWorldNormal(world);


            return CollisionUtils::Dot(worldPoint - planeOrigin, planeNormal);
        }
    };
}