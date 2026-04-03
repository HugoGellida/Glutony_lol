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
#include <unordered_set>
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
    std::unordered_map<std::string, std::string> m_materialShaderPaths;
    std::unordered_set<std::string> m_pendingChangedShaderPaths;
    std::unordered_map<AssetType, std::vector<std::string>, AssetTypeHash> m_assetPaths;

    AssetManager() = default;

    std::unique_ptr<dataStruct::Material> createMaterialAsset(const std::string& normalizedPath)
    {
        MaterialAssetDefinition definition;
        if (!MaterialAssetIO::loadDefinition(runtimePath(normalizedPath), definition))
        {
            std::cerr << "Failed to load material asset: " << normalizedPath << std::endl;
            return nullptr;
        }

        m_materialShaderPaths[normalizedPath] = normalizeRelativePath(definition.shaderPath);

        Shader* shader = loadShader(definition.shaderPath);
        if (shader == nullptr)
            return nullptr;

        std::unique_ptr<dataStruct::Material> material;
        if (definition.kind == MaterialAssetKind::Unlit)
        {
            std::unique_ptr<dataStruct::UnlitMaterial> unlit = std::make_unique<dataStruct::UnlitMaterial>(shader);
            unlit->setRuntimeDefinitionHeader(definition.kind, definition.shaderPath);
            material = std::move(unlit);
        }
        else
        {
            std::unique_ptr<dataStruct::LitMaterial> lit = std::make_unique<dataStruct::LitMaterial>(shader);
            lit->setRuntimeDefinitionHeader(definition.kind, definition.shaderPath);
            material = std::move(lit);
        }

        for (const MaterialUniformDefinition& uniform : definition.uniforms)
        {
            switch (uniform.kind)
            {
            case MaterialUniformKind::Bool:
                material->addBoolUniform(uniform.name, uniform.boolValue);
                break;
            case MaterialUniformKind::Int:
                material->addIntUniform(uniform.name, uniform.intValue);
                break;
            case MaterialUniformKind::Float:
                material->addFloatUniform(uniform.name, uniform.floatValue);
                break;
            case MaterialUniformKind::Vec3:
                material->addVec3Uniform(uniform.name, uniform.vec3Value);
                break;
            case MaterialUniformKind::Texture:
                if (!uniform.textureAssetPath.empty())
                    material->addTexture(uniform.name, runtimePath(uniform.textureAssetPath));
                break;
            }
        }

        material->setAssetPath(normalizedPath);
        return material;
    }

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
        {
            if (it->second->refreshIfSourcesChanged())
                m_pendingChangedShaderPaths.insert(normalizedPath);
            return it->second.get();
        }

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

        std::unique_ptr<dataStruct::Material> material = createMaterialAsset(normalizedPath);
        if (material == nullptr)
            return nullptr;

        dataStruct::Material* materialPtr = material.get();
        m_materialAssets[normalizedPath] = std::move(material);
        return materialPtr;
    }

    bool reloadMaterial(const std::string& relativePath, dataStruct::Material*& materialOut)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        if (!hasExtension(normalizedPath, ".mat"))
        {
            materialOut = nullptr;
            return false;
        }

        registerGlobalAsset(AssetType::Material, normalizedPath);

        std::unique_ptr<dataStruct::Material> previousMaterial;
        const auto existing = m_materialAssets.find(normalizedPath);
        if (existing != m_materialAssets.end())
        {
            previousMaterial = std::move(existing->second);
            m_materialAssets.erase(existing);
        }

        std::unique_ptr<dataStruct::Material> reloadedMaterial = createMaterialAsset(normalizedPath);
        if (reloadedMaterial == nullptr)
        {
            materialOut = nullptr;
            if (previousMaterial != nullptr)
            {
                materialOut = previousMaterial.get();
                m_materialAssets[normalizedPath] = std::move(previousMaterial);
            }
            return false;
        }

        materialOut = reloadedMaterial.get();
        m_materialAssets[normalizedPath] = std::move(reloadedMaterial);
        return true;
    }

    std::vector<std::string> collectMaterialsNeedingShaderRefresh()
    {
        std::unordered_set<std::string> changedShaderPaths = std::move(m_pendingChangedShaderPaths);
        m_pendingChangedShaderPaths.clear();

        for (auto& entry : m_shaderAssets)
        {
            if (entry.second != nullptr && entry.second->refreshIfSourcesChanged())
                changedShaderPaths.insert(entry.first);
        }

        if (changedShaderPaths.empty())
            return {};

        std::vector<std::string> materialPaths;
        for (const auto& entry : m_materialShaderPaths)
        {
            if (changedShaderPaths.count(entry.second) == 0)
                continue;
            if (m_materialAssets.find(entry.first) == m_materialAssets.end())
                continue;
            materialPaths.push_back(entry.first);
        }

        return materialPaths;
    }

    bool popDirtyMaterialState(std::string& assetPathOut, MaterialAssetDefinition& definitionOut)
    {
        for (auto& entry : m_materialAssets)
        {
            if (entry.second == nullptr)
                continue;

            if (!entry.second->consumeRuntimeDefinition(definitionOut))
                continue;

            assetPathOut = entry.first;
            return true;
        }

        assetPathOut.clear();
        return false;
    }

    const std::vector<std::string>& getAssets(AssetType type) const
    {
        static const std::vector<std::string> empty;
        const auto it = m_assetPaths.find(type);
        return it != m_assetPaths.end() ? it->second : empty;
    }
};
}