#pragma once

#include "Component.hpp"
#include "Mesh.hpp"
#include "ComponentSerialization.hpp"
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
        std::string m_meshAssetPath;
        std::string m_materialAssetPath;
        bool m_onGPU = false;
        GLuint m_VBO = 0;
        GLuint m_VAO = 0;
        GLuint m_EBO = 0;
        GLuint m_NORMALS = 0;
        GLuint m_UVS = 0;
        bool m_hasNorm = false;
        bool m_hasUVS = false;
        bool invertCull = false;
        bool m_wireframe = false;

        void releaseGpuResources()
        {
            if (m_VBO != 0)
            {
                glDeleteBuffers(1, &m_VBO);
                m_VBO = 0;
            }

            if (m_EBO != 0)
            {
                glDeleteBuffers(1, &m_EBO);
                m_EBO = 0;
            }

            if (m_NORMALS != 0)
            {
                glDeleteBuffers(1, &m_NORMALS);
                m_NORMALS = 0;
            }

            if (m_UVS != 0)
            {
                glDeleteBuffers(1, &m_UVS);
                m_UVS = 0;
            }

            if (m_VAO != 0)
            {
                glDeleteVertexArrays(1, &m_VAO);
                m_VAO = 0;
            }

            m_hasNorm = false;
            m_hasUVS = false;
            m_onGPU = false;
        }

        void prepareRenderState()
        {
            if (m_wireframe)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            else
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            glBindVertexArray(m_VAO);
            glEnable(GL_CULL_FACE);
            glCullFace(invertCull ? GL_FRONT : GL_BACK);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
        }

        void finishRenderState()
        {
            glUseProgram(0);
            glBindVertexArray(0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    public:
        MeshRenderer(component::Mesh * mesh, Material * mat) : Component()
        {
            setMesh(mesh);
            setMaterial(mat);
        }

        void setInverted()
        {
            invertCull = true;
        }

        void setMesh(component::Mesh* mesh)
        {
            if (m_mesh != mesh)
                releaseGpuResources();

            m_mesh = mesh;
            m_meshAssetPath = (m_mesh != nullptr) ? m_mesh->getAssetPath() : "";
        }

        component::Mesh* getMesh() const
        {
            return m_mesh;
        }

        void setMaterial(Material* material)
        {
            m_mat = material;
            m_materialAssetPath = (m_mat != nullptr) ? m_mat->getAssetPath() : "";
        }

        Material* getMaterial() const
        {
            return m_mat;
        }

        void setMeshAssetPath(const std::string& assetPath)
        {
            m_meshAssetPath = assetPath;
        }

        const std::string& getMeshAssetPath() const
        {
            return m_meshAssetPath;
        }

        void setMaterialAssetPath(const std::string& assetPath)
        {
            m_materialAssetPath = assetPath;
        }

        const std::string& getMaterialAssetPath() const
        {
            return m_materialAssetPath;
        }

        static const component_meta::ComponentDescriptor& componentDescriptor();

        const component_meta::ComponentDescriptor* getComponentDescriptor() const override
        {
            return &componentDescriptor();
        }

        void run() override 
        {
            if (m_mesh == nullptr || m_mat == nullptr)
                return;

            if (m_onGPU && m_mesh -> isOnGPU())
                return;
            if (m_onGPU)
                releaseGpuResources();

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
            if (!m_onGPU || m_mesh == nullptr || m_mat == nullptr) // will be renderered when stored on gpu.
                return;
            prepareRenderState();

            m_mat -> bind(camera, transform);
            glDrawElements(
                GL_TRIANGLES,
                m_mesh -> trianglesCount() * 3,
                GL_UNSIGNED_INT,
                (void*)0
            );
            finishRenderState();
        }

        void renderWithMaterial(Camera const& camera, Transform& transform, Material& material)
        {
            if (!m_onGPU || m_mesh == nullptr)
                return;
            
            prepareRenderState();

            material.bind(camera, transform);
            glDrawElements(
                GL_TRIANGLES,
                m_mesh -> trianglesCount() * 3,
                GL_UNSIGNED_INT,
                (void*)0
            );

            finishRenderState();
        }

        void renderOverlayWithMaterial(Camera const& camera, Transform& transform, Material& material)
        {
            if (!m_onGPU || m_mesh == nullptr)
                return;

            prepareRenderState();

            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            material.bind(camera, transform);
            glDrawElements(
                GL_TRIANGLES,
                m_mesh -> trianglesCount() * 3,
                GL_UNSIGNED_INT,
                (void*)0
            );
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
            finishRenderState();
        }

        ~MeshRenderer()
        {
            releaseGpuResources();
        }


        void toggleWireframe()
        {
            m_wireframe = !m_wireframe;
        }
    };
}
