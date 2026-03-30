#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "../gameobject/GameObject.hpp"
#include "../gameobject/component/Component.hpp"
#include "SphereCollider.hpp"
#include "../gameobject/component/ComponentSerialization.hpp"

namespace physics
{
    class RigidBody : public Component
    {
    private:
        GameObject * m_parent = nullptr;
    public:
        glm::vec3 accumulatedForce = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 accumulatedTorque = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 m_accumulatedContactNormal{0.0f, 0.0f, 0.0f};
        glm::vec3 m_previousPosition{0.0f, 0.0f, 0.0f};
        glm::vec3 m_previousRotation{0.0f, 0.0f, 0.0f};
        glm::quat m_previousOrientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 m_position{0.0f, 0.0f, 0.0f};
        glm::vec3 m_rotation{0.0f, 0.0f, 0.0f};
        glm::quat m_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 m_linearVelocity{0.0f, 0.0f, 0.0f};
        glm::vec3 m_angularVelocity{0.0f, 0.0f, 0.0f};
        float linearDamping = 0.0f;
        float angularDamping = 0.0f;
        bool useGravity = true;
        bool enableAngularDynamics = true;

        float mass = 1.0f;
        float inverseMass = 1.0f;
        bool useAutomaticInertiaTensor = true;
        glm::mat3 inverseInertiaTensorLocal{0.0f};
        glm::mat3 inverseInertiaTensorWorld{0.0f};

        bool isStatic = false;
        bool isKinematic = false;

        bool freezePositionX = false;
        bool freezePositionY = false;
        bool freezePositionZ = false;
        bool freezeRotationX = false;
        bool freezeRotationY = false;
        bool freezeRotationZ = false;

        float bounciness = 0.0f;
        float staticFriction = 0.6f;
        float dynamicFriction = 0.4f;
        bool m_hadContact = false;
        bool registered = false;


        RigidBody(GameObject * parent) : Component()
        {
            m_parent = parent;
            m_position = m_parent -> transform.getWorldPos(glm::vec3(0, 0, 0));
            m_rotation = m_parent -> transform.getRotation();
            m_orientation = m_parent -> transform.getOrientation();
            m_previousPosition = m_position;
            m_previousRotation = m_rotation;
            m_previousOrientation = m_orientation;
        }

        void run();

        void BeginSimulationStep();

        void RecomputeInverseMass();

        void RecomputeMassProperties();

        void UpdateDerivedData();

        void RegisterContact(const glm::vec3 & contactNormal);

        glm::vec3 GetAverageContactNormal() const;

        void ApplyMotionConstraints();

        bool isDynamic() const;
        
        /// @brief add a force to the object (like the influence of a field, like gravity or other)
        void AddForce(glm::vec3 force);

        /// @brief add a force at a world-space point
        void AddForceAtPosition(const glm::vec3 & force, const glm::vec3 & worldPoint);

        /// @brief add a torque to the object
        void AddTorque(glm::vec3 torque);

        /// @brief add a torque expressed in local rigidbody space
        void AddRelativeTorque(const glm::vec3 & localTorque);

        /// @brief add an impulse to an object (act instantly, ignoring mass)
        void Impulse(glm::vec3 force);

        /// @brief add an impulse at a world-space point
        void ApplyImpulseAtWorldPoint(const glm::vec3 & impulse, const glm::vec3 & worldPoint);

        glm::vec3 GetVelocityAtWorldPoint(const glm::vec3 & worldPoint) const;

        void SetInverseInertiaTensorLocal(const glm::mat3 & inverseTensor);

        const glm::mat3 & GetInverseInertiaTensorWorld() const;

        /// @brief Move the object to somewhere else 
        void Teleport(glm::vec3 newPos);

        void SetRotation(glm::vec3 eulerDegrees);

        /// @brief set the object current velocity
        void SetVelocity(glm::vec3 v);

        void SetAngularVelocity(glm::vec3 v);

        static const component_meta::ComponentDescriptor& componentDescriptor()
        {
            static const component_meta::ComponentDescriptor descriptor = []()
            {
                component_meta::ComponentDescriptor value;
                value.typeKey = "physics.rigidbody";
                value.displayName = "RigidBody";
                value.version = 1;
                value.factory = []() -> component::Component* {return new RigidBody(nullptr);};
                value.fields = {
                    {
                        "position",
                        "Position",
                        component_meta::FieldKind::Vec3,
                        [](const component::Component& component) -> component_meta::SerializedValue {
                            return static_cast<const RigidBody&>(component).m_position;
                        },
                        [](component::Component& component, const component_meta::SerializedValue& value) -> bool {
                            const glm::vec3* parsed = std::get_if<glm::vec3>(&value);
                            if (parsed == nullptr)
                                return false;
                            
                            static_cast<RigidBody&>(component).m_position = *parsed;
                            return true;
                        },
                        {}
                    },
                    {
                        "rotation",
                        "Rotation",
                        component_meta::FieldKind::Vec3,
                        [](const component::Component& component) -> component_meta::SerializedValue {
                            return static_cast<const RigidBody&>(component).m_rotation;
                        },
                        [](component::Component& component, const component_meta::SerializedValue& value) -> bool {
                            const glm::vec3* parsed = std::get_if<glm::vec3>(&value);
                            if (parsed == nullptr)
                                return false;

                            static_cast<RigidBody&>(component).m_rotation = *parsed;
                            return true;
                        },
                        {}
                    },
                    {
                        "velocity",
                        "Velocity",
                        component_meta::FieldKind::Vec3,
                        [](const component::Component& component) -> component_meta::SerializedValue {
                            return static_cast<const RigidBody&>(component).m_linearVelocity;
                        },
                        [](component::Component& component, const component_meta::SerializedValue& value) -> bool {
                            const glm::vec3* parsed = std::get_if<glm::vec3>(&value);
                            if (parsed == nullptr)
                                return false;
                            
                            static_cast<RigidBody&>(component).m_linearVelocity = *parsed;
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

        const component_meta::ComponentDescriptor*  getComponentDescriptor() const override
        {
            return &componentDescriptor();
        }

        ~RigidBody(); // TOCHECK
        
    };
}