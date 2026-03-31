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

    inline Ray getRayFromClick(bool editorMode, int m_uiViewportX, int m_uiViewportY, int m_uiViewportWidth, int m_uiViewportHeight, int sceneClickX, int sceneClickY, Camera& m_camera, GLFWwindow* window)
    {
        int viewportX = 0;
        int viewportY = 0;
        int viewportWidth = 0;
        int viewportHeight = 0;

        if (editorMode)
        {
            viewportX = m_uiViewportX;
            viewportY = m_uiViewportY;
            viewportWidth = m_uiViewportWidth;
            viewportHeight = m_uiViewportHeight;
        }
        else
        {
            glfwGetWindowSize(window, &viewportWidth, &viewportHeight);
        }

        const bool insideViewport =
            sceneClickX >= viewportX &&
            sceneClickX < viewportX + viewportWidth &&
            sceneClickY >= viewportY &&
            sceneClickY < viewportY + viewportHeight;

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
}