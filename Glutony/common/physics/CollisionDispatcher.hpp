#pragma once
#include "../gameobject/Transform.hpp"
#include "SphereCollider.hpp"
#include "CapsuleCollider.hpp"
#include "BoxCollider.hpp"
#include "Collider.hpp"
#include "CollisionManifold.hpp"
#include "CollisionTest.hpp"
#include "PlaneCollider.hpp"
namespace physics
{
    class CollisionDispatcher
    {
    public:
        static CollisionManifold Test(const Collider & a, const Transform & ta, const Collider & b, const Transform & tb)
        {
            const ColliderType typeA = a.getType();
            const ColliderType typeB = b.getType();

            if (typeA == ColliderType::Sphere && typeB == ColliderType::Sphere)
                return CollisionTests::TestSphereSphere(
                    static_cast<const SphereCollider &>(a), ta,
                    static_cast<const SphereCollider &>(b), tb
                );
            if (typeA == ColliderType::Sphere && typeB == ColliderType::Plane)
                return CollisionTests::TestSpherePlane(
                    static_cast<const SphereCollider &>(a), ta,
                    static_cast<const PlaneCollider &>(b), tb
                );
            if (typeA == ColliderType::Plane && typeB == ColliderType::Sphere)
            {
                CollisionManifold manifold = CollisionTests::TestSpherePlane(
                    static_cast<const SphereCollider &>(b), tb,
                    static_cast<const PlaneCollider &>(a), ta
                );

                if (manifold.hasCollision)
                    manifold.Invert();

                return manifold;
            }
            if (typeA == ColliderType::Sphere && typeB == ColliderType::Capsule)
                return CollisionTests::TestSphereCapsule(
                    static_cast<const SphereCollider &>(a), ta,
                    static_cast<const CapsuleCollider &>(b), tb
                );
            if (typeA == ColliderType::Capsule && typeB == ColliderType::Sphere)
            {
                CollisionManifold manifold = CollisionTests::TestSphereCapsule(
                    static_cast<const SphereCollider &>(b), tb,
                    static_cast<const CapsuleCollider &>(a), ta
                );

                if (manifold.hasCollision)
                    manifold.Invert();

                return manifold;
            }

            if (typeA == ColliderType::Capsule && typeB == ColliderType::Plane)
                return CollisionTests::TestCapsulePlane(
                    static_cast<const CapsuleCollider &>(a), ta,
                    static_cast<const PlaneCollider &>(b), tb
                );

            if (typeA == ColliderType::Plane && typeB == ColliderType::Capsule)
            {
                CollisionManifold manifold = CollisionTests::TestCapsulePlane(
                    static_cast<const CapsuleCollider &>(b), tb,
                    static_cast<const PlaneCollider &>(a), ta
                );

                if (manifold.hasCollision)
                    manifold.Invert();

                return manifold;
            }

            if (typeA == ColliderType::Capsule && typeB == ColliderType::Capsule)
                return CollisionTests::TestCapsuleCapsule(
                    static_cast<const CapsuleCollider &>(a), ta,
                    static_cast<const CapsuleCollider &>(b), tb
                );

            if (typeA == ColliderType::Sphere && typeB == ColliderType::Box)
                return CollisionTests::TestSphereBox(
                    static_cast<const SphereCollider &>(a), ta,
                    static_cast<const BoxCollider &>(b), tb
                );

            if (typeA == ColliderType::Box && typeB == ColliderType::Sphere)
            {
                CollisionManifold manifold = CollisionTests::TestSphereBox(
                    static_cast<const SphereCollider &>(b), tb,
                    static_cast<const BoxCollider &>(a), ta
                );

                if (manifold.hasCollision)
                    manifold.Invert();

                return manifold;
            }

            if (typeA == ColliderType::Box && typeB == ColliderType::Plane)
                return CollisionTests::TestBoxPlane(
                    static_cast<const BoxCollider &>(a), ta,
                    static_cast<const PlaneCollider &>(b), tb
                );

            if (typeA == ColliderType::Plane && typeB == ColliderType::Box)
            {
                CollisionManifold manifold = CollisionTests::TestBoxPlane(
                    static_cast<const BoxCollider &>(b), tb,
                    static_cast<const PlaneCollider &>(a), ta
                );

                if (manifold.hasCollision)
                    manifold.Invert();

                return manifold;
            }

            if (typeA == ColliderType::Box && typeB == ColliderType::Box)
                return CollisionTests::TestBoxBox(
                    static_cast<const BoxCollider &>(a), ta,
                    static_cast<const BoxCollider &>(b), tb
                );
            if (typeA == ColliderType::Capsule && typeB == ColliderType::Box)
                return CollisionTests::TestCapsuleBox(
                    static_cast<const CapsuleCollider &>(a), ta,
                    static_cast<const BoxCollider &>(b), tb
                );

            if (typeA == ColliderType::Box && typeB == ColliderType::Capsule)
            {
                CollisionManifold manifold = CollisionTests::TestCapsuleBox(
                    static_cast<const CapsuleCollider &>(b), tb,
                    static_cast<const BoxCollider &>(a), ta
                );

                if (manifold.hasCollision)
                    manifold.Invert();

                return manifold;
            }
            

            return CollisionManifold{};
        }
    };
}
