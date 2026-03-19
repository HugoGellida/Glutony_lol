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
        }
        ~LitMaterial()
        {
            delete m_main_col;
        }
    };
}