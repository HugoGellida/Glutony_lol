#include "PhysicEngine.hpp"

void PhysicsSystem::IntegrateVelocities(PhysicBody & body, float dt)
{
    physics::RigidBody& rb = *body.rb;
    
    if (!rb.isDynamic())
        return;
    
    glm::vec3 totalForce = rb.accumulatedForce;

    if (rb.useGravity)
        totalForce += gravity * rb.mass;
    
    const glm::vec3 acceleration = totalForce * rb.inverseMass;
    rb.m_linearVelocity += acceleration * dt;
    
    // damping simple
    if (rb.linearDamping > 0.0f)
    {
        const float dampingFactor = std::max(0.0f, 1.0f - rb.linearDamping * dt);
        rb.m_linearVelocity *= dampingFactor;
    }

    // freeze axes sur la vitesse
    if (rb.freezePositionX) rb.m_linearVelocity.x = 0.0f;
    if (rb.freezePositionY) rb.m_linearVelocity.y = 0.0f;
    if (rb.freezePositionZ) rb.m_linearVelocity.z = 0.0f;    
}

void PhysicsSystem::IntegratePositions(PhysicBody & body, float dt)
{
    physics::RigidBody & rb = *body.rb;

    if (!rb.isDynamic())
        return;
    rb.m_position += rb.m_linearVelocity * dt;
}

void PhysicsSystem::ApplyPositionConstraints(physics::RigidBody * rb)
{
    // faudrait une reference si on veut un vrai freeze. Pour le moment, geler la velocité fait l'affaire.
}

void PhysicsSystem::SyncTransformFromRigidbody(PhysicBody & body)
{
    body.transform->setPosition(body.rb->m_position);
}

bool PhysicsSystem::CanBodiesCollide(const PhysicBody & a, const PhysicBody & b) const
{
    const physics::RigidBody& rbA = *a.rb;
    const physics::RigidBody& rbB = *b.rb;

    if ((rbA.isStatic || rbA.isKinematic) && (rbB.isStatic || rbB.isKinematic))
        return false;
    return true;
}

PhysicsSystem * PhysicEngine::getInstance()
{
    if (__INSTANCE == nullptr)
        __INSTANCE = new PhysicsSystem();
    return __INSTANCE;
}