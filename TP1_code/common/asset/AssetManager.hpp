#pragma once

#include "DataAssetIO.hpp"
#include "ComponentScriptAssetIO.hpp"
#include "MaterialAssetIO.hpp"
#include "RenderPassAssetIO.hpp"
#include "RenderPhaseAssetIO.hpp"
#include "UniformFactoryAssetIO.hpp"
#include "SceneScriptAssetIO.hpp"
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
    RenderPhase,
    RenderPass,
    UniformFactory,
    Data,
    SceneScript,
    ComponentScript,
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
    std::unordered_map<std::string, RenderPhaseAssetDefinition> m_renderPhaseAssets;
    std::unordered_map<std::string, RenderPassAssetDefinition> m_renderPassAssets;
    std::unordered_map<std::string, UniformFactoryAssetDefinition> m_uniformFactoryAssets;
    std::unordered_map<std::string, DataAssetDefinition> m_dataAssets;
    std::unordered_map<std::string, SceneScriptAssetDefinition> m_sceneScriptAssets;
    std::unordered_map<std::string, ComponentScriptAssetDefinition> m_componentScriptAssets;
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

        if (!definition.renderPassPath.empty())
            loadRenderPassDefinition(definition.renderPassPath);

        Shader* shader = loadShader(definition.shaderPath);
        if (shader == nullptr)
            return nullptr;

        std::unique_ptr<dataStruct::Material> material;
        if (definition.kind == MaterialAssetKind::Unlit)
        {
            std::unique_ptr<dataStruct::UnlitMaterial> unlit = std::make_unique<dataStruct::UnlitMaterial>(shader);
            unlit->setRuntimeDefinitionHeader(definition.kind, definition.shaderPath, definition.renderPassPath);
            material = std::move(unlit);
        }
        else
        {
            std::unique_ptr<dataStruct::LitMaterial> lit = std::make_unique<dataStruct::LitMaterial>(shader);
            lit->setRuntimeDefinitionHeader(definition.kind, definition.shaderPath, definition.renderPassPath);
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

    RenderPhaseAssetDefinition* loadRenderPhaseDefinition(const std::string& relativePath)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        if (!hasExtension(normalizedPath, ".render_phase"))
        {
            std::cerr << "RenderPhase asset must use .render_phase extension: " << normalizedPath << std::endl;
            return nullptr;
        }

        registerGlobalAsset(AssetType::RenderPhase, normalizedPath);

        const auto it = m_renderPhaseAssets.find(normalizedPath);
        if (it != m_renderPhaseAssets.end())
            return &it->second;

        RenderPhaseAssetDefinition definition;
        if (!RenderPhaseAssetIO::loadDefinition(runtimePath(normalizedPath), definition))
        {
            std::cerr << "Failed to load render phase asset: " << normalizedPath << std::endl;
            return nullptr;
        }

        auto inserted = m_renderPhaseAssets.emplace(normalizedPath, std::move(definition));
        return &inserted.first->second;
    }

    bool reloadRenderPhaseDefinition(const std::string& relativePath, RenderPhaseAssetDefinition*& definitionOut)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        if (!hasExtension(normalizedPath, ".render_phase"))
        {
            definitionOut = nullptr;
            return false;
        }

        registerGlobalAsset(AssetType::RenderPhase, normalizedPath);

        RenderPhaseAssetDefinition definition;
        if (!RenderPhaseAssetIO::loadDefinition(runtimePath(normalizedPath), definition))
        {
            definitionOut = nullptr;
            return false;
        }

        auto it = m_renderPhaseAssets.find(normalizedPath);
        if (it == m_renderPhaseAssets.end())
            it = m_renderPhaseAssets.emplace(normalizedPath, std::move(definition)).first;
        else
            it->second = std::move(definition);

        definitionOut = &it->second;
        return true;
    }

    RenderPassAssetDefinition* loadRenderPassDefinition(const std::string& relativePath)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        if (!hasExtension(normalizedPath, ".render_pass"))
        {
            std::cerr << "RenderPass asset must use .render_pass extension: " << normalizedPath << std::endl;
            return nullptr;
        }

        registerGlobalAsset(AssetType::RenderPass, normalizedPath);

        const auto it = m_renderPassAssets.find(normalizedPath);
        if (it != m_renderPassAssets.end())
            return &it->second;

        RenderPassAssetDefinition definition;
        if (!RenderPassAssetIO::loadDefinition(runtimePath(normalizedPath), definition))
        {
            std::cerr << "Failed to load render pass asset: " << normalizedPath << std::endl;
            return nullptr;
        }

        auto inserted = m_renderPassAssets.emplace(normalizedPath, std::move(definition));
        return &inserted.first->second;
    }

    bool reloadRenderPassDefinition(const std::string& relativePath, RenderPassAssetDefinition*& definitionOut)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        if (!hasExtension(normalizedPath, ".render_pass"))
        {
            definitionOut = nullptr;
            return false;
        }

        registerGlobalAsset(AssetType::RenderPass, normalizedPath);

        RenderPassAssetDefinition definition;
        if (!RenderPassAssetIO::loadDefinition(runtimePath(normalizedPath), definition))
        {
            definitionOut = nullptr;
            return false;
        }

        auto it = m_renderPassAssets.find(normalizedPath);
        if (it == m_renderPassAssets.end())
            it = m_renderPassAssets.emplace(normalizedPath, std::move(definition)).first;
        else
            it->second = std::move(definition);

        definitionOut = &it->second;
        return true;
    }

    DataAssetDefinition* loadDataAssetDefinition(const std::string& relativePath)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        if (!hasExtension(normalizedPath, ".data"))
        {
            std::cerr << "Data asset must use .data extension: " << normalizedPath << std::endl;
            return nullptr;
        }

        registerGlobalAsset(AssetType::Data, normalizedPath);

        const auto it = m_dataAssets.find(normalizedPath);
        if (it != m_dataAssets.end())
            return &it->second;

        DataAssetDefinition definition;
        if (!DataAssetIO::loadDefinition(runtimePath(normalizedPath), definition))
        {
            std::cerr << "Failed to load data asset: " << normalizedPath << std::endl;
            return nullptr;
        }

        auto inserted = m_dataAssets.emplace(normalizedPath, std::move(definition));
        return &inserted.first->second;
    }

    UniformFactoryAssetDefinition* loadUniformFactoryDefinition(const std::string& relativePath)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        if (!hasExtension(normalizedPath, ".uniform_factory"))
        {
            std::cerr << "UniformFactory asset must use .uniform_factory extension: " << normalizedPath << std::endl;
            return nullptr;
        }

        registerGlobalAsset(AssetType::UniformFactory, normalizedPath);

        const auto it = m_uniformFactoryAssets.find(normalizedPath);
        if (it != m_uniformFactoryAssets.end())
            return &it->second;

        UniformFactoryAssetDefinition definition;
        if (!UniformFactoryAssetIO::loadDefinition(runtimePath(normalizedPath), definition))
        {
            std::cerr << "Failed to load uniform factory asset: " << normalizedPath << std::endl;
            return nullptr;
        }

        definition.sourcePath = normalizeRelativePath(definition.sourcePath);
        auto inserted = m_uniformFactoryAssets.emplace(normalizedPath, std::move(definition));
        return &inserted.first->second;
    }

    bool reloadUniformFactoryDefinition(const std::string& relativePath, UniformFactoryAssetDefinition*& definitionOut)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        if (!hasExtension(normalizedPath, ".uniform_factory"))
        {
            definitionOut = nullptr;
            return false;
        }

        registerGlobalAsset(AssetType::UniformFactory, normalizedPath);

        UniformFactoryAssetDefinition definition;
        if (!UniformFactoryAssetIO::loadDefinition(runtimePath(normalizedPath), definition))
        {
            definitionOut = nullptr;
            return false;
        }

        definition.sourcePath = normalizeRelativePath(definition.sourcePath);
        auto it = m_uniformFactoryAssets.find(normalizedPath);
        if (it == m_uniformFactoryAssets.end())
            it = m_uniformFactoryAssets.emplace(normalizedPath, std::move(definition)).first;
        else
            it->second = std::move(definition);

        definitionOut = &it->second;
        return true;
    }

    bool reloadDataAssetDefinition(const std::string& relativePath, DataAssetDefinition*& definitionOut)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        if (!hasExtension(normalizedPath, ".data"))
        {
            definitionOut = nullptr;
            return false;
        }

        registerGlobalAsset(AssetType::Data, normalizedPath);

        DataAssetDefinition definition;
        if (!DataAssetIO::loadDefinition(runtimePath(normalizedPath), definition))
        {
            definitionOut = nullptr;
            return false;
        }

        auto it = m_dataAssets.find(normalizedPath);
        if (it == m_dataAssets.end())
            it = m_dataAssets.emplace(normalizedPath, std::move(definition)).first;
        else
            it->second = std::move(definition);

        definitionOut = &it->second;
        return true;
    }

    SceneScriptAssetDefinition* loadSceneScriptAssetDefinition(const std::string& relativePath)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        if (!hasExtension(normalizedPath, ".scene_script"))
        {
            std::cerr << "SceneScript asset must use .scene_script extension: " << normalizedPath << std::endl;
            return nullptr;
        }

        registerGlobalAsset(AssetType::SceneScript, normalizedPath);

        const auto it = m_sceneScriptAssets.find(normalizedPath);
        if (it != m_sceneScriptAssets.end())
            return &it->second;

        SceneScriptAssetDefinition definition;
        if (!SceneScriptAssetIO::loadDefinition(runtimePath(normalizedPath), definition))
        {
            std::cerr << "Failed to load scene script asset: " << normalizedPath << std::endl;
            return nullptr;
        }

        definition.sourcePath = normalizeRelativePath(definition.sourcePath);
        auto inserted = m_sceneScriptAssets.emplace(normalizedPath, std::move(definition));
        return &inserted.first->second;
    }

    bool reloadSceneScriptAssetDefinition(const std::string& relativePath, SceneScriptAssetDefinition*& definitionOut)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        if (!hasExtension(normalizedPath, ".scene_script"))
        {
            definitionOut = nullptr;
            return false;
        }

        registerGlobalAsset(AssetType::SceneScript, normalizedPath);

        SceneScriptAssetDefinition definition;
        if (!SceneScriptAssetIO::loadDefinition(runtimePath(normalizedPath), definition))
        {
            definitionOut = nullptr;
            return false;
        }

        definition.sourcePath = normalizeRelativePath(definition.sourcePath);
        auto it = m_sceneScriptAssets.find(normalizedPath);
        if (it == m_sceneScriptAssets.end())
            it = m_sceneScriptAssets.emplace(normalizedPath, std::move(definition)).first;
        else
            it->second = std::move(definition);

        definitionOut = &it->second;
        return true;
    }

    ComponentScriptAssetDefinition* loadComponentScriptAssetDefinition(const std::string& relativePath)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        if (!hasExtension(normalizedPath, ".component_script"))
        {
            std::cerr << "ComponentScript asset must use .component_script extension: " << normalizedPath << std::endl;
            return nullptr;
        }

        registerGlobalAsset(AssetType::ComponentScript, normalizedPath);

        const auto it = m_componentScriptAssets.find(normalizedPath);
        if (it != m_componentScriptAssets.end())
            return &it->second;

        ComponentScriptAssetDefinition definition;
        if (!ComponentScriptAssetIO::loadDefinition(runtimePath(normalizedPath), definition))
        {
            std::cerr << "Failed to load component script asset: " << normalizedPath << std::endl;
            return nullptr;
        }

        definition.sourcePath = normalizeRelativePath(definition.sourcePath);
        auto inserted = m_componentScriptAssets.emplace(normalizedPath, std::move(definition));
        return &inserted.first->second;
    }

    bool reloadComponentScriptAssetDefinition(const std::string& relativePath, ComponentScriptAssetDefinition*& definitionOut)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        if (!hasExtension(normalizedPath, ".component_script"))
        {
            definitionOut = nullptr;
            return false;
        }

        registerGlobalAsset(AssetType::ComponentScript, normalizedPath);

        ComponentScriptAssetDefinition definition;
        if (!ComponentScriptAssetIO::loadDefinition(runtimePath(normalizedPath), definition))
        {
            definitionOut = nullptr;
            return false;
        }

        definition.sourcePath = normalizeRelativePath(definition.sourcePath);
        auto it = m_componentScriptAssets.find(normalizedPath);
        if (it == m_componentScriptAssets.end())
            it = m_componentScriptAssets.emplace(normalizedPath, std::move(definition)).first;
        else
            it->second = std::move(definition);

        definitionOut = &it->second;
        return true;
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