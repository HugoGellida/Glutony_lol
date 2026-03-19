#pragma once
#include "glm/glm.hpp"

namespace physics
{
    struct AABB
    {
        glm::vec3 min{0.0f, 0.0f, 0.0f};
        glm::vec3 max{0.0f, 0.0f, 0.0f};

        bool Overlaps(const AABB & other) const
        {
            if (max.x < other.min.x || min.x > other.max.x) return false;
            if (max.y < other.min.y || min.y > other.max.y) return false;
            if (max.z < other.min.z || min.z > other.max.z) return false;
            return true;
        }
    };

    
}