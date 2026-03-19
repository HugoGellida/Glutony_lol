#include "RigidBody.hpp"
#include "PhysicEngine.hpp"


void physics::RigidBody::run()
{
    if (registered)
        return;

    // Register using the transform's current world position so Scene setup order
    // cannot leave the rigid body with a stale initial position.
    m_position = m_parent -> transform.getWorldPos(glm::vec3(0.0f, 0.0f, 0.0f));
    Collider * c = m_parent -> getComponent<Collider>();
    if (c != nullptr)
    {
        PhysicBody pb;
        pb.collider = c;
        pb.rb = this;
        pb.transform = &(m_parent -> transform);

        PhysicEngine::getInstance() -> AddBody(pb);
        registered = true;
        return;
    }
}

void physics::RigidBody::RecomputeInverseMass()
{
    if (isStatic || isKinematic || mass <= 0.0f)
        inverseMass = 0.0f;
    else
        inverseMass = 1.0f / mass;
}

bool physics::RigidBody::isDynamic() const
{
    return !isStatic && !isKinematic && inverseMass > 0.0f;
}

void physics::RigidBody::AddForce(glm::vec3 force)
{
    accumulatedForce+=force;
}

void physics::RigidBody::Impulse(glm::vec3 force)
{
    m_linearVelocity+=force * inverseMass;
}

void physics::RigidBody::Teleport(glm::vec3 newPos)
{
    m_position = newPos;
}

void physics::RigidBody::SetVelocity(glm::vec3 v)
{
    m_linearVelocity = v;
}

physics::RigidBody::~RigidBody()
{
    if (registered)
    {
        Collider * c = m_parent -> getComponent<Collider>();
        if (c != nullptr)
        {
            PhysicBody pb;
            pb.collider = c;
            pb.rb = this;
            pb.transform = &(m_parent -> transform);

            PhysicEngine::getInstance() -> RemoveBody(pb);
        }
    }
}