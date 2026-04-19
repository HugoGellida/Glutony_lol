#pragma once

#include "common/Camera.hpp"
#include "common/physics/AABB.hpp"
#include "common/gameobject/Transform.hpp"


#include <algorithm>
#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>


namespace Raycast
{
    inline glm::vec2 WindowSpaceToScreenSpace(double x, double y, int viewportX, int viewportY, int viewportWidth, int viewportHeight)
    {
        if (viewportWidth <= 0 || viewportHeight <= 0)
            return glm::vec2(0.0f);

        const glm::vec2 screenCoords(
            static_cast<float>((x - static_cast<double>(viewportX)) / static_cast<double>(viewportWidth)),
            static_cast<float>((y - static_cast<double>(viewportY)) / static_cast<double>(viewportHeight))
        );

        return glm::clamp(screenCoords, glm::vec2(0.0f), glm::vec2(1.0f));
    }

    inline glm::vec3 ScreenSpaceToWorldSpace(const Camera& camera, glm::vec2 screenCoords)
    {
        const glm::vec2 clampedCoords = glm::clamp(screenCoords, glm::vec2(0.0f), glm::vec2(1.0f));
        const float tanHalfFov = tan(glm::radians(camera.m_fov) * 0.5f);

        const glm::vec3 rayCameraSpace = glm::normalize(glm::vec3(
            (2.0f * clampedCoords.x - 1.0f) * tanHalfFov * camera.m_aspectRatio,
            (1.0f - 2.0f * clampedCoords.y) * tanHalfFov,
            -1.0f
        ));

        return glm::normalize(glm::vec3(
            Transform::rotationMatrix(camera.m_orientation) * glm::vec4(rayCameraSpace, 0.0f)
        ));
    }

    struct Ray 
    {
        glm::vec3 o;
        glm::vec3 d;
    };

    inline Ray getRayFromClick(int viewportX, int viewportY, int viewportWidth, int viewportHeight, double sceneClickX, double sceneClickY, const Camera& m_camera)
    {
        glm::vec2 screenCoords = Raycast::WindowSpaceToScreenSpace(
            sceneClickX,
            sceneClickY,
            viewportX,
            viewportY,
            viewportWidth,
            viewportHeight
        );

        glm::vec3 rayOrigin = m_camera.m_position;
        glm::vec3 rayDirection = Raycast::ScreenSpaceToWorldSpace(m_camera, screenCoords);

        struct Ray ray;
        ray.o = rayOrigin;
        ray.d = rayDirection;
        return ray;
    }

    

    inline bool raycastAABB(const physics::AABB& aabb, const Ray& ray, float* t)
    {
        float tMin = 0.0f;
        float tMax = std::numeric_limits<float>::max();

        for (int axis = 0; axis < 3; ++axis)
        {
            const float origin = ray.o[axis];
            const float direction = ray.d[axis];
            const float minBound = aabb.min[axis];
            const float maxBound = aabb.max[axis];

            if (std::abs(direction) < 1e-6f)
            {
                if (origin < minBound || origin > maxBound)
                    return false;

                continue;
            }

            float axisTMin = (minBound - origin) / direction;
            float axisTMax = (maxBound - origin) / direction;

            if (axisTMin > axisTMax)
                std::swap(axisTMin, axisTMax);

            tMin = std::max(tMin, axisTMin);
            tMax = std::min(tMax, axisTMax);

            if (tMin > tMax)
                return false;
        }

        if (t != nullptr)
            *t = tMin;

        return true;
    }

    inline bool raycastTransformedAABB(const Transform& transform, const physics::AABB& aabb, const Ray& ray, float* t)
    {
        return raycastAABB(transform.applyToAABB(aabb), ray, t);
    }

    inline bool raycastPlane(const Ray& ray, const glm::vec3& planePoint, const glm::vec3& planeNormal, glm::vec3* hitPoint, float* t)
    {
        const glm::vec3 normalizedNormal = glm::normalize(planeNormal);
        const float denominator = glm::dot(normalizedNormal, ray.d);
        if (std::abs(denominator) < 1e-6f)
            return false;

        const float distance = glm::dot(planePoint - ray.o, normalizedNormal) / denominator;
        if (distance < 0.0f)
            return false;

        if (t != nullptr)
            *t = distance;

        if (hitPoint != nullptr)
            *hitPoint = ray.o + ray.d * distance;

        return true;
    }

    inline bool raycastOBB(const glm::vec3& center, const glm::mat3& orientation, const glm::vec3& halfExtents, const Ray& ray, float* t)
    {
        const glm::mat3 inverseOrientation = glm::transpose(orientation);
        Ray localRay;
        localRay.o = inverseOrientation * (ray.o - center);
        localRay.d = inverseOrientation * ray.d;

        physics::AABB localBounds;
        localBounds.min = -halfExtents;
        localBounds.max = halfExtents;
        return raycastAABB(localBounds, localRay, t);
    }
}
