#pragma once

#include "MaterialAssetIO.hpp"
#include "common/FileLoader.hpp"
#include "common/gameobject/component/Mesh.hpp"
#include "common/shader/LitMaterial.hpp"
#include "common/shader/Shader.hpp"
#include "common/shader/UnlitMaterial.hpp"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace asset
{
enum class AssetType
{
    Mesh,
    Shader,
    Material,
};

struct AssetTypeHash
{
    std::size_t operator()(AssetType type) const
    {
        return static_cast<std::size_t>(type);
    }
};

class SceneAssetRegistry
{
private:
    std::unordered_map<AssetType, std::vector<std::string>, AssetTypeHash> m_assetPaths;

public:
    void clear()
    {
        m_assetPaths.clear();
    }

    void registerAsset(AssetType type, const std::string& relativePath)
    {
        std::vector<std::string>& paths = m_assetPaths[type];
        if (std::find(paths.begin(), paths.end(), relativePath) == paths.end())
            paths.push_back(relativePath);
    }

    const std::vector<std::string>& getAssets(AssetType type) const
    {
        static const std::vector<std::string> empty;
        const auto it = m_assetPaths.find(type);
        return it != m_assetPaths.end() ? it->second : empty;
    }

    const std::unordered_map<AssetType, std::vector<std::string>, AssetTypeHash>& getAllAssets() const
    {
        return m_assetPaths;
    }
};

class AssetManager
{
private:
    std::unordered_map<std::string, component::Mesh*> m_meshAssets;
    std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaderAssets;
    std::unordered_map<std::string, std::unique_ptr<dataStruct::Material>> m_materialAssets;
    std::unordered_map<AssetType, std::vector<std::string>, AssetTypeHash> m_assetPaths;

    AssetManager() = default;

    void registerGlobalAsset(AssetType type, const std::string& relativePath)
    {
        std::vector<std::string>& paths = m_assetPaths[type];
        if (std::find(paths.begin(), paths.end(), relativePath) == paths.end())
            paths.push_back(relativePath);
    }

public:
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    ~AssetManager()
    {
        for (auto& entry : m_meshAssets)
        {
            component::Mesh* mesh = entry.second;
            if (mesh == nullptr)
                continue;

            if (mesh->ownerCount > 0)
                mesh->ownerCount--;

            if (mesh->ownerCount == 0)
                delete mesh;
        }
    }

    static AssetManager& instance()
    {
        static AssetManager manager;
        return manager;
    }

    static std::string normalizeRelativePath(const std::string& rawPath)
    {
        std::string normalized = rawPath;
        std::replace(normalized.begin(), normalized.end(), '\\', '/');

        while (normalized.rfind("./", 0) == 0)
            normalized.erase(0, 2);

        while (!normalized.empty() && normalized.front() == '/')
            normalized.erase(normalized.begin());

        return normalized;
    }

    static std::string runtimePath(const std::string& relativePath)
    {
        return "./" + normalizeRelativePath(relativePath);
    }

    static bool hasExtension(const std::string& path, const std::string& extension)
    {
        if (path.size() < extension.size())
            return false;

        return path.compare(path.size() - extension.size(), extension.size(), extension) == 0;
    }

    component::Mesh* loadMesh(const std::string& relativePath)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        registerGlobalAsset(AssetType::Mesh, normalizedPath);

        const auto it = m_meshAssets.find(normalizedPath);
        if (it != m_meshAssets.end())
            return it->second;

        component::Mesh* mesh = fileLoader::loadModelFile(runtimePath(normalizedPath));
        if (mesh == nullptr)
            return nullptr;

        mesh->setAssetPath(normalizedPath);
        mesh->ownerCount++;
        m_meshAssets[normalizedPath] = mesh;
        return mesh;
    }

    Shader* loadShader(const std::string& relativePath)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        registerGlobalAsset(AssetType::Shader, normalizedPath);

        const auto it = m_shaderAssets.find(normalizedPath);
        if (it != m_shaderAssets.end())
            return it->second.get();

        const std::string vertexPath = runtimePath(normalizedPath + "/vertex.glsl");
        const std::string fragmentPath = runtimePath(normalizedPath + "/fragment.glsl");

        std::unique_ptr<Shader> shader = std::make_unique<Shader>(vertexPath, fragmentPath);
    shader->setAssetPath(normalizedPath);
        Shader* shaderPtr = shader.get();
        m_shaderAssets[normalizedPath] = std::move(shader);
        return shaderPtr;
    }

    dataStruct::Material* loadMaterial(const std::string& relativePath)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        if (!hasExtension(normalizedPath, ".mat"))
        {
            std::cerr << "Material asset must use .mat extension: " << normalizedPath << std::endl;
            return nullptr;
        }

        registerGlobalAsset(AssetType::Material, normalizedPath);

        const auto it = m_materialAssets.find(normalizedPath);
        if (it != m_materialAssets.end())
            return it->second.get();

        MaterialAssetDefinition definition;
        if (!MaterialAssetIO::loadDefinition(runtimePath(normalizedPath), definition))
        {
            std::cerr << "Failed to load material asset: " << normalizedPath << std::endl;
            return nullptr;
        }

        Shader* shader = loadShader(definition.shaderPath);
        if (shader == nullptr)
            return nullptr;

        std::unique_ptr<dataStruct::Material> material;
        if (definition.kind == MaterialAssetKind::Unlit)
        {
            std::unique_ptr<dataStruct::UnlitMaterial> unlit = std::make_unique<dataStruct::UnlitMaterial>(shader);
            unlit->setMainColor(definition.mainColor);
            material = std::move(unlit);
        }
        else
        {
            std::unique_ptr<dataStruct::LitMaterial> lit = std::make_unique<dataStruct::LitMaterial>(shader);
            lit->setMainColor(definition.mainColor);
            material = std::move(lit);
        }

        dataStruct::Material* materialPtr = material.get();
        materialPtr->setAssetPath(normalizedPath);
        m_materialAssets[normalizedPath] = std::move(material);
        return materialPtr;
    }

    const std::vector<std::string>& getAssets(AssetType type) const
    {
        static const std::vector<std::string> empty;
        const auto it = m_assetPaths.find(type);
        return it != m_assetPaths.end() ? it->second : empty;
    }
};
}