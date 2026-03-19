#pragma once

#include <glm/glm.hpp>
#include "Component.hpp"

namespace component
{
    class Mesh : public Component
    {
    protected:
        float * m_vertices = nullptr;
        float * m_normals = nullptr;
        float * m_colors = nullptr;
        float * m_uvs = nullptr;
        uint * m_triangles = nullptr;
        uint m_vStride = 0;
        uint m_tStride = 0;
        bool m_hasNormals = false;
        bool m_hasColors = false;
        bool m_hasUVs = false;
        bool m_onGPU = false;
    public:
        Mesh() : Component() {}
        Mesh(uint vStride, uint tStride, bool hasNormals = false, bool hasColors = false, bool hasUVs = false) : Component()
        {
            m_vStride = vStride;
            m_vertices = new float[vStride * 3];
            m_tStride = tStride;
            m_triangles = new uint[tStride * 3];
            
            m_hasNormals = hasNormals;
            if (hasNormals)
                m_normals = new float[vStride * 3];
            else
                m_normals = new float[0];
            
            m_hasColors = hasColors;
            if (hasColors)
                m_colors = new float[vStride * 3];

            m_hasUVs = hasUVs;
            if (hasUVs)
                m_uvs = new float[vStride * 2];
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
        }
        void setTriangle(uint start, uint t1, uint t2, uint t3)
        {
            m_triangles[start * 3] = t1;
            m_triangles[start * 3 + 1] = t2;
            m_triangles[start * 3 + 2] = t3;
        }

        void computeNormals()
        {

            float * tNormals = new float[m_tStride * 3];
            delete[] m_normals;
            m_normals = new float[m_vStride * 3];
            int * vTri = new int[m_vStride];

            for (uint i = 0; i < m_tStride; i++)
            {
                glm::vec3 v0 = glm::vec3(m_vertices[m_triangles[i*3]*3], m_vertices[m_triangles[i*3]*3+1], m_vertices[m_triangles[i*3]*3+2]);
                glm::vec3 v1 = glm::vec3(m_vertices[m_triangles[i*3+1]*3], m_vertices[m_triangles[i*3+1]*3+1], m_vertices[m_triangles[i*3+1]*3+2]);
                glm::vec3 v2 = glm::vec3(m_vertices[m_triangles[i*3+2]*3], m_vertices[m_triangles[i*3+2]*3+1], m_vertices[m_triangles[i*3+2]*3+2]);
                glm::vec3 n = glm::normalize(glm::cross(glm::normalize(v2 - v0),glm::normalize(v1 - v0)));
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
                m_normals[i*3]/=-vTri[i];
                m_normals[i*3+1]/=-vTri[i];
                m_normals[i*3+2]/=-vTri[i];
            }


            delete[] tNormals;
            delete[] vTri;
            m_hasNormals = true;
            m_onGPU = false;
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
            m_onGPU = false;
        }

        void run() override
        {

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

        bool isOnGPU()
        {
            return m_onGPU;
        }

        ~Mesh()
        {
            delete[] m_vertices;
            delete[] m_normals;
            delete[] m_colors;
            delete[] m_uvs;
            delete[] m_triangles;
        }
    };
}

