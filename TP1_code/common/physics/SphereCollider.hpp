#pragma once
#include "Collider.hpp"
#include "../gameobject/component/ComponentSerialization.hpp"
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

        static const component_meta::ComponentDescriptor& componentDescriptor()
        {
            static const component_meta::ComponentDescriptor descriptor = []() {
                component_meta::ComponentDescriptor value;
                value.typeKey = "physics.sphere_collider";
                value.displayName = "Sphere Collider";
                value.version = 1;
                value.factory = []() -> component::Component* { return new SphereCollider(); };
                value.fields = {
                    {
                        "local_center",
                        "Center",
                        component_meta::FieldKind::Vec3,
                        [](const component::Component& component) -> component_meta::SerializedValue {
                            return static_cast<const SphereCollider&>(component).m_localCenter;
                        },
                        [](component::Component& component, const component_meta::SerializedValue& value) -> bool {
                            const glm::vec3* parsed = std::get_if<glm::vec3>(&value);
                            if (parsed == nullptr)
                                return false;

                            static_cast<SphereCollider&>(component).m_localCenter = *parsed;
                            return true;
                        },
                        {}
                    },
                    {
                        "radius",
                        "Radius",
                        component_meta::FieldKind::Float,
                        [](const component::Component& component) -> component_meta::SerializedValue {
                            return static_cast<const SphereCollider&>(component).m_radius;
                        },
                        [](component::Component& component, const component_meta::SerializedValue& value) -> bool {
                            const float* parsed = std::get_if<float>(&value);
                            if (parsed == nullptr)
                                return false;

                            static_cast<SphereCollider&>(component).m_radius = *parsed;
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
            const glm::vec3 worldScale = world.getWorldScale();
            const float maxScale = std::max(std::abs(worldScale.x), std::max(std::abs(worldScale.y), std::abs(worldScale.z)));
            return m_radius * maxScale;
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