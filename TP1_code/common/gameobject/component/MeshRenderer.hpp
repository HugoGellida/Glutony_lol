#pragma once

#include "Component.hpp"
#include "Mesh.hpp"
#include "../../shader/Material.hpp"
#include <GL/glew.h>

using namespace component;
using namespace dataStruct;

namespace component
{
    class MeshRenderer : public Component
    {
    private:
        component::Mesh * m_mesh = nullptr;
        Material * m_mat = nullptr;
        bool m_onGPU = false;
        GLuint m_VBO;
        GLuint m_VAO;
        GLuint m_EBO;
        GLuint m_NORMALS;
        GLuint m_UVS;
        bool m_hasNorm = false;
        bool m_hasUVS = false;
        bool invertCull = false;
        bool m_wireframe = false;
    public:
        MeshRenderer(component::Mesh * mesh, Material * mat) : Component()
        {
            m_mesh = mesh;
            m_mat = mat;
        }

        void setInverted()
        {
            invertCull = true;
        }

        void run() override 
        {
            if (m_onGPU)
                return;
            glGenVertexArrays(1, &m_VAO);
            glBindVertexArray(m_VAO);

            // vert buff
            glGenBuffers(1, &m_VBO);
            glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
            glBufferData(GL_ARRAY_BUFFER, m_mesh -> verticesCount() * 3 * sizeof(float), m_mesh -> vertices(), GL_STATIC_DRAW);

            // triangles buff
            glGenBuffers(1, &m_EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_mesh -> trianglesCount() * 3 * sizeof(uint), m_mesh -> triangles(), GL_STATIC_DRAW);

            if (m_mesh -> hasNormals())
            {
                glGenBuffers(1, &m_NORMALS);
                glBindBuffer(GL_ARRAY_BUFFER, m_NORMALS);
                glBufferData(GL_ARRAY_BUFFER, m_mesh -> verticesCount() * 3 * sizeof(float), m_mesh -> normals(), GL_STATIC_DRAW);
            }

            if (m_mesh -> hasColors())
            {

            }
            
            if (m_mesh -> hasUVs())
            {
                glGenBuffers(1, &m_UVS);
                glBindBuffer(GL_ARRAY_BUFFER, m_UVS);
                glBufferData(GL_ARRAY_BUFFER, m_mesh -> verticesCount() * 2 * sizeof(float), m_mesh -> uvs(), GL_STATIC_DRAW);
            }

            // attach to VAO;
            int attributeIndex = 0;

            glEnableVertexAttribArray(attributeIndex);

            // v a b
            glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
            glVertexAttribPointer(
                attributeIndex,
                3,
                GL_FLOAT,
                GL_FALSE,
                0,
                (void *)0
            );
            if (m_mesh -> hasNormals())
            {
                m_hasNorm = true;
                attributeIndex = 1;
                glBindBuffer(GL_ARRAY_BUFFER, m_NORMALS);
                glVertexAttribPointer(attributeIndex, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
                glEnableVertexAttribArray(attributeIndex);
            }
            if (m_mesh -> hasUVs())
            {
                m_hasUVS = true;
                attributeIndex = 2;
                glBindBuffer(GL_ARRAY_BUFFER, m_UVS);
                glVertexAttribPointer(attributeIndex, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
                glEnableVertexAttribArray(attributeIndex);

                // UV T B
            }
            if (m_mesh -> hasColors())
            {

            }

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
            
            m_onGPU = true;
            
        }
        void render(Camera const & camera, Transform & transform)
        {
            if (!m_onGPU) // will be renderered when stored on gpu.
                return;
            if (m_wireframe)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            else
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glBindVertexArray(m_VAO);

            glDisable(GL_CULL_FACE);
            //glEnable(GL_CULL_FACE);
            //glCullFace(invertCull ? GL_FRONT : GL_BACK);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);

            m_mat -> bind(camera, transform);
            glDrawElements(
                GL_TRIANGLES,
                m_mesh -> trianglesCount() * 3,
                GL_UNSIGNED_INT,
                (void*)0
            );
            // clean state
            glUseProgram(0);
            glBindVertexArray(0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        ~MeshRenderer()
        {
            // clean FBO
            glDeleteBuffers(1, &m_VBO);
            glDeleteBuffers(1, &m_EBO);
            if (m_hasNorm)
                glDeleteBuffers(1, &m_NORMALS);
            if (m_hasUVS)
                glDeleteBuffers(1, &m_UVS);
            glDeleteVertexArrays(1, &m_VAO);
        }


        void toggleWireframe()
        {
            m_wireframe = !m_wireframe;
        }
    };
}
