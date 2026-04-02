#pragma once

#include "../gameobject/Transform.hpp"
#include "Collider.hpp"
#include "RigidBody.hpp"
#include "AABB.hpp"
#include "RigidBody.hpp"
#include "CollisionManifold.hpp"
#include "CollisionDispatcher.hpp"
#include "CollisionSolver.hpp"
#include <algorithm>
#include "glm/glm.hpp"
#include <vector>

struct PhysicBody
{
    Transform * transform = nullptr;
    physics::RigidBody * rb = nullptr;
    physics::Collider * collider = nullptr;

    bool isValid() const
    {
        return rb != nullptr && collider != nullptr && transform != nullptr;
    }
};




class PhysicsSystem
{
public:
    glm::vec3 gravity{0.0f, -9.81f, 0.0f};

private:
    static constexpr int kSolverIterations = 20;

    struct BroadPhaseEntry
    {
        PhysicBody body;
        physics::AABB aabb;
    };

    struct CandidatePair
    {
        size_t a = 0;
        size_t b = 0;
    };

    std::vector<PhysicBody> m_bodies;

    void IntegrateVelocities(PhysicBody & body, float dt);
    
    void IntegratePositions(PhysicBody & body, float dt);
    
    void ApplyPositionConstraints(physics::RigidBody * rb);

    void ApplySleepThresholds(physics::RigidBody & rb);

    void SyncTransformFromRigidbody(PhysicBody & body);
    
    bool CanBodiesCollide(const PhysicBody & a, const PhysicBody & b) const;
    
public:
    PhysicsSystem()
    {

    }
    void AddBody(PhysicBody pb)
    {
        if (!pb.isValid())
            return;
        m_bodies.push_back(pb);
    }
    void RemoveBody(PhysicBody body)
    {
        const auto it = std::find_if(m_bodies.begin(), m_bodies.end(), [&](const PhysicBody& candidate) {
            return candidate.collider == body.collider
                && candidate.rb == body.rb
                && candidate.transform == body.transform;
        });

        if (it == m_bodies.end())
            return;

        m_bodies.erase(it);
    }

    void Step(float dt)
    {
        if (dt <= 0.0f)
            return;
        
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

            body.transform->setPosition(body.rb->m_position);
            body.transform->setOrientation(body.rb->m_orientation);

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

                bodyA.transform->setPosition(bodyA.rb->m_position);
                bodyB.transform->setPosition(bodyB.rb->m_position);
                bodyA.transform->setOrientation(bodyA.rb->m_orientation);
                bodyB.transform->setOrientation(bodyB.rb->m_orientation);

                physics::CollisionManifold manifold = physics::CollisionDispatcher::Test(
                    *bodyA.collider, *bodyA.transform,
                    *bodyB.collider, *bodyB.transform
                );

                if (!manifold.hasCollision)
                    continue;

                physics::CollisionSolver::Resolve(*bodyA.rb, *bodyB.rb, manifold, applyPositionalCorrection);

                ApplyPositionConstraints(bodyA.rb);
                ApplyPositionConstraints(bodyB.rb);

                bodyA.transform->setPosition(bodyA.rb->m_position);
                bodyB.transform->setPosition(bodyB.rb->m_position);
                bodyA.transform->setOrientation(bodyA.rb->m_orientation);
                bodyB.transform->setOrientation(bodyB.rb->m_orientation);
            }
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
};

namespace PhysicEngine
{
    static PhysicsSystem * __INSTANCE = nullptr;
    PhysicsSystem * getInstance();
}