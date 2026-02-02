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
    Plane(glm::vec3 position, double size = 1.0) : component::Mesh()
    {
        m_size = size;
        double d = m_size / 16.0;
        double o = m_size / 2.0;
        // vert (16 * 16)
        std::vector<glm::vec3> vertices = std::vector<glm::vec3>(16*16);
        std::vector<glm::vec2> uvs = std::vector<glm::vec2>(16*16);
        m_hasUVs = true;


        for (int x = 0; x < 16; x++)
            for (int z = 0; z < 16; z++)
            {
                vertices[x * 16 + z] = glm::vec3((double) x * d - o, 0, (double) z * d - o);
                uvs[x * 16 + z] = glm::vec2((x/15.0), (z/15.0));
            }
        // triangles (15 * 15 * 2 * 3)
        std::vector<int> triangles = std::vector<int>(15 * 15 * 2 * 3);
        for (int x = 0; x < 15; x++)
            for (int z = 0; z < 15; z++) // insert all triangles
            {
                triangles[(x * 15 + z) * 6] = ((x * 16) + z);
                triangles[(x * 15 + z) * 6 + 1] = (x * 16 + z + 1);
                triangles[(x * 15 + z) * 6 + 2] = ((x + 1) * 16 + z);
                triangles[(x * 15 + z) * 6 + 3] = (x * 16 + z + 1);
                triangles[(x * 15 + z) * 6 + 4] = ((x + 1) * 16 + z + 1);
                triangles[(x * 15 + z) * 6 + 5] = ((x + 1) * 16 + z);
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
        
            
        
    }

    ~Plane() {
    }
};
