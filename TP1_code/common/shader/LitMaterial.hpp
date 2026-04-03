#pragma once
#include "Material.hpp"


namespace dataStruct
{
    class LitMaterial : public Material
    {
    private:
        glm::vec3 * m_main_col;
    public:
        LitMaterial() = delete;
        LitMaterial(Shader * shader) : Material(shader)
        {
            m_main_col = new glm::vec3(0.5, 0.5, 0.5);
            push_back(m_uniVec3f, new UniformVec3f("_mainCol", m_main_col), m_uniVec3f_stride);
        }
        void setMainColor(glm::vec3 col)
        {
            *m_main_col = col;
            addVec3Uniform("_mainCol", col);
        }

        void setRuntimePreviewSyncEnabled(bool enabled)
        {
            Material::setRuntimePreviewSyncEnabled(enabled);
        }

        bool consumeRuntimeDefinition(asset::MaterialAssetDefinition& definitionOut)
        {
            return Material::consumeRuntimeDefinition(definitionOut);
        }
        ~LitMaterial()
        {
            delete m_main_col;
        }
    };
}