#include "PhysicEngine.hpp"
#include <cmath>

namespace
{
    constexpr float kLinearSleepSpeed = 2e-4f;
    constexpr float kAngularSleepSpeed = 5e-4f;
    constexpr float kRestingTangentSpeed = 2e-3f;
    constexpr float kRestingAngularSpeed = 1e-2f;
}

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

    if (rb.enableAngularDynamics)
    {
        const glm::vec3 angularAcceleration = rb.GetInverseInertiaTensorWorld() * rb.accumulatedTorque;
        rb.m_angularVelocity += angularAcceleration * dt;
    }
    else
    {
        rb.m_angularVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
    }
    
    // damping simple
    if (rb.linearDamping > 0.0f)
    {
        const float dampingFactor = std::max(0.0f, 1.0f - rb.linearDamping * dt);
        rb.m_linearVelocity *= dampingFactor;
    }
    if (rb.enableAngularDynamics && rb.angularDamping > 0.0f)
    {
        const float dampingFactor = std::max(0.0f, 1.0f - rb.angularDamping * dt);
        rb.m_angularVelocity *= dampingFactor;
    }

    rb.ApplyMotionConstraints();
}

void PhysicsSystem::IntegratePositions(PhysicBody & body, float dt)
{
    physics::RigidBody & rb = *body.rb;

    if (!rb.isDynamic())
        return;
    rb.m_position += rb.m_linearVelocity * dt;
    if (rb.enableAngularDynamics)
    {
        const float angularSpeed = std::sqrt(physics::CollisionUtils::LengthSq(rb.m_angularVelocity));
        if (angularSpeed > physics::CollisionUtils::kEpsilon)
        {
            const glm::vec3 axis = rb.m_angularVelocity / angularSpeed;
            const glm::quat deltaRotation = glm::angleAxis(angularSpeed * dt, axis);
            rb.m_orientation = glm::normalize(deltaRotation * rb.m_orientation);
            rb.m_rotation = glm::degrees(glm::eulerAngles(rb.m_orientation));
        }
    }
    rb.ApplyMotionConstraints();
}

void PhysicsSystem::ApplyPositionConstraints(physics::RigidBody * rb)
{
    if (rb == nullptr)
        return;

    rb->ApplyMotionConstraints();
}

void PhysicsSystem::ApplySleepThresholds(physics::RigidBody & rb)
{
    if (physics::CollisionUtils::LengthSq(rb.m_linearVelocity) <= (kLinearSleepSpeed * kLinearSleepSpeed))
        rb.m_linearVelocity = glm::vec3(0.0f, 0.0f, 0.0f);

    if (physics::CollisionUtils::LengthSq(rb.m_angularVelocity) <= (kAngularSleepSpeed * kAngularSleepSpeed))
        rb.m_angularVelocity = glm::vec3(0.0f, 0.0f, 0.0f);

    if (!rb.m_hadContact)
        return;

    const glm::vec3 contactNormal = rb.GetAverageContactNormal();
    const float normalSpeed = physics::CollisionUtils::Dot(rb.m_linearVelocity, contactNormal);
    const glm::vec3 tangentialVelocity = rb.m_linearVelocity - contactNormal * normalSpeed;

    if (physics::CollisionUtils::LengthSq(tangentialVelocity) <= (kRestingTangentSpeed * kRestingTangentSpeed)
        && physics::CollisionUtils::LengthSq(rb.m_angularVelocity) <= (kRestingAngularSpeed * kRestingAngularSpeed))
    {
        rb.m_linearVelocity -= tangentialVelocity;
    }
}

void PhysicsSystem::SyncTransformFromRigidbody(PhysicBody & body)
{
    body.transform->setPosition(body.rb->m_position);
    body.transform->setOrientation(body.rb->m_orientation);
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