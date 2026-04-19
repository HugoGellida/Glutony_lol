#pragma once
#include "glm/glm.hpp"
#include <vector>

namespace physics
{
    struct ContactPoint
    {
        glm::vec3 position;
        float penetration;
    };

    struct CollisionManifold
    {
        bool hasCollision = false;
        // convention : de A vers B
        glm::vec3 normal{0.0f, 0.0f, 0.0f};
        float penetration = 0.0f;
        std::vector<ContactPoint> contacts;

        void Reset()
        {
            hasCollision = false;
            normal = glm::vec3(0.0f, 0.0f, 0.0f);
            penetration = 0.0f;
            contacts.clear();
        }

        void Invert()
        {
            normal = -normal;
        }
    };
}