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
    struct BroadPhaseEntry
    {
        PhysicBody body;
        physics::AABB aabb;
    };

    std::vector<PhysicBody> m_bodies;

    void IntegrateVelocities(PhysicBody & body, float dt);
    
    void IntegratePositions(PhysicBody & body, float dt);
    
    void ApplyPositionConstraints(physics::RigidBody * rb);

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
        std::vector<PhysicBody> n(m_bodies.size()-1);
        size_t a = 0;
        for (size_t i = 0; i < m_bodies.size(); i++)
        {
            if (m_bodies[i].collider == body.collider && m_bodies[i].rb == body.rb && m_bodies[i].transform == body.transform)
                continue;
            n[a++] = m_bodies[i];
        }
        m_bodies = n;
    }

    void Step(float dt)
    {
        if (dt <= 0.0f)
            return;
        
            for (PhysicBody body : m_bodies)
            {
                if (!body.isValid())
                    continue;
                IntegrateVelocities(body, dt);                
            }

            for (PhysicBody body : m_bodies)
            {
                if (!body.isValid())
                    continue;
                IntegratePositions(body, dt);
                ApplyPositionConstraints(body.rb);
            }

            std::vector<BroadPhaseEntry> broadPhaseEntries;
            
            for (PhysicBody body : m_bodies)
            {
                if (!body.isValid())
                    continue;

                // rb as truth
                body.transform->setPosition(body.rb->m_position);
                
                BroadPhaseEntry entry;
                entry.body = body;
                entry.aabb = body.collider->computeAABB(body.transform);
                broadPhaseEntries.push_back(entry);
            }

            const size_t count = broadPhaseEntries.size();

            for (size_t i = 0; i < count; ++i)
            {
                for (size_t j = i+1; j < count; ++j)
                {
                    PhysicBody & bodyA = broadPhaseEntries[i].body;
                    PhysicBody & bodyB = broadPhaseEntries[j].body;
                    if (!CanBodiesCollide(bodyA, bodyB)) // BroadPHASE
                        continue;
                    // TODO - utiliser l'algo sweep and prune !!! (check si meilleurs que mes 6 bools)
                    if (!broadPhaseEntries[i].aabb.Overlaps(broadPhaseEntries[j].aabb))
                        continue;
                    
                    physics::CollisionManifold manifold = physics::CollisionDispatcher::Test(
                        *bodyA.collider, *bodyA.transform,
                        *bodyB.collider, *bodyB.transform
                    );

                    if (!manifold.hasCollision)
                        continue;
                    
                    physics::CollisionSolver::Resolve(*bodyA.rb, *bodyB.rb, manifold);

                    ApplyPositionConstraints(bodyA.rb);
                    ApplyPositionConstraints(bodyB.rb);

                    bodyA.transform->setPosition(bodyA.rb->m_position);
                    bodyB.transform->setPosition(bodyB.rb->m_position);
                }
            }

            for (PhysicBody body : m_bodies)
            {
                if (!body.isValid())
                    continue;
                
                SyncTransformFromRigidbody(body);

                body.rb->accumulatedForce = glm::vec3(0.f, 0.f, 0.f);
            }
    }
};

namespace PhysicEngine
{
    static PhysicsSystem * __INSTANCE = nullptr;
    PhysicsSystem * getInstance();
}