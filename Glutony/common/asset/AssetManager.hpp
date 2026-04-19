#pragma once

#include "DataAssetIO.hpp"
#include "ComponentScriptAssetIO.hpp"
#include "MaterialAssetIO.hpp"
#include "RenderPassAssetIO.hpp"
#include "RenderPhaseAssetIO.hpp"
#include "TextureAssetIO.hpp"
#include "UniformFactoryAssetIO.hpp"
#include "SceneScriptAssetIO.hpp"
#include "common/FileLoader.hpp"
#include "common/app/RuntimePaths.hpp"
#include "common/gameobject/component/Mesh.hpp"
#include "common/shader/LitMaterial.hpp"
#include "common/shader/Shader.hpp"
#include "common/shader/UnlitMaterial.hpp"
#include "external/stb_image/stb_image.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <exception>
#include <filesystem>
#include <future>
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
    Texture,
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
    enum class AsyncJobState
    {
        Idle,
        Queued,
        Running,
    };

    enum class TextureCpuFormat
    {
        None,
        Rgba8,
        RFloat,
    };

    struct MeshAsyncState
    {
        AsyncJobState jobState = AsyncJobState::Idle;
        std::filesystem::file_time_type requestedWriteTime = std::filesystem::file_time_type::min();
        std::filesystem::file_time_type loadingWriteTime = std::filesystem::file_time_type::min();
        std::future<std::unique_ptr<component::Mesh>> future;
    };

    struct TextureCpuPayload
    {
        TextureCpuFormat format = TextureCpuFormat::None;
        int width = 0;
        int height = 0;
        std::vector<unsigned char> rgba8Pixels;
        std::vector<float> rFloatPixels;
        bool success = false;
    };

    struct TextureAsyncState
    {
        AsyncJobState jobState = AsyncJobState::Idle;
        std::filesystem::file_time_type requestedWriteTime = std::filesystem::file_time_type::min();
        std::filesystem::file_time_type loadingWriteTime = std::filesystem::file_time_type::min();
        std::filesystem::file_time_type appliedWriteTime = std::filesystem::file_time_type::min();
        std::future<TextureCpuPayload> future;
        TextureCpuPayload pendingUpload;
        bool hasPendingUpload = false;
        GLuint textureId = 0;
    };

    static constexpr std::size_t MaxConcurrentMeshLoads = 2;
    static constexpr std::size_t MaxConcurrentTextureLoads = 2;
    static constexpr std::size_t MaxMeshCommitsPerFrame = 2;
    static constexpr std::size_t MaxTextureUploadsPerFrame = 1;

    std::unordered_map<std::string, component::Mesh*> m_meshAssets;
    std::unordered_map<std::string, std::filesystem::file_time_type> m_meshWriteTimes;
    std::unordered_map<std::string, MeshAsyncState> m_meshAsyncStates;
    std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaderAssets;
    std::unordered_map<std::string, std::unique_ptr<dataStruct::Material>> m_materialAssets;
    std::unordered_map<std::string, RenderPhaseAssetDefinition> m_renderPhaseAssets;
    std::unordered_map<std::string, RenderPassAssetDefinition> m_renderPassAssets;
    std::unordered_map<std::string, UniformFactoryAssetDefinition> m_uniformFactoryAssets;
    std::unordered_map<std::string, DataAssetDefinition> m_dataAssets;
    std::unordered_map<std::string, SceneScriptAssetDefinition> m_sceneScriptAssets;
    std::unordered_map<std::string, ComponentScriptAssetDefinition> m_componentScriptAssets;
    std::unordered_map<std::string, std::string> m_materialShaderPaths;
    std::unordered_map<std::string, TextureAsyncState> m_textureAsyncStates;
    std::unordered_set<std::string> m_pendingChangedShaderPaths;
    std::unordered_map<AssetType, std::vector<std::string>, AssetTypeHash> m_assetPaths;
    GLuint m_fallbackColorTextureId = 0;
    GLuint m_fallbackFloatTextureId = 0;

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

        material->setRuntimePreviewSyncEnabled(false);

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
            {
                bool hasTexture = false;
                if (!uniform.textureAssetPath.empty())
                {
                    if (!resolveTextureRuntimePath(uniform.textureAssetPath).empty())
                    {
                        requestTextureAssetPrefetch(uniform.textureAssetPath);
                        material->addTextureAsset(uniform.name, uniform.textureAssetPath);
                        hasTexture = true;
                    }
                }
                material->addBoolUniform(uniform.name + "_present", hasTexture);
                break;
            }
            }
        }

        material->setRuntimePreviewSyncEnabled(true);

        material->setAssetPath(normalizedPath);
        return material;
    }

    void registerGlobalAsset(AssetType type, const std::string& relativePath)
    {
        std::vector<std::string>& paths = m_assetPaths[type];
        if (std::find(paths.begin(), paths.end(), relativePath) == paths.end())
            paths.push_back(relativePath);
    }

    static std::filesystem::file_time_type safeLastWriteTime(const std::string& path)
    {
        std::error_code errorCode;
        const std::filesystem::file_time_type writeTime = std::filesystem::last_write_time(path, errorCode);
        if (errorCode)
            return std::filesystem::file_time_type::min();
        return writeTime;
    }

    std::unique_ptr<component::Mesh> loadMeshFromDisk(const std::string& normalizedPath)
    {
        const std::string diskPath = runtimePath(normalizedPath);

        try
        {
            std::unique_ptr<component::Mesh> mesh(fileLoader::loadModelFile(diskPath));
            if (mesh != nullptr)
                mesh->setAssetPath(normalizedPath);
            return mesh;
        }
        catch (const std::exception& exception)
        {
            std::cerr << "Failed to load mesh asset: " << normalizedPath << " (" << exception.what() << ")" << std::endl;
        }
        catch (const char* message)
        {
            std::cerr << "Failed to load mesh asset: " << normalizedPath << " (" << message << ")" << std::endl;
        }
        catch (...)
        {
            std::cerr << "Failed to load mesh asset: " << normalizedPath << std::endl;
        }

        return nullptr;
    }

    bool reloadMeshAssetInPlace(const std::string& normalizedPath,
                                component::Mesh& existingMesh,
                                const std::filesystem::file_time_type& writeTime)
    {
        std::unique_ptr<component::Mesh> reloadedMesh = loadMeshFromDisk(normalizedPath);
        m_meshWriteTimes[normalizedPath] = writeTime;
        if (reloadedMesh == nullptr)
        {
            std::cerr << "Mesh reload failed for " << normalizedPath << ". Keeping previous valid mesh." << std::endl;
            return false;
        }

        existingMesh.replaceGeometryFrom(*reloadedMesh);
        existingMesh.setAssetPath(normalizedPath);
        return true;
    }

    static std::string lowercasePathExtension(const std::string& rawPath)
    {
        std::string extension = std::filesystem::path(normalizeRelativePath(rawPath)).extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
        return extension;
    }

    std::size_t runningMeshLoadCount() const
    {
        std::size_t count = 0;
        for (const auto& entry : m_meshAsyncStates)
        {
            if (entry.second.jobState == AsyncJobState::Running)
                ++count;
        }
        return count;
    }

    std::size_t runningTextureLoadCount() const
    {
        std::size_t count = 0;
        for (const auto& entry : m_textureAsyncStates)
        {
            if (entry.second.jobState == AsyncJobState::Running)
                ++count;
        }
        return count;
    }

    void queueMeshLoad(const std::string& normalizedPath, const std::filesystem::file_time_type& writeTime)
    {
        MeshAsyncState& state = m_meshAsyncStates[normalizedPath];
        state.requestedWriteTime = writeTime;
        if (state.jobState != AsyncJobState::Running)
            state.jobState = AsyncJobState::Queued;
    }

    void startQueuedMeshLoads()
    {
        std::size_t runningCount = runningMeshLoadCount();
        for (auto& entry : m_meshAsyncStates)
        {
            if (runningCount >= MaxConcurrentMeshLoads)
                break;

            MeshAsyncState& state = entry.second;
            if (state.jobState != AsyncJobState::Queued)
                continue;

            state.loadingWriteTime = state.requestedWriteTime;
            state.future = std::async(std::launch::async, [this, normalizedPath = entry.first]() {
                return loadMeshFromDisk(normalizedPath);
            });
            state.jobState = AsyncJobState::Running;
            ++runningCount;
        }
    }

    void finalizeCompletedMeshLoads()
    {
        std::size_t committedCount = 0;
        for (auto& entry : m_meshAsyncStates)
        {
            if (committedCount >= MaxMeshCommitsPerFrame)
                break;

            MeshAsyncState& state = entry.second;
            if (state.jobState != AsyncJobState::Running || !state.future.valid())
                continue;

            if (state.future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
                continue;

            component::Mesh* existingMesh = nullptr;
            const auto meshIt = m_meshAssets.find(entry.first);
            if (meshIt != m_meshAssets.end())
                existingMesh = meshIt->second;

            std::unique_ptr<component::Mesh> loadedMesh = state.future.get();
            m_meshWriteTimes[entry.first] = state.loadingWriteTime;
            if (existingMesh != nullptr && loadedMesh != nullptr)
            {
                existingMesh->replaceGeometryFrom(*loadedMesh);
                existingMesh->setAssetPath(entry.first);
            }
            else if (loadedMesh == nullptr && existingMesh != nullptr && existingMesh->verticesCount() != 0)
                std::cerr << "Mesh reload failed for " << entry.first << ". Keeping previous valid mesh." << std::endl;

            state.jobState = AsyncJobState::Idle;
            if (state.requestedWriteTime != state.loadingWriteTime)
                state.jobState = AsyncJobState::Queued;
            ++committedCount;
        }
    }

    TextureCpuPayload loadTexturePayloadFromDisk(const std::string& normalizedPath)
    {
        TextureCpuPayload payload;
        const std::string diskPath = runtimePath(normalizedPath);
        const std::string extension = lowercasePathExtension(normalizedPath);

        if (extension == ".rttex")
        {
            if (!TextureAssetIO::loadRFloatTexture(diskPath, payload.width, payload.height, payload.rFloatPixels))
                return payload;

            payload.format = TextureCpuFormat::RFloat;
            payload.success = payload.width > 0 && payload.height > 0 && !payload.rFloatPixels.empty();
            return payload;
        }

        int channels = 0;
        unsigned char* rawPixels = stbi_load(diskPath.c_str(), &payload.width, &payload.height, &channels, 4);
        if (rawPixels == nullptr)
            return payload;

        payload.format = TextureCpuFormat::Rgba8;
        const std::size_t byteCount = static_cast<std::size_t>(payload.width) * static_cast<std::size_t>(payload.height) * 4U;
        payload.rgba8Pixels.assign(rawPixels, rawPixels + byteCount);
        payload.success = payload.width > 0 && payload.height > 0 && !payload.rgba8Pixels.empty();
        stbi_image_free(rawPixels);
        return payload;
    }

    void queueTextureLoad(const std::string& normalizedPath, const std::filesystem::file_time_type& writeTime)
    {
        TextureAsyncState& state = m_textureAsyncStates[normalizedPath];
        if (state.textureId != 0 && state.appliedWriteTime == writeTime)
            return;

        if (state.jobState == AsyncJobState::Running && state.loadingWriteTime == writeTime)
        {
            state.requestedWriteTime = writeTime;
            return;
        }

        if (state.hasPendingUpload && state.loadingWriteTime == writeTime)
        {
            state.requestedWriteTime = writeTime;
            return;
        }

        if (state.jobState == AsyncJobState::Idle && !state.hasPendingUpload && state.appliedWriteTime == writeTime)
            return;

        state.requestedWriteTime = writeTime;
        if (state.jobState != AsyncJobState::Running)
            state.jobState = AsyncJobState::Queued;
    }

    void startQueuedTextureLoads()
    {
        std::size_t runningCount = runningTextureLoadCount();
        for (auto& entry : m_textureAsyncStates)
        {
            if (runningCount >= MaxConcurrentTextureLoads)
                break;

            TextureAsyncState& state = entry.second;
            if (state.jobState != AsyncJobState::Queued)
                continue;

            state.loadingWriteTime = state.requestedWriteTime;
            state.future = std::async(std::launch::async, [this, normalizedPath = entry.first]() {
                return loadTexturePayloadFromDisk(normalizedPath);
            });
            state.jobState = AsyncJobState::Running;
            ++runningCount;
        }
    }

    GLuint ensureFallbackColorTexture()
    {
        if (m_fallbackColorTextureId != 0)
            return m_fallbackColorTextureId;

        const unsigned char pixels[4] = {255, 255, 255, 255};
        glGenTextures(1, &m_fallbackColorTextureId);
        glBindTexture(GL_TEXTURE_2D, m_fallbackColorTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glBindTexture(GL_TEXTURE_2D, 0);
        return m_fallbackColorTextureId;
    }

    GLuint ensureFallbackFloatTexture()
    {
        if (m_fallbackFloatTextureId != 0)
            return m_fallbackFloatTextureId;

        const float pixel = 0.0f;
        glGenTextures(1, &m_fallbackFloatTextureId);
        glBindTexture(GL_TEXTURE_2D, m_fallbackFloatTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 1, 1, 0, GL_RED, GL_FLOAT, &pixel);
        glBindTexture(GL_TEXTURE_2D, 0);
        return m_fallbackFloatTextureId;
    }

    GLuint uploadTexturePayload(const TextureCpuPayload& payload) const
    {
        if (!payload.success || payload.width <= 0 || payload.height <= 0)
            return 0;

        GLuint textureId = 0;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        if (payload.format == TextureCpuFormat::RFloat)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_R32F,
                payload.width,
                payload.height,
                0,
                GL_RED,
                GL_FLOAT,
                payload.rFloatPixels.data());
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA,
                payload.width,
                payload.height,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                payload.rgba8Pixels.data());
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        return textureId;
    }

    void harvestCompletedTextureLoads()
    {
        for (auto& entry : m_textureAsyncStates)
        {
            TextureAsyncState& state = entry.second;
            if (state.jobState != AsyncJobState::Running || !state.future.valid())
                continue;

            if (state.future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
                continue;

            state.pendingUpload = state.future.get();
            state.hasPendingUpload = true;
            state.jobState = AsyncJobState::Idle;
            if (state.requestedWriteTime != state.loadingWriteTime)
                state.jobState = AsyncJobState::Queued;
        }
    }

    void uploadReadyTextures()
    {
        std::size_t uploadCount = 0;
        for (auto& entry : m_textureAsyncStates)
        {
            if (uploadCount >= MaxTextureUploadsPerFrame)
                break;

            TextureAsyncState& state = entry.second;
            if (!state.hasPendingUpload)
                continue;

            if (state.pendingUpload.success)
            {
                const GLuint newTextureId = uploadTexturePayload(state.pendingUpload);
                if (newTextureId != 0)
                {
                    if (state.textureId != 0)
                        glDeleteTextures(1, &state.textureId);
                    state.textureId = newTextureId;
                }
            }

            state.appliedWriteTime = state.loadingWriteTime;
            state.pendingUpload = TextureCpuPayload();
            state.hasPendingUpload = false;
            ++uploadCount;
        }
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
        return runtime_app::runtimePath(normalizeRelativePath(relativePath));
    }

    static bool hasExtension(const std::string& path, const std::string& extension)
    {
        if (path.size() < extension.size())
            return false;

        return path.compare(path.size() - extension.size(), extension.size(), extension) == 0;
    }

    static bool isTextureAssetPath(const std::string& path)
    {
        std::string extension = std::filesystem::path(normalizeRelativePath(path)).extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });

        return extension == ".png" ||
               extension == ".jpg" ||
               extension == ".jpeg" ||
               extension == ".bmp" ||
             extension == ".tga" ||
             extension == ".rttex";
    }

    std::string resolveTextureRuntimePath(const std::string& relativePath, bool logErrors = true)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        if (normalizedPath.empty())
            return "";

        if (!isTextureAssetPath(normalizedPath))
        {
            if (logErrors)
                std::cerr << "Texture asset must use a supported image extension: " << normalizedPath << std::endl;
            return "";
        }

        registerGlobalAsset(AssetType::Texture, normalizedPath);

        const std::string diskPath = runtimePath(normalizedPath);
        std::error_code errorCode;
        if (!std::filesystem::exists(diskPath, errorCode) || errorCode)
        {
            if (logErrors)
                std::cerr << "Failed to resolve texture asset: " << normalizedPath << std::endl;
            return "";
        }

        return diskPath;
    }

    void requestTextureAssetPrefetch(const std::string& relativePath)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        if (normalizedPath.empty() || !isTextureAssetPath(normalizedPath))
            return;

        registerGlobalAsset(AssetType::Texture, normalizedPath);
        queueTextureLoad(normalizedPath, safeLastWriteTime(runtimePath(normalizedPath)));
    }

    GLuint resolveTextureAssetTextureId(const std::string& relativePath, bool allowFallback = true)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        if (normalizedPath.empty() || !isTextureAssetPath(normalizedPath))
            return 0;

        registerGlobalAsset(AssetType::Texture, normalizedPath);

        auto it = m_textureAsyncStates.find(normalizedPath);
        if (it == m_textureAsyncStates.end())
        {
            queueTextureLoad(normalizedPath, safeLastWriteTime(runtimePath(normalizedPath)));
            it = m_textureAsyncStates.find(normalizedPath);
        }

        if (it != m_textureAsyncStates.end() && it->second.textureId != 0)
            return it->second.textureId;

        if (!allowFallback)
            return 0;

        return lowercasePathExtension(normalizedPath) == ".rttex"
            ? ensureFallbackFloatTexture()
            : ensureFallbackColorTexture();
    }

    component::Mesh* loadMesh(const std::string& relativePath)
    {
        const std::string normalizedPath = normalizeRelativePath(relativePath);
        registerGlobalAsset(AssetType::Mesh, normalizedPath);

        const auto it = m_meshAssets.find(normalizedPath);
        if (it != m_meshAssets.end())
            return it->second;

        std::unique_ptr<component::Mesh> placeholder = std::make_unique<component::Mesh>();
        placeholder->setAssetPath(normalizedPath);
        component::Mesh* meshPtr = placeholder.get();
        meshPtr->ownerCount++;
        m_meshAssets[normalizedPath] = placeholder.release();
        m_meshWriteTimes[normalizedPath] = std::filesystem::file_time_type::min();
        queueMeshLoad(normalizedPath, safeLastWriteTime(runtimePath(normalizedPath)));
        return meshPtr;
    }

    void refreshLoadedMeshesIfSourcesChanged()
    {
        for (auto& entry : m_meshAssets)
        {
            component::Mesh* mesh = entry.second;
            if (mesh == nullptr)
                continue;

            const std::string diskPath = runtimePath(entry.first);
            const std::filesystem::file_time_type currentWriteTime = safeLastWriteTime(diskPath);
            auto observedWriteTime = m_meshWriteTimes.find(entry.first);
            if (observedWriteTime == m_meshWriteTimes.end())
            {
                m_meshWriteTimes[entry.first] = currentWriteTime;
                continue;
            }

            if (observedWriteTime->second == currentWriteTime)
                continue;

            queueMeshLoad(entry.first, currentWriteTime);
        }
    }

    void refreshLoadedTextureAssetsIfSourcesChanged()
    {
        for (auto& entry : m_textureAsyncStates)
        {
            const std::filesystem::file_time_type currentWriteTime = safeLastWriteTime(runtimePath(entry.first));
            if (entry.second.appliedWriteTime == currentWriteTime && entry.second.requestedWriteTime == currentWriteTime)
                continue;

            queueTextureLoad(entry.first, currentWriteTime);
        }
    }

    void pumpAsyncLoads()
    {
        finalizeCompletedMeshLoads();
        harvestCompletedTextureLoads();
        uploadReadyTextures();
        startQueuedMeshLoads();
        startQueuedTextureLoads();
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
