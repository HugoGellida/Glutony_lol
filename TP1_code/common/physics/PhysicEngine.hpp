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
    static constexpr int kSolverIterations = 8; // nombre d'it du solver (plus grand => stabilise les collisions)
    static constexpr float kMaxStableDeltaTime = 0.032f; // si une frame a été mangée (genre 2/60 s pour faire une update) on divise l'update en petits bouts
    static constexpr float kFixedSubstepDeltaTime = 0.01f; // le pas pour couper notre deltaT.

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

    void StepSimulation(float dt);

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

    void Step(float dt);
};

namespace PhysicEngine
{
    static PhysicsSystem * __INSTANCE = nullptr;
    PhysicsSystem * getInstance();
}