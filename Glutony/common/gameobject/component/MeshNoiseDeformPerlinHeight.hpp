#pragma once
#include "Component.hpp"
#include "Mesh.hpp"
#include "../../noise/PerlinNoise.hpp"


namespace component
{
    class MeshNoisePerlinHeight : public Component
    {
    protected:
        bool m_upToDate = false;
        Mesh * m_mesh = nullptr;
    public:
        MeshNoisePerlinHeight(Mesh * mesh) : Component() 
        {
            m_mesh = mesh;
        }
        void run() override
        {
            if (m_upToDate && m_mesh != nullptr && m_mesh -> verticesCount() > 0)
                return;
            for (int i = 0; i < m_mesh->verticesCount() * 3; i+=3)
                m_mesh->vertices()[i+1] += noise::PerlinNoise::noise(
                    m_mesh->vertices()[i],
                    m_mesh->vertices()[i+1],
                    m_mesh->vertices()[i+2]
                );
            m_mesh->computeNormals();
            m_upToDate = true;
        }
    };
}