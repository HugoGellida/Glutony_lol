#pragma once

#include "../Camera.hpp"
#include "../gameobject/GameObject.hpp"
#include "../gameobject/component/MeshRenderer.hpp"
#include "../shader/Material.hpp"
#include "RenderLightData.hpp"

#include <string>

class Scene;

namespace render
{
struct UniformFactoryExecutionContext
{
    const Scene* scene = nullptr;
    const Camera* camera = nullptr;
    const GameObject* gameObject = nullptr;
    const component::MeshRenderer* meshRenderer = nullptr;
    const Transform* transform = nullptr;
    const dataStruct::Material* sourceMaterial = nullptr;
    const LightInput* light = nullptr;
    int currentIteration = 0;
    int iterationCount = 1;

    bool hasLight() const
    {
        return light != nullptr;
    }
};

class UniformFactoryWriter
{
private:
    dataStruct::Material* m_material = nullptr;

public:
    explicit UniformFactoryWriter(dataStruct::Material& material)
        : m_material(&material)
    {
    }

    void addBoolUniform(const std::string& uniformName, bool value) const
    {
        if (m_material != nullptr)
            m_material->addBoolUniform(uniformName, value);
    }

    void addIntUniform(const std::string& uniformName, int value) const
    {
        if (m_material != nullptr)
            m_material->addIntUniform(uniformName, value);
    }

    void addFloatUniform(const std::string& uniformName, float value) const
    {
        if (m_material != nullptr)
            m_material->addFloatUniform(uniformName, value);
    }

    void addVec3Uniform(const std::string& uniformName, const glm::vec3& value) const
    {
        if (m_material != nullptr)
            m_material->addVec3Uniform(uniformName, value);
    }

    void addMat4Uniform(const std::string& uniformName, const glm::mat4& value) const
    {
        if (m_material != nullptr)
            m_material->addMat4Uniform(uniformName, value);
    }
};

using UniformFactoryIterationCountEntry = int(*)(const UniformFactoryExecutionContext& context);
using UniformFactoryIterationGroupEntry = std::string(*)(const UniformFactoryExecutionContext& context);
using UniformFactoryEntry = bool(*)(const UniformFactoryExecutionContext& context, const UniformFactoryWriter& writer);
}