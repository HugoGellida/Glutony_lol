#pragma once

#include "../AssetManager.hpp"
#include "../MaterialAssetIO.hpp"

#include <common/gameobject/component/ComponentSerialization.hpp>

#include <cstdint>
#include <unordered_set>
#include <string>
#include <vector>

namespace asset::editor
{
struct MaterialAssetEditorField
{
    std::string key;
    std::string label;
    component_meta::FieldKind fieldKind = component_meta::FieldKind::String;
    component_meta::AssetReferenceKind assetReferenceKind = component_meta::AssetReferenceKind::None;
    component_meta::SerializedValue value = std::string();
};

struct MaterialAssetEditorModel
{
    std::string normalizedAssetPath;
    MaterialAssetDefinition definition;
    std::vector<MaterialAssetEditorField> fields;
    std::vector<std::string> unsupportedUniforms;
    uint64_t shaderRevision = 0;
    bool available = false;
    bool valid = false;
    std::string errorMessage;
};

inline const char* materialAssetKindValue(MaterialAssetKind kind)
{
    return kind == MaterialAssetKind::Unlit ? "unlit" : "lit";
}

inline component_meta::FieldKind fieldKindFromUniformKind(MaterialUniformKind kind)
{
    switch (kind)
    {
    case MaterialUniformKind::Bool:
        return component_meta::FieldKind::Bool;
    case MaterialUniformKind::Int:
        return component_meta::FieldKind::Int;
    case MaterialUniformKind::Float:
        return component_meta::FieldKind::Float;
    case MaterialUniformKind::Vec3:
        return component_meta::FieldKind::Vec3;
    case MaterialUniformKind::Texture:
        return component_meta::FieldKind::Asset;
    }

    return component_meta::FieldKind::String;
}

inline component_meta::AssetReferenceKind assetReferenceKindFromUniformKind(MaterialUniformKind kind)
{
    return kind == MaterialUniformKind::Texture ? component_meta::AssetReferenceKind::Texture : component_meta::AssetReferenceKind::None;
}

inline component_meta::SerializedValue serializedValueFromUniform(const MaterialUniformDefinition& uniform)
{
    switch (uniform.kind)
    {
    case MaterialUniformKind::Bool:
        return uniform.boolValue;
    case MaterialUniformKind::Int:
        return uniform.intValue;
    case MaterialUniformKind::Float:
        return uniform.floatValue;
    case MaterialUniformKind::Vec3:
        return uniform.vec3Value;
    case MaterialUniformKind::Texture:
        return uniform.textureAssetPath;
    }

    return std::string();
}

inline MaterialUniformKind uniformKindFromShaderKind(Shader::EditableUniformKind kind)
{
    switch (kind)
    {
    case Shader::EditableUniformKind::Bool:
        return MaterialUniformKind::Bool;
    case Shader::EditableUniformKind::Int:
        return MaterialUniformKind::Int;
    case Shader::EditableUniformKind::Float:
        return MaterialUniformKind::Float;
    case Shader::EditableUniformKind::Vec3:
        return MaterialUniformKind::Vec3;
    case Shader::EditableUniformKind::Texture:
        return MaterialUniformKind::Texture;
    case Shader::EditableUniformKind::Unsupported:
        break;
    }

    return MaterialUniformKind::Float;
}

inline MaterialUniformDefinition makeDefaultUniformDefinition(const Shader::UniformDescriptor& descriptor)
{
    MaterialUniformDefinition uniform;
    uniform.name = descriptor.name;
    uniform.kind = uniformKindFromShaderKind(descriptor.kind);
    if (uniform.kind == MaterialUniformKind::Vec3 && uniform.name == "_mainCol")
        uniform.vec3Value = glm::vec3(0.5f, 0.5f, 0.5f);
    return uniform;
}

inline const MaterialUniformDefinition* findUniformDefinition(const MaterialAssetDefinition& definition, const std::string& name, MaterialUniformKind kind)
{
    for (const MaterialUniformDefinition& uniform : definition.uniforms)
    {
        if (uniform.name == name && uniform.kind == kind)
            return &uniform;
    }
    return nullptr;
}

inline MaterialUniformDefinition* findUniformDefinition(MaterialAssetDefinition& definition, const std::string& name, MaterialUniformKind kind)
{
    for (MaterialUniformDefinition& uniform : definition.uniforms)
    {
        if (uniform.name == name && uniform.kind == kind)
            return &uniform;
    }
    return nullptr;
}

inline void appendUniqueString(std::vector<std::string>& values, const std::string& value)
{
    for (const std::string& existing : values)
    {
        if (existing == value)
            return;
    }

    values.push_back(value);
}

inline MaterialAssetDefinition normalizeDefinitionForShaders(const MaterialAssetDefinition& definition, const std::vector<Shader*>& shaders, std::vector<std::string>* unsupportedUniforms = nullptr)
{
    MaterialAssetDefinition normalized;
    normalized.kind = definition.kind;
    normalized.shaderPath = definition.shaderPath;
    normalized.renderPassPath = definition.renderPassPath;

    for (Shader* shader : shaders)
    {
        if (shader == nullptr)
            continue;

        for (const Shader::UniformDescriptor& descriptor : shader->getEditableUniforms())
        {
            if (!descriptor.supported())
            {
                if (unsupportedUniforms != nullptr)
                    appendUniqueString(*unsupportedUniforms, descriptor.name);
                continue;
            }

            const MaterialUniformKind kind = uniformKindFromShaderKind(descriptor.kind);
            if (findUniformDefinition(normalized, descriptor.name, kind) != nullptr)
                continue;

            if (const MaterialUniformDefinition* existing = findUniformDefinition(definition, descriptor.name, kind))
                normalized.uniforms.push_back(*existing);
            else
                normalized.uniforms.push_back(makeDefaultUniformDefinition(descriptor));
        }
    }

    for (const MaterialUniformDefinition& uniform : definition.uniforms)
    {
        if (findUniformDefinition(normalized, uniform.name, uniform.kind) != nullptr)
            continue;

        normalized.uniforms.push_back(uniform);
    }

    return normalized;
}

inline MaterialAssetDefinition normalizeDefinitionForShader(const MaterialAssetDefinition& definition, Shader& shader, std::vector<std::string>* unsupportedUniforms = nullptr)
{
    return normalizeDefinitionForShaders(definition, {&shader}, unsupportedUniforms);
}

inline bool resolveMaterialAssetShaders(const MaterialAssetDefinition& definition, std::vector<Shader*>& shadersOut, uint64_t& revisionOut, std::string* errorMessage = nullptr)
{
    shadersOut.clear();
    revisionOut = 0;

    std::unordered_set<std::string> seenShaderPaths;
    auto appendShader = [&](const std::string& shaderPath) -> bool {
        const std::string normalizedShaderPath = AssetManager::normalizeRelativePath(shaderPath);
        if (normalizedShaderPath.empty() || seenShaderPaths.count(normalizedShaderPath) > 0)
            return true;

        Shader* shader = AssetManager::instance().loadShader(normalizedShaderPath);
        if (shader == nullptr)
        {
            if (errorMessage != nullptr)
                *errorMessage = "Failed to load shader asset.";
            return false;
        }

        seenShaderPaths.insert(normalizedShaderPath);
        shadersOut.push_back(shader);
        revisionOut = revisionOut * 1315423911ULL + shader->getReloadGeneration();
        return true;
    };

    if (!definition.renderPassPath.empty())
    {
        RenderPassAssetDefinition* renderPass = AssetManager::instance().loadRenderPassDefinition(definition.renderPassPath);
        if (renderPass == nullptr)
        {
            if (errorMessage != nullptr)
                *errorMessage = "Failed to load render pass asset.";
            return false;
        }

        for (const RenderPassStepDefinition& pass : renderPass->passes)
        {
            if (!appendShader(pass.shaderPath))
                return false;
        }

        if (!shadersOut.empty())
            return true;
    }

    if (!appendShader(definition.shaderPath))
        return false;

    if (!shadersOut.empty())
        return true;

    if (errorMessage != nullptr)
        *errorMessage = "No shader could be resolved for this material asset.";
    return false;
}

inline MaterialAssetEditorModel loadMaterialAssetEditorModel(const std::string& assetPath)
{
    MaterialAssetEditorModel model;
    model.normalizedAssetPath = AssetManager::normalizeRelativePath(assetPath);
    if (model.normalizedAssetPath.empty())
        return model;

    model.available = true;
    if (!AssetManager::hasExtension(model.normalizedAssetPath, ".mat"))
    {
        model.errorMessage = "Only .mat assets can be edited here.";
        return model;
    }

    if (!MaterialAssetIO::loadDefinition(AssetManager::runtimePath(model.normalizedAssetPath), model.definition))
    {
        model.errorMessage = "Failed to load material asset.";
        return model;
    }

    std::vector<Shader*> shaders;
    if (!resolveMaterialAssetShaders(model.definition, shaders, model.shaderRevision, &model.errorMessage))
    {
        return model;
    }

    model.definition = normalizeDefinitionForShaders(model.definition, shaders, &model.unsupportedUniforms);

    for (const MaterialUniformDefinition& uniform : model.definition.uniforms)
    {
        MaterialAssetEditorField field;
        field.key = uniform.name;
        field.label = uniform.name;
        field.fieldKind = fieldKindFromUniformKind(uniform.kind);
        field.assetReferenceKind = assetReferenceKindFromUniformKind(uniform.kind);
        field.value = serializedValueFromUniform(uniform);
        model.fields.push_back(field);
    }

    model.valid = true;
    return model;
}
}