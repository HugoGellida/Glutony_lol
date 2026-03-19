#pragma once
#include "../gameobject/Transform.hpp"
#include "AABB.hpp"
#include "../gameobject/component/Component.hpp"

namespace physics
{
    enum ColliderType
    {
        Sphere,
        Box,
        Capsule,
        Plane
    };

    class Collider : public component::Component
    {
    public:
        Collider()
        {

        }
        void run()
        {

        }
        ~Collider() 
        {

        }
        virtual const ColliderType getType() const = 0;
        virtual const AABB computeAABB(const Transform * world) = 0;
    };
}