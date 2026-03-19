#pragma once
#include "glm/glm.hpp"
#include "../gameobject/GameObject.hpp"
#include "../gameobject/component/Component.hpp"
#include "SphereCollider.hpp"

namespace physics
{
    class RigidBody : public Component
    {
    private:
        GameObject * m_parent = nullptr;
    public:
        glm::vec3 accumulatedForce = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 m_position{0.0f, 0.0f, 0.0f};
        glm::vec3 m_linearVelocity{0.0f, 0.0f, 0.0f};
        float linearDamping = 0.0f;
        bool useGravity = true;

        float mass = 1.0f;
        float inverseMass = 1.0f;

        bool isStatic = false;
        bool isKinematic = false;

        bool freezePositionX = false;
        bool freezePositionY = false;
        bool freezePositionZ = false;

        float restitution = 0.0f;
        bool registered = false;


        RigidBody(GameObject * parent) : Component()
        {
            m_parent = parent;
            m_position = m_parent -> transform.getWorldPos(glm::vec3(0, 0, 0));
        }

        void run();

        void RecomputeInverseMass();

        bool isDynamic() const;
        
        /// @brief add a force to the object (like the influence of a field, like gravity or other)
        void AddForce(glm::vec3 force);

        /// @brief add an impulse to an object (act instantly, ignoring mass)
        void Impulse(glm::vec3 force);

        /// @brief Move the object to somewhere else 
        void Teleport(glm::vec3 newPos);

        /// @brief set the object current velocity
        void SetVelocity(glm::vec3 v);

        ~RigidBody(); // TOCHECK
        
    };
}