#pragma once

#include "Collider.hpp"
#include "../gameobject/Transform.hpp"
#include "glm/glm.hpp"
#include "AABB.hpp"
#include "CollisionUtils.hpp"
#include "../gameobject/component/ComponentSerialization.hpp"

namespace physics
{
    class PlaneCollider : public Collider
    {
    private:
        glm::vec3 m_localOrigin;
        glm::vec3 m_localNormal;
    public:
        PlaneCollider() : Collider()
        {
            m_localOrigin = glm::vec3(0, 0, 0);
            m_localNormal = glm::vec3(0, 1, 0);
        }
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

        glm::mat3 computeLocalInverseInertiaTensor(float mass) const override
        {
            return glm::mat3(0.0f);
        }



        static const component_meta::ComponentDescriptor& componentDescriptor()
        {
            static const component_meta::ComponentDescriptor descriptor = []()
            {
                component_meta::ComponentDescriptor value;
                value.typeKey = "physics.planeCollider";
                value.displayName = "Plane Collider";
                value.version = 1;
                value.factory = []() -> component::Component* {return new PlaneCollider();};
                value.fields = {
                    {
                        "planeOrigin",
                        "Plane Origin",
                        component_meta::FieldKind::Vec3,
                        [](const component::Component& component) -> component_meta::SerializedValue {
                            return static_cast<const PlaneCollider&>(component).m_localOrigin;
                        },
                        [](component::Component& component, const component_meta::SerializedValue& value) -> bool {
                            const glm::vec3* parsed = std::get_if<glm::vec3>(&value);
                            if (parsed == nullptr)
                                return false;
                            
                            static_cast<PlaneCollider&>(component).m_localOrigin = *parsed;
                            return true;
                        },
                        {}
                    },
                    {
                        "planeNormal",
                        "Plane Normal",
                        component_meta::FieldKind::Vec3,
                        [](const component::Component& component) -> component_meta::SerializedValue {
                            return static_cast<const PlaneCollider&>(component).m_localNormal;
                        },
                        [](component::Component& component, const component_meta::SerializedValue& value) -> bool {
                            const glm::vec3* parsed = std::get_if<glm::vec3>(&value);
                            if (parsed == nullptr)
                                return false;
                            
                            static_cast<PlaneCollider&>(component).m_localNormal = *parsed;
                            return true;
                        },
                        {}
                    }
                };
                return value;
            }();
            static const bool registered = []() {
                component_meta::registerComponentDescriptor(descriptor);
                return true;
            }();
            (void)registered;
            return descriptor;
        }
        const component_meta::ComponentDescriptor* getComponentDescriptor() const override
        {
            return &componentDescriptor();
        }
    };
}