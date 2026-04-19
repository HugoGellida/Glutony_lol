#pragma once
#include "../shader/Shader.hpp"
#include "../shader/Uniforms.hpp"
#include <vector>
#include <GL/glew.h>
#include <glm/vec3.hpp>

namespace simpleMeshRenderer
{
    class SimpleMeshRenderer
    {
        private:
            Shader * m_shader;
            bool m_fboBuilt = false;

            // buffs
            GLuint m_VAO;
            GLuint m_VBO;
            GLuint m_EBO;
            GLuint m_NORMALS;
            bool m_hasNormals = false;
            bool m_invertCull = false;

            //
            std::vector<glm::vec3> * m_vertices;
            std::vector<int> * m_triangles;

            SimpleMeshRenderer() = delete;

            void buildFBO()
            {
                // create fbo
                m_fboBuilt = true;

                //VAO
                glGenVertexArrays(1, &m_VAO);
                glBindVertexArray(m_VAO);
                // vertBuff
                glGenBuffers(1, &m_VBO);
                glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
                glBufferData(GL_ARRAY_BUFFER, m_vertices  -> size() * sizeof(float) * 3, m_vertices -> data(), GL_STATIC_DRAW);

                // triangle Buff
                glGenBuffers(1, &m_EBO);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_triangles -> size() * sizeof(uint), m_triangles -> data(), GL_STATIC_DRAW);

                if (m_hasNormals)
                {
                    //TODO
                }

            }
        public:
            std::vector<IUniform *> m_uniforms;
            SimpleMeshRenderer(Shader * shader, std::vector<glm::vec3> * vertices, std::vector<int> * triangles) 
            {
                m_shader = shader;
                m_vertices = vertices;
                m_triangles = triangles;
            }
            ~SimpleMeshRenderer() {
                glDeleteBuffers(1, &m_VBO);
                glDeleteBuffers(1, &m_EBO);


                glDeleteVertexArrays(1, &m_VAO);
            }

            void rebuildFBO()
            {
                if (m_fboBuilt)
                {
                    glDeleteBuffers(1, &m_VBO);
                    glDeleteBuffers(1, &m_EBO);
                    

                    glDeleteVertexArrays(1, &m_VAO);
                }
                buildFBO();
            }

            void render()
            {
                if (!m_fboBuilt)
                    buildFBO();
                m_shader -> setActive();
                for (int i = 0; i < m_uniforms.size(); i++)
                    m_shader -> Upload(m_uniforms[i]);
                glBindVertexArray(m_VAO);
                // GL state
                glEnable(GL_CULL_FACE);
                glCullFace(m_invertCull ? GL_FRONT : GL_BACK);
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LEQUAL);
                glDrawElements(GL_TRIANGLES, m_triangles -> size(), GL_UNSIGNED_INT, (void*)0);

                // cleanup
                glUseProgram(0);
                glBindVertexArray(0);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
    };
}