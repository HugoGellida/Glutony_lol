#pragma once

#include "../AssetManager.hpp"

#include <common/gameobject/component/ComponentSerialization.hpp>

#include <string>

namespace asset::editor
{
struct DataAssetEditorModel
{
    std::string normalizedAssetPath;
    DataAssetDefinition definition;
    bool available = false;
    bool valid = false;
    std::string errorMessage;
};

inline component_meta::FieldKind fieldKindFromDataAssetValueKind(DataAssetValueKind kind)
{
    switch (kind)
    {
    case DataAssetValueKind::Bool:
        return component_meta::FieldKind::Bool;
    case DataAssetValueKind::Int:
        return component_meta::FieldKind::Int;
    case DataAssetValueKind::Float:
        return component_meta::FieldKind::Float;
    case DataAssetValueKind::Vec3:
        return component_meta::FieldKind::Vec3;
    case DataAssetValueKind::String:
        return component_meta::FieldKind::String;
    case DataAssetValueKind::Group:
    default:
        return component_meta::FieldKind::String;
    }
}

inline component_meta::SerializedValue serializedValueFromDataAssetNode(const DataAssetNodeDefinition& node)
{
    switch (node.kind)
    {
    case DataAssetValueKind::Bool:
        return node.boolValue;
    case DataAssetValueKind::Int:
        return node.intValue;
    case DataAssetValueKind::Float:
        return node.floatValue;
    case DataAssetValueKind::Vec3:
        return node.vec3Value;
    case DataAssetValueKind::String:
        return node.stringValue;
    case DataAssetValueKind::Group:
    default:
        return std::string();
    }
}

inline DataAssetEditorModel loadDataAssetEditorModel(const std::string& assetPath)
{
    DataAssetEditorModel model;
    model.normalizedAssetPath = AssetManager::normalizeRelativePath(assetPath);
    if (model.normalizedAssetPath.empty())
        return model;

    model.available = true;
    if (!AssetManager::hasExtension(model.normalizedAssetPath, ".data"))
    {
        model.errorMessage = "Only .data assets can be edited here.";
        return model;
    }

    DataAssetDefinition* definition = AssetManager::instance().loadDataAssetDefinition(model.normalizedAssetPath);
    if (definition == nullptr)
    {
        model.errorMessage = "Failed to load data asset.";
        return model;
    }

    model.definition = *definition;
    model.valid = true;
    return model;
}
}