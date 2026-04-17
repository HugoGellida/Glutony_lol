#pragma once

#include <cmath>
#include <glm/glm.hpp>
#include <string>
#include "Component.hpp"
#include "../../physics/AABB.hpp"

namespace component
{
    class Mesh : public Component
    {
    private:
        void updateAABB() const
        {
            physics::AABB res = physics::AABB();
            res.min = glm::vec3(MAXFLOAT, MAXFLOAT, MAXFLOAT);
            res.max = glm::vec3(-MAXFLOAT, -MAXFLOAT, -MAXFLOAT);
            for (size_t i = 0; i < m_vStride; i++)
                for (size_t j = 0; j < 3; j++)
                {
                    if (m_vertices[i*3+j] > res.max[j])
                        res.max[j] = m_vertices[i*3+j];
                    if (m_vertices[i*3+j] < res.min[j])
                        res.min[j] = m_vertices[i*3+j];
                }
            
            m_AABB = res;
            m_boundsDirty = false;
        }

        void ensureBoundsUpToDate() const
        {
            if (!m_boundsDirty)
                return;

            updateAABB();
        }


    protected:
        float * m_vertices = nullptr;
        float * m_normals = nullptr;
        float * m_colors = nullptr;
        float * m_uvs = nullptr;
        float * m_tangents = nullptr;
        uint * m_triangles = nullptr;
        uint m_vStride = 0;
        uint m_tStride = 0;
        bool m_hasNormals = false;
        bool m_hasColors = false;
        bool m_hasUVs = false;
        bool m_hasTangents = false;
        bool m_tangentsDirty = false;
        bool m_onGPU = false;
        mutable physics::AABB m_AABB = physics::AABB();
        mutable bool m_boundsDirty = true;
        std::string m_assetPath;

        void invalidateTangents()
        {
            m_hasTangents = false;
            m_tangentsDirty = m_hasNormals && m_hasUVs;
        }

        static glm::vec3 fallbackTangentForNormal(const glm::vec3& normal)
        {
            const glm::vec3 referenceAxis = std::abs(normal.y) < 0.999f
                ? glm::vec3(0.0f, 1.0f, 0.0f)
                : glm::vec3(1.0f, 0.0f, 0.0f);
            const glm::vec3 tangent = glm::cross(referenceAxis, normal);
            return glm::length(tangent) > 0.0f ? glm::normalize(tangent) : glm::vec3(1.0f, 0.0f, 0.0f);
        }
    public:
        Mesh() : Component() {}
        Mesh(uint vStride, uint tStride, bool hasNormals = false, bool hasColors = false, bool hasUVs = false) : Component()
        {
            m_vStride = vStride;
            m_vertices = new float[vStride * 3]();
            m_tStride = tStride;
            m_triangles = new uint[tStride * 3]();
            
            m_hasNormals = hasNormals;
            if (hasNormals)
                m_normals = new float[vStride * 3]();
            
            m_hasColors = hasColors;
            if (hasColors)
                m_colors = new float[vStride * 3]();

            m_hasUVs = hasUVs;
            if (hasUVs)
                m_uvs = new float[vStride * 2]();
            m_tangentsDirty = hasNormals && hasUVs;
            updateAABB();
        }
        void setVertice(uint i, glm::vec3 pos, glm::vec3 normal = glm::vec3(), glm::vec3 color = glm::vec3(), glm::vec2 uv = glm::vec2())
        {
            m_vertices[i * 3] = pos.x;
            m_vertices[i * 3 + 1] = pos.y;
            m_vertices[i * 3 + 2] = pos.z;

            if (m_hasNormals) {
                m_normals[i * 3] = normal.x;
                m_normals[i * 3 + 1] = normal.y;
                m_normals[i * 3 + 2] = normal.z;
            }
            if (m_hasColors) {
                m_colors[i * 3] = color.x;
                m_colors[i * 3 + 1] = color.y;
                m_colors[i * 3 + 2] = color.z;
            }
            if (m_hasUVs) {
                m_uvs[i * 2] = uv.x;
                m_uvs[i * 2 + 1] = uv.y;
            }
            invalidateTangents();
            m_onGPU = false;
            m_boundsDirty = true;
        }
        void setTriangle(uint start, uint t1, uint t2, uint t3)
        {
            m_triangles[start * 3] = t1;
            m_triangles[start * 3 + 1] = t2;
            m_triangles[start * 3 + 2] = t3;
            invalidateTangents();
            m_onGPU = false;
        }

        void replaceGeometryFrom(Mesh& replacement)
        {
            if (&replacement == this)
                return;

            delete[] m_vertices;
            delete[] m_normals;
            delete[] m_colors;
            delete[] m_uvs;
            delete[] m_tangents;
            delete[] m_triangles;

            m_vertices = replacement.m_vertices;
            m_normals = replacement.m_normals;
            m_colors = replacement.m_colors;
            m_uvs = replacement.m_uvs;
            m_tangents = replacement.m_tangents;
            m_triangles = replacement.m_triangles;
            m_vStride = replacement.m_vStride;
            m_tStride = replacement.m_tStride;
            m_hasNormals = replacement.m_hasNormals;
            m_hasColors = replacement.m_hasColors;
            m_hasUVs = replacement.m_hasUVs;
            m_hasTangents = replacement.m_hasTangents;
            m_tangentsDirty = replacement.m_tangentsDirty;
            m_onGPU = false;
            m_boundsDirty = true;

            replacement.m_vertices = nullptr;
            replacement.m_normals = nullptr;
            replacement.m_colors = nullptr;
            replacement.m_uvs = nullptr;
            replacement.m_tangents = nullptr;
            replacement.m_triangles = nullptr;
            replacement.m_vStride = 0;
            replacement.m_tStride = 0;
            replacement.m_hasNormals = false;
            replacement.m_hasColors = false;
            replacement.m_hasUVs = false;
            replacement.m_hasTangents = false;
            replacement.m_tangentsDirty = false;
            replacement.m_onGPU = false;
            replacement.m_boundsDirty = true;
        }

        void computeNormals()
        {

            float * tNormals = new float[m_tStride * 3]();
            delete[] m_normals;
            m_normals = new float[m_vStride * 3]();
            int * vTri = new int[m_vStride]();

            for (uint i = 0; i < m_tStride; i++)
            {
                glm::vec3 v0 = glm::vec3(m_vertices[m_triangles[i*3]*3], m_vertices[m_triangles[i*3]*3+1], m_vertices[m_triangles[i*3]*3+2]);
                glm::vec3 v1 = glm::vec3(m_vertices[m_triangles[i*3+1]*3], m_vertices[m_triangles[i*3+1]*3+1], m_vertices[m_triangles[i*3+1]*3+2]);
                glm::vec3 v2 = glm::vec3(m_vertices[m_triangles[i*3+2]*3], m_vertices[m_triangles[i*3+2]*3+1], m_vertices[m_triangles[i*3+2]*3+2]);
                const glm::vec3 edgeA = v2 - v0;
                const glm::vec3 edgeB = v1 - v0;
                glm::vec3 n(0.0f);
                if (glm::length(edgeA) > 0.0f && glm::length(edgeB) > 0.0f)
                    n = glm::normalize(glm::cross(glm::normalize(edgeA), glm::normalize(edgeB)));
                tNormals[i*3] = n.x;
                tNormals[i*3+1] = n.y;
                tNormals[i*3+2] = n.z;
            }
            for (uint i = 0; i < m_tStride; i++)
            {
                for (uint j = 0; j < 3; j++)
                {
                    m_normals[m_triangles[i*3+j]*3] += tNormals[i*3];
                    m_normals[m_triangles[i*3+j]*3+1] += tNormals[i*3+1];
                    m_normals[m_triangles[i*3+j]*3+2] += tNormals[i*3+2];
                    vTri[m_triangles[i*3+j]]++;
                }
            }
            for (uint i = 0; i < m_vStride; i++)
            {
                if (vTri[i] <= 0)
                    continue;

                glm::vec3 vertexNormal(
                    m_normals[i*3],
                    m_normals[i*3+1],
                    m_normals[i*3+2]
                );

                if (glm::length(vertexNormal) > 0.0f)
                    vertexNormal = glm::normalize(-vertexNormal);
                else
                    vertexNormal = glm::vec3(0.0f, 1.0f, 0.0f);

                m_normals[i*3] = vertexNormal.x;
                m_normals[i*3+1] = vertexNormal.y;
                m_normals[i*3+2] = vertexNormal.z;
            }


            delete[] tNormals;
            delete[] vTri;
            m_hasNormals = true;
            invalidateTangents();
            m_onGPU = false;
            m_boundsDirty = true;
        }

        void computeSphericalUVs()
        {
            if (m_hasUVs)
                delete[] m_uvs;

            m_uvs = new float[m_vStride * 2];
            const glm::vec3 up = glm::vec3(0, 1, 0);
            const glm::vec3 forward = glm::vec3(0, 0, 1);
            for (size_t i = 0; i < m_vStride; i++)
            {
                glm::vec3 v = glm::vec3(m_vertices[(i*3)], m_vertices[(i*3)+1], m_vertices[(i*3)+2]);
                v = glm::normalize(v);
                glm::vec3 v2 = glm::normalize(glm::vec3(v.x, 0, v.z)); // projection sur le plan forward de normale up pas chere
                float u = glm::dot(v, up);
                m_uvs[(i*2)] = (u + 1.0f) * 0.5f;
                m_uvs[(i*2)+1] = u == 1 || u == -1 ? 0.0f : (glm::dot(v2, forward) + 1.0f) * 0.5f;
            }

            m_hasUVs = true;
            invalidateTangents();
            m_onGPU = false;
            m_boundsDirty = true;
        }

        void computeTangents()
        {
            if (!m_hasNormals || !m_hasUVs || m_vertices == nullptr || m_normals == nullptr || m_uvs == nullptr || m_triangles == nullptr)
            {
                delete[] m_tangents;
                m_tangents = nullptr;
                m_hasTangents = false;
                m_tangentsDirty = false;
                return;
            }

            if (!m_tangentsDirty && m_hasTangents && m_tangents != nullptr)
                return;

            if (m_tangents == nullptr)
                m_tangents = new float[m_vStride * 3]();
            else
                for (uint i = 0; i < m_vStride * 3; ++i)
                    m_tangents[i] = 0.0f;

            int * tangentContributions = new int[m_vStride]();

            for (uint i = 0; i < m_tStride; ++i)
            {
                const uint i0 = m_triangles[i * 3];
                const uint i1 = m_triangles[i * 3 + 1];
                const uint i2 = m_triangles[i * 3 + 2];

                const glm::vec3 p0(m_vertices[i0 * 3], m_vertices[i0 * 3 + 1], m_vertices[i0 * 3 + 2]);
                const glm::vec3 p1(m_vertices[i1 * 3], m_vertices[i1 * 3 + 1], m_vertices[i1 * 3 + 2]);
                const glm::vec3 p2(m_vertices[i2 * 3], m_vertices[i2 * 3 + 1], m_vertices[i2 * 3 + 2]);

                const glm::vec2 uv0(m_uvs[i0 * 2], m_uvs[i0 * 2 + 1]);
                const glm::vec2 uv1(m_uvs[i1 * 2], m_uvs[i1 * 2 + 1]);
                const glm::vec2 uv2(m_uvs[i2 * 2], m_uvs[i2 * 2 + 1]);

                const glm::vec3 edge1 = p1 - p0;
                const glm::vec3 edge2 = p2 - p0;
                const glm::vec2 deltaUv1 = uv1 - uv0;
                const glm::vec2 deltaUv2 = uv2 - uv0;

                const float determinant = (deltaUv1.x * deltaUv2.y) - (deltaUv1.y * deltaUv2.x);
                if (std::abs(determinant) <= 1e-8f)
                    continue;

                glm::vec3 tangent = ((edge1 * deltaUv2.y) - (edge2 * deltaUv1.y)) / determinant;
                if (glm::length(tangent) <= 0.0f)
                    continue;

                tangent = glm::normalize(tangent);
                for (const uint vertexIndex : {i0, i1, i2})
                {
                    m_tangents[vertexIndex * 3] += tangent.x;
                    m_tangents[vertexIndex * 3 + 1] += tangent.y;
                    m_tangents[vertexIndex * 3 + 2] += tangent.z;
                    tangentContributions[vertexIndex]++;
                }
            }

            for (uint i = 0; i < m_vStride; ++i)
            {
                glm::vec3 normal(m_normals[i * 3], m_normals[i * 3 + 1], m_normals[i * 3 + 2]);
                if (glm::length(normal) > 0.0f)
                    normal = glm::normalize(normal);
                else
                    normal = glm::vec3(0.0f, 1.0f, 0.0f);

                glm::vec3 tangent(m_tangents[i * 3], m_tangents[i * 3 + 1], m_tangents[i * 3 + 2]);
                if (tangentContributions[i] > 0 && glm::length(tangent) > 0.0f)
                {
                    tangent -= normal * glm::dot(normal, tangent);
                    tangent = glm::length(tangent) > 0.0f ? glm::normalize(tangent) : fallbackTangentForNormal(normal);
                }
                else
                    tangent = fallbackTangentForNormal(normal);

                m_tangents[i * 3] = tangent.x;
                m_tangents[i * 3 + 1] = tangent.y;
                m_tangents[i * 3 + 2] = tangent.z;
            }

            delete[] tangentContributions;
            m_hasTangents = true;
            m_tangentsDirty = false;
            m_onGPU = false;
        }

        void run() override
        {
            if (!m_onGPU || m_boundsDirty)
                ensureBoundsUpToDate();
        }


        float * vertices()
        {
            return m_vertices;
        }

        uint * triangles()
        {
            return m_triangles;
        }

        float * normals()
        {
            return m_normals;
        }

        float * uvs()
        {
            return m_uvs;
        }

        float * colors()
        {
            return m_colors;
        }

        float * tangents()
        {
            return m_tangents;
        }

        uint verticesCount()
        {
            return m_vStride;
        }

        uint trianglesCount()
        {
            return m_tStride;
        }

        bool hasNormals()
        {
            return m_hasNormals;
        }

        bool hasUVs()
        {
            return m_hasUVs;
        }

        bool hasColors()
        {
            return m_hasColors;
        }

        bool hasTangents()
        {
            return m_hasTangents;
        }

        bool isOnGPU()
        {
            return m_onGPU;
        }

        physics::AABB getAABB() const
        {
            ensureBoundsUpToDate();
            return m_AABB;
        }

        void setAssetPath(const std::string& assetPath)
        {
            m_assetPath = assetPath;
        }

        const std::string& getAssetPath() const
        {
            return m_assetPath;
        }

        ~Mesh()
        {
            delete[] m_vertices;
            delete[] m_normals;
            delete[] m_colors;
            delete[] m_uvs;
            delete[] m_tangents;
            delete[] m_triangles;
        }
    };
}
