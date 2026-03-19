#pragma once

#include <cmath>
#include "glm/glm.hpp"

namespace physics
{
    namespace CollisionUtils
    {
        constexpr float kEpsilon = 1e-6f;

        inline float LengthSq(const glm::vec3 & v)
        {
            return (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
        }

        inline float Length(const glm::vec3 & v)
        {
            return std::sqrt(LengthSq(v));
        }

        inline glm::vec3 NormalizeSafe(const glm::vec3 & v, const glm::vec3 & fallback = glm::vec3(0, 1, 0))
        {
            const float lenSq = LengthSq(v);
            if (lenSq <= kEpsilon)
                return fallback;
            
            const float invLen = 1.0f / std::sqrt(lenSq);
            return v * invLen;
        }

        inline float Dot(const glm::vec3 &a, const glm::vec3 & b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        inline glm::vec3 min(glm::vec3 a, glm::vec3 b)
        {
            return glm::vec3(std::min(a.x, b.x),std::min(a.y, b.y),std::min(a.z, b.z));
        }

        inline glm::vec3 max(glm::vec3 a, glm::vec3 b)
        {
            return glm::vec3(std::max(a.x, b.x),std::max(a.y, b.y),std::max(a.z, b.z));
        }

        inline float Clamp01(float t)
        {
            if (t < 0.0f) return 0.0f;
            if (t > 1.0f) return 1.0f;
            return t;
        }

        inline glm::vec3 ClosestPointOnSegment(
            const glm::vec3 & p,
            const glm::vec3 & a,
            const glm::vec3 & b
        )
        {
            const glm::vec3 ab = b - a;
            const float abLenSq = LengthSq(ab);

            if (abLenSq <= CollisionUtils::kEpsilon)
                return a;

            const float t = Clamp01(Dot(p - a, ab) / abLenSq);
            return a + ab * t;
        }

        inline void ClosestPointsSegmentSegment(
            const glm::vec3 & p1,
            const glm::vec3 & q1,
            const glm::vec3 & p2,
            const glm::vec3 & q2,
            glm::vec3 & c1,
            glm::vec3 & c2
        )
        {
            const glm::vec3 d1 = q1 - p1;
            const glm::vec3 d2 = q2 - p2;
            const glm::vec3 r = p1 - p2;

            const float a = CollisionUtils::Dot(d1, d1);
            const float e = CollisionUtils::Dot(d2, d2);
            const float f = CollisionUtils::Dot(d2, r);

            float s = 0.0f;
            float t = 0.0f;

            if (a <= kEpsilon && e <= kEpsilon)
            {
                c1 = p1;
                c2 = p2;
                return;
            }
            
            if (a <= kEpsilon)
            {
                s = 0.0f;
                t = Clamp01(f / e);
            }
            else
            {
                const float c = Dot(d1, r);

                if (e <= kEpsilon)
                {
                    t = 0.0f;
                    s = Clamp01(-c / a);
                }
                else
                {
                    const float b = Dot(d1, d2);
                    const float denom = a * e - b * b;

                    if (denom > kEpsilon)
                        s = Clamp01((b * f - c * e) / denom);
                    else
                        s = 0.0f;
                    
                    t = (b * s + f) / e;

                    if (t < 0.0f)
                    {
                        t = 0.0f;
                        s = Clamp01(-c / a);
                    }
                    else if (t > 1.0f)
                    {
                        t = 1.0f;
                        s = Clamp01((b - c) / a);
                    }
                }
            }

            c1 = p1 + d1 * s;
            c2 = p2 + d2 * t;
        }

        inline float Clamp(float v, float minV, float maxV)
        {
            if (v < minV) return minV;
            if (v > maxV) return maxV;
            return v;
        }

        inline float ClampRange(float value, float minValue, float maxValue)
        {
            if (value < minValue) return minValue;
            if (value > maxValue) return maxValue;
            return value;
        }

        inline bool IntersectSegmentAABBLocal(
            const glm::vec3 & p0,
            const glm::vec3 & p1,
            const glm::vec3 & halfExtents,
            float & tEnter
        )
        {
            const glm::vec3 d = p1 - p0;

            float tMin = 0.0f;
            float tMax = 1.0f;

            for (int i = 0; i < 3; ++i)
            {
                if (std::abs(d[i]) <= CollisionUtils::kEpsilon)
                {
                    if (p0[i] < -halfExtents[i] || p0[i] > halfExtents[i])
                        return false;
                    continue;
                }

                const float invD = 1.0f / d[i];
                float t1 = (-halfExtents[i] - p0[i]) * invD;
                float t2 = ( halfExtents[i] - p0[i]) * invD;

                if (t1 > t2)
                {
                    const float tmp = t1;
                    t1 = t2;
                    t2 = tmp;
                }

                if (t1 > tMin) tMin = t1;
                if (t2 < tMax) tMax = t2;

                if (tMin > tMax)
                    return false;
            }

            tEnter = tMin;
            return true;
        }
    }
}