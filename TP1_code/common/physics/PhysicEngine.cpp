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

void PhysicsSystem::StepSimulation(float dt)
{
    for (PhysicBody & body : m_bodies)
    {
        if (!body.isValid())
            continue;
        body.rb->BeginSimulationStep();
        IntegrateVelocities(body, dt);
    }

    for (PhysicBody & body : m_bodies)
    {
        if (!body.isValid())
            continue;
        IntegratePositions(body, dt);
        ApplyPositionConstraints(body.rb);
    }

    std::vector<BroadPhaseEntry> broadPhaseEntries;
    broadPhaseEntries.reserve(m_bodies.size());

    for (PhysicBody & body : m_bodies)
    {
        if (!body.isValid())
            continue;

        body.transform->setWorldPosition(body.rb->m_position);
        body.transform->setWorldOrientation(body.rb->m_orientation);

        BroadPhaseEntry entry;
        entry.body = body;
        entry.aabb = body.collider->computeAABB(body.transform);
        broadPhaseEntries.push_back(entry);
    }

    std::vector<CandidatePair> candidatePairs;
    const size_t count = broadPhaseEntries.size();
    candidatePairs.reserve((count * (count - 1)) / 2);

    for (size_t i = 0; i < count; ++i)
    {
        for (size_t j = i + 1; j < count; ++j)
        {
            PhysicBody & bodyA = broadPhaseEntries[i].body;
            PhysicBody & bodyB = broadPhaseEntries[j].body;
            if (!CanBodiesCollide(bodyA, bodyB))
                continue;
            if (!broadPhaseEntries[i].aabb.Overlaps(broadPhaseEntries[j].aabb))
                continue;

            CandidatePair pair;
            pair.a = i;
            pair.b = j;
            candidatePairs.push_back(pair);
        }
    }

    for (int iteration = 0; iteration < kSolverIterations; ++iteration)
    {
        const bool applyPositionalCorrection = (iteration == 0);

        for (const CandidatePair & pair : candidatePairs)
        {
            PhysicBody & bodyA = broadPhaseEntries[pair.a].body;
            PhysicBody & bodyB = broadPhaseEntries[pair.b].body;

            if (!bodyA.isValid() || !bodyB.isValid())
                continue;

            bodyA.transform->setWorldPosition(bodyA.rb->m_position);
            bodyB.transform->setWorldPosition(bodyB.rb->m_position);
            bodyA.transform->setWorldOrientation(bodyA.rb->m_orientation);
            bodyB.transform->setWorldOrientation(bodyB.rb->m_orientation);

            physics::CollisionManifold manifold = physics::CollisionDispatcher::Test(
                *bodyA.collider, *bodyA.transform,
                *bodyB.collider, *bodyB.transform
            );

            if (!manifold.hasCollision)
                continue;

            physics::CollisionSolver::Resolve(*bodyA.rb, *bodyB.rb, manifold, applyPositionalCorrection);

            ApplyPositionConstraints(bodyA.rb);
            ApplyPositionConstraints(bodyB.rb);

            bodyA.transform->setWorldPosition(bodyA.rb->m_position);
            bodyB.transform->setWorldPosition(bodyB.rb->m_position);
            bodyA.transform->setWorldOrientation(bodyA.rb->m_orientation);
            bodyB.transform->setWorldOrientation(bodyB.rb->m_orientation);
        }
    }
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
    body.transform->setWorldPosition(body.rb->m_position);
    body.transform->setWorldOrientation(body.rb->m_orientation);
}

bool PhysicsSystem::CanBodiesCollide(const PhysicBody & a, const PhysicBody & b) const
{
    const physics::RigidBody& rbA = *a.rb;
    const physics::RigidBody& rbB = *b.rb;

    if ((rbA.isStatic || rbA.isKinematic) && (rbB.isStatic || rbB.isKinematic))
        return false;
    return true;
}

void PhysicsSystem::Step(float dt)
{
    if (dt <= 0.0f)
        return;

    if (dt > kMaxStableDeltaTime)
    {
        const int fixedStepCount = static_cast<int>(dt / kFixedSubstepDeltaTime);
        const float remainingDt = dt - (static_cast<float>(fixedStepCount) * kFixedSubstepDeltaTime);

        for (int stepIndex = 0; stepIndex < fixedStepCount; ++stepIndex)
            StepSimulation(kFixedSubstepDeltaTime);

        if (remainingDt > 0.0f)
            StepSimulation(remainingDt);
    }
    else
    {
        StepSimulation(dt);
    }

    for (PhysicBody & body : m_bodies)
    {
        if (!body.isValid())
            continue;

        ApplySleepThresholds(*body.rb);
        SyncTransformFromRigidbody(body);
        body.rb->accumulatedForce = glm::vec3(0.f, 0.f, 0.f);
        body.rb->accumulatedTorque = glm::vec3(0.f, 0.f, 0.f);
    }
}

PhysicsSystem * PhysicEngine::getInstance()
{
    if (__INSTANCE == nullptr)
        __INSTANCE = new PhysicsSystem();
    return __INSTANCE;
}