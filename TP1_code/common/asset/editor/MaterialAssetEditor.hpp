#pragma once

#include "../AssetManager.hpp"
#include "../MaterialAssetIO.hpp"

#include <common/gameobject/component/ComponentSerialization.hpp>

#include <cstdint>
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

inline MaterialAssetDefinition normalizeDefinitionForShader(const MaterialAssetDefinition& definition, Shader& shader, std::vector<std::string>* unsupportedUniforms = nullptr)
{
    MaterialAssetDefinition normalized;
    normalized.kind = definition.kind;
    normalized.shaderPath = definition.shaderPath;

    for (const Shader::UniformDescriptor& descriptor : shader.getEditableUniforms())
    {
        if (!descriptor.supported())
        {
            if (unsupportedUniforms != nullptr)
                unsupportedUniforms->push_back(descriptor.name);
            continue;
        }

        const MaterialUniformKind kind = uniformKindFromShaderKind(descriptor.kind);
        if (const MaterialUniformDefinition* existing = findUniformDefinition(definition, descriptor.name, kind))
            normalized.uniforms.push_back(*existing);
        else
            normalized.uniforms.push_back(makeDefaultUniformDefinition(descriptor));
    }

    return normalized;
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

    Shader* shader = AssetManager::instance().loadShader(model.definition.shaderPath);
    if (shader == nullptr)
    {
        model.errorMessage = "Failed to load shader asset.";
        return model;
    }

    model.shaderRevision = shader->getReloadGeneration();

    model.definition = normalizeDefinitionForShader(model.definition, *shader, &model.unsupportedUniforms);

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