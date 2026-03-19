#pragma once
#include "CollisionManifold.hpp"
#include "SphereCollider.hpp"
#include "PlaneCollider.hpp"
#include "CapsuleCollider.hpp"
#include "../gameobject/Transform.hpp"
#include "CollisionUtils.hpp"
#include "BoxCollider.hpp"

namespace physics
{
    namespace CollisionTests
    {

        inline CollisionManifold TestSphereSphere(
            const physics::SphereCollider & a, const Transform & ta,
            const physics::SphereCollider & b, const Transform & tb
        )
        {
            CollisionManifold manifold{};
            const glm::vec3 centerA = a.getWorldCenter(ta);
            const glm::vec3 centerB = b.getWorldCenter(tb);

            const float radiusA = a.getWorldRadius(ta);
            const float radiusB = b.getWorldRadius(tb);

            const glm::vec3 delta = centerB - centerA;
            const float distSq = CollisionUtils::LengthSq(delta);
            const float radiusSum = radiusA + radiusB;
            const float radiussumSq = radiusSum * radiusSum;

            if (distSq > radiussumSq)
                return manifold;
            manifold.hasCollision = true;
            const float distance = CollisionUtils::Length(delta);

            if (distance > CollisionUtils::kEpsilon)
            {
                manifold.normal = delta / distance;
                manifold.penetration = radiusSum - distance;

                ContactPoint cp;
                cp.position = centerA + manifold.normal * (radiusA - manifold.penetration * 0.5f);
                cp.penetration = manifold.penetration;
                manifold.contacts.push_back(cp);
            }
            else
            {
                // centre confondu : normale arbitraire mais stable.
                manifold.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                manifold.penetration = radiusSum;

                ContactPoint cp;
                cp.position = centerA;
                cp.penetration = manifold.penetration;
                manifold.contacts.push_back(cp);
            }

            return manifold;
        }

        inline CollisionManifold TestSpherePlane(
            const SphereCollider & sphere, const Transform & ts,
            const PlaneCollider & plane, const Transform & tp
        )
        {
            CollisionManifold manifold{};

            const glm::vec3 sphereCenter = sphere.getWorldCenter(ts);
            const float sphereRadius = sphere.getWorldRadius(ts);

            const glm::vec3 planeOrigin = plane.getWorldOrigin(tp);
            const glm::vec3 planeNormal = plane.getWorldNormal(tp);

            const float signedDistance = CollisionUtils::Dot(sphereCenter - planeOrigin, planeNormal); // already available on planeCollider. Check if can be removed

            const float backPointDistance = signedDistance - sphereRadius;

            if (backPointDistance >= 0.0f)
                return manifold;
            
            manifold.hasCollision = true;
            manifold.normal = -planeNormal;
            manifold.penetration = -backPointDistance;

            ContactPoint cp;
            cp.position = sphereCenter - planeNormal * sphereRadius;
            cp.penetration = manifold.penetration;
            manifold.contacts.push_back(cp);
            return manifold;
        }

        inline CollisionManifold TestSphereCapsule(
            const SphereCollider & sphere, const Transform & ts,
            const CapsuleCollider & capsule, const Transform & tc
        )
        {
            CollisionManifold manifold{};

            const glm::vec3 sphereCenter = sphere.getWorldCenter(ts);
            const float sphereRadius = sphere.getWorldRadius(ts);

            glm::vec3 segA;
            glm::vec3 segB;
            capsule.getWorldSegment(tc, segA, segB);

            const glm::vec3 closest = CollisionUtils::ClosestPointOnSegment(sphereCenter, segA, segB);
            glm::vec3 delta = closest - sphereCenter;

            const float capsuleRadius = capsule.getWorldRadius(tc);
            const float radiusSum = sphereRadius + capsuleRadius;
            const float distSq = CollisionUtils::LengthSq(delta);

            if (distSq > radiusSum * radiusSum)
                return manifold;
            
            manifold.hasCollision = true;

            const float distance = CollisionUtils::Length(delta);
            if (distance > CollisionUtils::kEpsilon)
            {
                manifold.normal = delta / distance;
                manifold.penetration = radiusSum - distance;
            }
            else
            {
                manifold.normal = capsule.getWorldAxis(tc);
                manifold.penetration = radiusSum;
            }
            
            ContactPoint cp;
            cp.position = sphereCenter + manifold.normal;
            cp.penetration = manifold.penetration;
            manifold.contacts.push_back(cp);
            
            return manifold;
        }

        inline CollisionManifold TestCapsulePlane(
            const CapsuleCollider & capsule, const Transform & tc,
            const PlaneCollider & plane, const Transform & tp
        )
        {
            CollisionManifold manifold{};

            glm::vec3 segA;
            glm::vec3 segB;
            capsule.getWorldSegment(tc, segA, segB);

            const float capsuleRadius = capsule.getWorldRadius(tc);
            const glm::vec3 planeNormal = plane.getWorldNormal(tp);

            const float distA = plane.SignedDistance(segA, tp);
            const float distB = plane.SignedDistance(segB, tp);

            glm::vec3 deepestPoint = segA;
            float minDistance = distA;

            if (distB < distA)
            {
                minDistance = distB;
                deepestPoint = segB;
            }

            const float backPointDistance = minDistance - capsuleRadius;
            if (backPointDistance >= 0.0f)
                return manifold;

            manifold.hasCollision = true;
            manifold.normal = -planeNormal;
            manifold.penetration = -backPointDistance;

            ContactPoint cp;
            cp.position = deepestPoint - planeNormal * capsuleRadius;
            cp.penetration = manifold.penetration;
            manifold.contacts.push_back(cp);

            return manifold;
        }

        inline CollisionManifold TestCapsuleCapsule(
            const CapsuleCollider & a, const Transform & ta,
            const CapsuleCollider & b, const Transform & tb
        )
        {
            CollisionManifold manifold{};

            glm::vec3 a0;
            glm::vec3 a1;
            glm::vec3 b0;
            glm::vec3 b1;

            a.getWorldSegment(ta, a0, a1);
            b.getWorldSegment(tb, b0, b1);

            glm::vec3 closestA;
            glm::vec3 closestB;
            CollisionUtils::ClosestPointsSegmentSegment(a0, a1, b0, b1, closestA, closestB);

            glm::vec3 delta = closestB - closestA;
            const float distSq = CollisionUtils::LengthSq(delta);

            const float radiusA = a.getWorldRadius(ta);
            const float radiusB = b.getWorldRadius(tb);
            const float radiusSum = radiusA + radiusB;

            if (distSq > radiusSum * radiusSum)
                return manifold;

            manifold.hasCollision = true;

            const float distance = CollisionUtils::Length(delta);
            if (distance > CollisionUtils::kEpsilon)
            {
                manifold.normal = delta / distance;
                manifold.penetration = radiusSum - distance;
            }
            else
            {
                const glm::vec3 centerA = a.getWorldCenter(ta);
                const glm::vec3 centerB = b.getWorldCenter(tb);
                manifold.normal = CollisionUtils::NormalizeSafe(centerB - centerA);
                manifold.penetration = radiusSum;
            }

            const glm::vec3 surfaceA = closestA + manifold.normal * radiusA;
            const glm::vec3 surfaceB = closestB - manifold.normal * radiusB;

            ContactPoint cp;
            cp.position = (surfaceA + surfaceB) * 0.5f;
            cp.penetration = manifold.penetration;
            manifold.contacts.push_back(cp);
            
            return manifold;
        }
        

        inline CollisionManifold TestSphereBox(
            const SphereCollider & sphere, const Transform & ts,
            const BoxCollider & box, const Transform & tb
        )
        {
            CollisionManifold manifold{};

            const glm::vec3 sphereCenter = sphere.getWorldCenter(ts);
            const float sphereRadius = sphere.getWorldRadius(ts);

            const glm::vec3 closest = BoxCollider::ClosestPointOnOBB(sphereCenter, box, tb);
            glm::vec3 delta = closest - sphereCenter;

            const float distSq = CollisionUtils::LengthSq(delta);
            if (distSq > sphereRadius * sphereRadius)
                return manifold;

            manifold.hasCollision = true;

            const float distance = CollisionUtils::Length(delta);
            if (distance > CollisionUtils::kEpsilon)
            {
                manifold.normal = delta / distance;
                manifold.penetration = sphereRadius - distance;
            }
            else
            {
                manifold.normal = CollisionUtils::NormalizeSafe(box.getWorldCenter(tb) - sphereCenter);
                manifold.penetration = sphereRadius;
            }

            ContactPoint cp;
            cp.position = closest;
            cp.penetration = manifold.penetration;
            manifold.contacts.push_back(cp);

            return manifold;
        }

        inline CollisionManifold TestBoxPlane(
            const BoxCollider & box, const Transform & tb,
            const PlaneCollider & plane, const Transform & tp
        )
        {
            CollisionManifold manifold{};

            const glm::vec3 center = box.getWorldCenter(tb);
            const glm::vec3 ext = box.getWorldHalfExtents(tb);
            const glm::mat3 rot = box.getWorldOrientation(tb);
            const glm::vec3 planeNormal = plane.getWorldNormal(tp);

            const float centerDistance = plane.SignedDistance(center, tp);

            const float projectedRadius =
                std::abs(CollisionUtils::Dot(planeNormal, rot[0])) * ext.x +
                std::abs(CollisionUtils::Dot(planeNormal, rot[1])) * ext.y +
                std::abs(CollisionUtils::Dot(planeNormal, rot[2])) * ext.z;

            const float backPointDistance = centerDistance - projectedRadius;
            if (backPointDistance >= 0.0f)
                return manifold;

            manifold.hasCollision = true;
            manifold.normal = -planeNormal;
            manifold.penetration = -backPointDistance;

            ContactPoint cp;
            cp.position = center - planeNormal * projectedRadius;
            cp.penetration = manifold.penetration;
            manifold.contacts.push_back(cp);

            return manifold;
        }

        inline CollisionManifold TestBoxBox(
            const BoxCollider & a, const Transform & ta,
            const BoxCollider & b, const Transform & tb
        )
        {
            CollisionManifold manifold{};

            const glm::vec3 centerA = a.getWorldCenter(ta);
            const glm::vec3 centerB = b.getWorldCenter(tb);

            const glm::vec3 extA = a.getWorldHalfExtents(ta);
            const glm::vec3 extB = b.getWorldHalfExtents(tb);

            const glm::mat3 rotA = a.getWorldOrientation(ta);
            const glm::mat3 rotB = b.getWorldOrientation(tb);

            glm::vec3 axes[15];
            axes[0] = rotA[0];
            axes[1] = rotA[1];
            axes[2] = rotA[2];
            axes[3] = rotB[0];
            axes[4] = rotB[1];
            axes[5] = rotB[2];

            int axisCount = 6;
            for (int i = 0; i < 3; ++i)
            {
                for (int j = 0; j < 3; ++j)
                {
                    glm::vec3 axis = glm::cross(rotA[i], rotB[j]);
                    if (CollisionUtils::LengthSq(axis) > CollisionUtils::kEpsilon)
                        axes[axisCount++] = CollisionUtils::NormalizeSafe(axis);
                }
            }

            const glm::vec3 centerDelta = centerB - centerA;

            float bestOverlap = 1e30f;
            glm::vec3 bestAxis(0.0f, 1.0f, 0.0f);

            for (int k = 0; k < axisCount; ++k)
            {
                const glm::vec3 axis = axes[k];

                const float dist = std::abs(CollisionUtils::Dot(centerDelta, axis));

                const float projA =
                    std::abs(CollisionUtils::Dot(rotA[0], axis)) * extA.x +
                    std::abs(CollisionUtils::Dot(rotA[1], axis)) * extA.y +
                    std::abs(CollisionUtils::Dot(rotA[2], axis)) * extA.z;

                const float projB =
                    std::abs(CollisionUtils::Dot(rotB[0], axis)) * extB.x +
                    std::abs(CollisionUtils::Dot(rotB[1], axis)) * extB.y +
                    std::abs(CollisionUtils::Dot(rotB[2], axis)) * extB.z;

                const float overlap = projA + projB - dist;
                if (overlap < 0.0f)
                    return manifold;

                if (overlap < bestOverlap)
                {
                    bestOverlap = overlap;
                    bestAxis = axis;
                }
            }

            manifold.hasCollision = true;
            manifold.normal = CollisionUtils::Dot(centerDelta, bestAxis) >= 0.0f ? bestAxis : -bestAxis;
            manifold.penetration = bestOverlap;

            ContactPoint cp;
            cp.position = (centerA + centerB) * 0.5f;
            cp.penetration = manifold.penetration;
            manifold.contacts.push_back(cp);

            return manifold;
        }
        inline CollisionManifold TestCapsuleBox(
            const CapsuleCollider & capsule, const Transform & tc,
            const BoxCollider & box, const Transform & tb
        )
        {
            CollisionManifold manifold{};

            glm::vec3 segA;
            glm::vec3 segB;
            capsule.getWorldSegment(tc, segA, segB);

            const float capsuleRadius = capsule.getWorldRadius(tc);
            const glm::vec3 boxExtents = box.getWorldHalfExtents(tb);

            const glm::vec3 localA = BoxCollider::ToOBBLocalPoint(segA, box, tb);
            const glm::vec3 localB = BoxCollider::ToOBBLocalPoint(segB, box, tb);

            float tEnter = 0.0f;
            if (!CollisionUtils::IntersectSegmentAABBLocal(localA, localB, boxExtents + glm::vec3(capsuleRadius), tEnter))
                return manifold;

            const glm::vec3 localSegPoint = localA + (localB - localA) * tEnter;

            glm::vec3 localBoxPoint(
                CollisionUtils::ClampRange(localSegPoint.x, -boxExtents.x, boxExtents.x),
                CollisionUtils::ClampRange(localSegPoint.y, -boxExtents.y, boxExtents.y),
                CollisionUtils::ClampRange(localSegPoint.z, -boxExtents.z, boxExtents.z)
            );

            glm::vec3 deltaLocal = localBoxPoint - localSegPoint;
            const float distSq = CollisionUtils::LengthSq(deltaLocal);

            manifold.hasCollision = true;

            if (distSq > CollisionUtils::kEpsilon)
            {
                const glm::vec3 worldSegPoint = BoxCollider::FromOBBLocalPoint(localSegPoint, box, tb);
                const glm::vec3 worldBoxPoint = BoxCollider::FromOBBLocalPoint(localBoxPoint, box, tb);
                const glm::vec3 deltaWorld = worldBoxPoint - worldSegPoint;

                const float distance = CollisionUtils::Length(deltaWorld);
                manifold.normal = deltaWorld / distance;
                manifold.penetration = capsuleRadius - distance;

                ContactPoint cp;
                cp.position = worldBoxPoint;
                cp.penetration = manifold.penetration;
                manifold.contacts.push_back(cp);

                return manifold;
            }

            int bestAxis = 0;
            float bestDistance = boxExtents.x - std::abs(localSegPoint.x);

            for (int i = 1; i < 3; ++i)
            {
                const float candidate = boxExtents[i] - std::abs(localSegPoint[i]);
                if (candidate < bestDistance)
                {
                    bestDistance = candidate;
                    bestAxis = i;
                }
            }

            glm::vec3 localNormal(0.0f, 0.0f, 0.0f);
            localNormal[bestAxis] = (localSegPoint[bestAxis] >= 0.0f) ? 1.0f : -1.0f;

            glm::vec3 localSurfacePoint = localSegPoint;
            localSurfacePoint[bestAxis] = localNormal[bestAxis] * boxExtents[bestAxis];
            localSurfacePoint[(bestAxis + 1) % 3] =
                CollisionUtils::ClampRange(localSurfacePoint[(bestAxis + 1) % 3],
                        -boxExtents[(bestAxis + 1) % 3],
                            boxExtents[(bestAxis + 1) % 3]);
            localSurfacePoint[(bestAxis + 2) % 3] =
                CollisionUtils::ClampRange(localSurfacePoint[(bestAxis + 2) % 3],
                        -boxExtents[(bestAxis + 2) % 3],
                            boxExtents[(bestAxis + 2) % 3]);

            const glm::mat3 rot = box.getWorldOrientation(tb);
            manifold.normal = rot[bestAxis] * localNormal[bestAxis];
            manifold.penetration = capsuleRadius + bestDistance;

            ContactPoint cp;
            cp.position = BoxCollider::FromOBBLocalPoint(localSurfacePoint, box, tb);
            cp.penetration = manifold.penetration;
            manifold.contacts.push_back(cp);

            return manifold;
        }
    }
}