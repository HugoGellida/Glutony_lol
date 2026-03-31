#pragma once

#include "Collider.hpp"
#include "../gameobject/Transform.hpp"
#include "glm/glm.hpp"
#include "AABB.hpp"
#include "CollisionUtils.hpp"
#include <cmath>

namespace physics
{
    class BoxCollider : public Collider
    {
    private:
        glm::vec3 m_localCenter{0.0f, 0.0f, 0.0f};
        glm::vec3 m_halfExtents{0.5f, 0.5f, 0.5f};
        glm::vec3 m_localScale{1.0f, 1.0f, 1.0f};
    public:
        BoxCollider() : Collider()
        {

        }
        BoxCollider(glm::vec3 localCenter, glm::vec3 halfExtents, glm::vec3 localScale)
        {
            m_localCenter = localCenter;
            m_halfExtents = halfExtents;
            m_localScale = localScale;
        }

        const ColliderType getType() const override
        {
            return ColliderType::Box;
        }

        const AABB computeAABB(const Transform * world) override
        {
            const glm::vec3 center = getWorldCenter(*world);
            const glm::mat3 orientation = getWorldOrientation(*world);
            const glm::vec3 extents = getWorldHalfExtents(*world);

            const glm::mat3 absR(
                std::abs(orientation[0][0]), std::abs(orientation[0][1]), std::abs(orientation[0][2]),
                std::abs(orientation[1][0]), std::abs(orientation[1][1]), std::abs(orientation[1][2]),
                std::abs(orientation[2][0]), std::abs(orientation[2][1]), std::abs(orientation[2][2])
            );

            const glm::vec3 worldExtents = absR * extents;

            AABB aabb;
            aabb.min = center - worldExtents;
            aabb.max = center + worldExtents;
            return aabb;
        }

        const glm::vec3 getWorldCenter(const Transform & world) const
        {
            return world.getWorldPos(m_localCenter);
        }

        glm::mat3 getWorldOrientation(const Transform & world) const
        {
            const glm::mat4 normalMat4 = world.getNormalMat();
            glm::mat3 orientation(normalMat4);

            orientation[0] = CollisionUtils::NormalizeSafe(orientation[0], glm::vec3(1.0f, 0.0f, 0.0f));
            orientation[1] = CollisionUtils::NormalizeSafe(orientation[1], glm::vec3(0.0f, 1.0f, 0.0f));
            orientation[2] = CollisionUtils::NormalizeSafe(orientation[2], glm::vec3(0.0f, 0.0f, 1.0f));

            return orientation;
        }

        glm::vec3 getWorldHalfExtents(const Transform & world) const
        {
            const glm::vec3 worldScale = world.getWorldScale();
            return glm::vec3(
                std::abs(m_halfExtents.x * m_localScale.x * worldScale.x),
                std::abs(m_halfExtents.y * m_localScale.y * worldScale.y),
                std::abs(m_halfExtents.z * m_localScale.z * worldScale.z)
            );
        }

        void getVertices(const Transform & world, glm::vec3 outVertices[8]) const
        {
            const glm::vec3 center = getWorldCenter(world);
            const glm::mat3 rot = getWorldOrientation(world);
            const glm::vec3 e = getWorldHalfExtents(world);

            const glm::vec3 ax = rot[0] * e.x;
            const glm::vec3 ay = rot[1] * e.y;
            const glm::vec3 az = rot[2] * e.z;
            
            outVertices[0] = center - ax - ay - az;
            outVertices[1] = center + ax - ay - az;
            outVertices[2] = center - ax + ay - az;
            outVertices[3] = center + ax + ay - az;
            outVertices[4] = center - ax - ay + az;
            outVertices[5] = center + ax - ay + az;
            outVertices[6] = center - ax + ay + az;
            outVertices[7] = center + ax + ay + az;
        }

        inline static glm::vec3 ClosestPointOnOBB(
            const glm::vec3 & point,
            const BoxCollider & box,
            const Transform & tb
        )
        {
            const glm::vec3 center = box.getWorldCenter(tb);
            const glm::mat3 rot = box.getWorldOrientation(tb);
            const glm::vec3 ext = box.getWorldHalfExtents(tb);

            glm::vec3 d = point - center;
            glm::vec3 q = center;

            for (int i = 0; i < 3; ++i)
            {
                float dist = CollisionUtils::Dot(d, rot[i]);
                dist = CollisionUtils::Clamp(dist, -ext[i], ext[i]);
                q += rot[i] * dist;
            }

            return q;
        }

        glm::mat3 computeLocalInverseInertiaTensor(float mass) const override
        {
            if (mass <= CollisionUtils::kEpsilon)
                return glm::mat3(0.0f);

            const glm::vec3 halfExtents(
                std::abs(m_halfExtents.x * m_localScale.x),
                std::abs(m_halfExtents.y * m_localScale.y),
                std::abs(m_halfExtents.z * m_localScale.z)
            );
            const glm::vec3 size = halfExtents * 2.0f;

            const float inertiaX = (mass / 12.0f) * ((size.y * size.y) + (size.z * size.z));
            const float inertiaY = (mass / 12.0f) * ((size.x * size.x) + (size.z * size.z));
            const float inertiaZ = (mass / 12.0f) * ((size.x * size.x) + (size.y * size.y));

            return glm::mat3(
                inertiaX > CollisionUtils::kEpsilon ? 1.0f / inertiaX : 0.0f, 0.0f, 0.0f,
                0.0f, inertiaY > CollisionUtils::kEpsilon ? 1.0f / inertiaY : 0.0f, 0.0f,
                0.0f, 0.0f, inertiaZ > CollisionUtils::kEpsilon ? 1.0f / inertiaZ : 0.0f
            );
        }

        inline static glm::vec3 ToOBBLocalPoint(
            const glm::vec3 & worldPoint,
            const BoxCollider & box,
            const Transform & tb
        )
        {
            const glm::vec3 center = box.getWorldCenter(tb);
            const glm::mat3 rot = box.getWorldOrientation(tb);
            const glm::vec3 d = worldPoint - center;

            return glm::vec3(
                CollisionUtils::Dot(d, rot[0]),
                CollisionUtils::Dot(d, rot[1]),
                CollisionUtils::Dot(d, rot[2])
            );
        }

        inline static glm::vec3 FromOBBLocalPoint(
            const glm::vec3 & localPoint,
            const BoxCollider & box,
            const Transform & tb
        )
        {
            const glm::vec3 center = box.getWorldCenter(tb);
            const glm::mat3 rot = box.getWorldOrientation(tb);

            return center + rot[0] * localPoint.x + rot[1] * localPoint.y + rot[2] * localPoint.z;
        }
    };
}