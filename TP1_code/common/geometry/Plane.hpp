#pragma once

#include "primitive.hpp"
#include <common/gameobject/component/Mesh.hpp>
#include <glm/vec3.hpp>
#include <vector>

class Plane : public component::Mesh
{
private:
    
    double m_size;
    Plane() = delete;
    

public:
    Plane(glm::vec3 position, double size = 1.0, int res = 16) : component::Mesh()
    {
        m_size = size;
        double d = m_size / (((double)res) - 1.0);
        double o = m_size / 2.0;
        const int sideVertexCount = res * res;
        // Duplicate the grid so each side keeps its own normal once backface culling is enabled.
        std::vector<glm::vec3> vertices = std::vector<glm::vec3>(sideVertexCount * 2);
        std::vector<glm::vec2> uvs = std::vector<glm::vec2>(sideVertexCount * 2);
        std::vector<glm::vec3> normals = std::vector<glm::vec3>(sideVertexCount * 2, glm::vec3(0, 1, 0));
        m_hasUVs = true;


        for (int x = 0; x < res; x++)
            for (int z = 0; z < res; z++)
            {
                const int index = x * res + z;
                const glm::vec3 vertex = glm::vec3((double) x * d - o, 0, (double) z * d - o);
                const glm::vec2 uv = glm::vec2(((((double)x)+0.5)/(((double)res))), ((((double)z) + 0.5)/(((double)res))));

                vertices[index] = vertex;
                uvs[index] = uv;
                normals[index] = glm::vec3(0, 1, 0);

                const int backIndex = index + sideVertexCount;
                vertices[backIndex] = vertex;
                uvs[backIndex] = uv;
                normals[backIndex] = glm::vec3(0, -1, 0);
            }
        // triangles (15 * 15 * 2 * 3) per side
        const int sideIndexCount = (res - 1) * (res - 1) * 2 * 3;
        std::vector<int> triangles = std::vector<int>(sideIndexCount * 2);
        for (int x = 0; x < (res - 1); x++)
            for (int z = 0; z < (res - 1); z++) // insert all triangles
            {
                const int triangleOffset = (x * (res - 1) + z) * 6;
                const int v00 = (x * res) + z;
                const int v01 = (x * res) + z + 1;
                const int v10 = ((x + 1) * res) + z;
                const int v11 = ((x + 1) * res) + z + 1;

                triangles[triangleOffset] = v00;
                triangles[triangleOffset + 1] = v01;
                triangles[triangleOffset + 2] = v10;
                triangles[triangleOffset + 3] = v01;
                triangles[triangleOffset + 4] = v11;
                triangles[triangleOffset + 5] = v10;

                const int backTriangleOffset = sideIndexCount + triangleOffset;
                const int backVertexOffset = sideVertexCount;
                triangles[backTriangleOffset] = v10 + backVertexOffset;
                triangles[backTriangleOffset + 1] = v01 + backVertexOffset;
                triangles[backTriangleOffset + 2] = v00 + backVertexOffset;
                triangles[backTriangleOffset + 3] = v10 + backVertexOffset;
                triangles[backTriangleOffset + 4] = v11 + backVertexOffset;
                triangles[backTriangleOffset + 5] = v01 + backVertexOffset;
            }
        
        m_tStride = triangles.size() / 3;
        m_vStride = vertices.size();

        m_vertices = new float[m_vStride * 3];
        m_uvs = new float[m_vStride * 2];
        m_triangles = new uint[m_tStride * 3];

        for (int v = 0; v < vertices.size(); v++)
        {    
            m_vertices[v*3]=vertices[v].x;
            m_vertices[v*3+1]=vertices[v].y;
            m_vertices[v*3+2]=vertices[v].z;
            m_uvs[v*2]=uvs[v].x;
            m_uvs[v*2+1]=uvs[v].y;
        }

        for (int t = 0; t < triangles.size(); t++)
            m_triangles[t]=triangles[t];

        m_normals = new float[m_vStride * 3];
        for (size_t n = 0; n < normals.size(); n++)
        {
            m_normals[n*3] = normals[n].x;
            m_normals[n*3+1] = normals[n].y;
            m_normals[n*3+2] = normals[n].z;
        }
        m_hasNormals = true;
    }

    ~Plane() {
    }
};
