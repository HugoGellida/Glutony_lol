#pragma once

#include <GL/glew.h>

#include "../asset/AssetManager.hpp"
#include "../asset/TextureAssetIO.hpp"
#include "../gameobject/GameObject.hpp"
#include "../gameobject/component/MeshRenderer.hpp"
#include "../shader/Material.hpp"
#include "RenderLightData.hpp"
#include "RenderTargetResource.hpp"
#include "SceneRenderTargetSettings.hpp"
#include "../physics/RigidBody.hpp"

#include <gameplay/GameplayEntry.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace render
{
class RenderPipeline
{
private:
    struct RenderTargetMetadata
    {
        bool bakeable = false;
        RenderTargetBakeCombineOp bakeCombine = RenderTargetBakeCombineOp::Multiply;
        RenderTargetFormat format = RenderTargetFormat::Rgba;
        bool hasFormat = false;
    };

    struct DrawItem
    {
        component::MeshRenderer* renderer = nullptr;
        Transform* transform = nullptr;
        std::string materialAssetPath;
        dataStruct::Material* fallbackMaterial = nullptr;
    };

    struct CachedTextureAsset
    {
        Texture2D texture;
        std::filesystem::file_time_type writeTime = std::filesystem::file_time_type::min();
    };

public:
    struct BakedRenderTargetResult
    {
        std::string targetName;
        std::string targetGroupSuffix;
        RenderTargetBakeCombineOp bakeCombine = RenderTargetBakeCombineOp::Multiply;
        RenderTargetFormat format = RenderTargetFormat::Rgba;
        int width = 0;
        int height = 0;
        std::vector<float> pixelsFloat;
        std::vector<unsigned char> pixelsRgba8;
    };

private:
    enum class DrawFilter
    {
        All,
        StaticOnly,
    };

    struct ExecutionOptions
    {
        DrawFilter drawFilter = DrawFilter::All;
        bool allowBakedInputs = true;
        bool dynamicOnlyOnBakeableTargets = false;
        std::vector<BakedRenderTargetResult>* bakedOutputs = nullptr;
    };

    struct Batch
    {
        std::string phaseAssetPath;
        std::string signature;
        asset::RenderPassStepDefinition pass;
        asset::RenderPassIterator iterator = asset::RenderPassIterator::None;
        std::string targetLogicalKey;
        std::string targetInstanceKey;
        std::vector<DrawItem> items;
    };

    std::vector<std::string> m_phaseOrder;
    std::vector<Batch> m_batches;
    std::unordered_map<std::string, RenderTargetResource> m_renderTargets;
    std::unordered_map<std::string, RenderTargetResource> m_compositedRenderTargets;
    std::unordered_map<std::string, RenderTargetMetadata> m_renderTargetMetadata;
    std::unordered_map<std::string, CachedTextureAsset> m_cachedTextureAssets;
    std::unordered_map<std::string, std::filesystem::file_time_type> m_trackedAssetWriteTimes;
    std::size_t m_structureHash = 0;
    bool m_compiled = false;
    bool m_failed = false;

    static GLuint fullscreenTriangleVao()
    {
        static GLuint vao = []() {
            GLuint created = 0;
            glGenVertexArrays(1, &created);
            return created;
        }();

        return vao;
    }

    static GLuint bakedCompositeProgram()
    {
        static GLuint program = []() {
            const char* vertexSource =
                "#version 330 core\n"
                "out vec2 uv;\n"
                "void main()\n"
                "{\n"
                "    vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));\n"
                "    vec2 position = positions[gl_VertexID];\n"
                "    uv = position * 0.5 + 0.5;\n"
                "    gl_Position = vec4(position, 0.0, 1.0);\n"
                "}\n";
            const char* fragmentSource =
                "#version 330 core\n"
                "in vec2 uv;\n"
                "uniform sampler2D _bakedTex;\n"
                "uniform sampler2D _dynamicTex;\n"
                "uniform int _combineOp;\n"
                "out vec4 color;\n"
                "void main()\n"
                "{\n"
                "    vec4 bakedValue = texture(_bakedTex, uv);\n"
                "    vec4 dynamicValue = texture(_dynamicTex, uv);\n"
                "    if (_combineOp == 1)\n"
                "        color = bakedValue + dynamicValue;\n"
                "    else if (_combineOp == 2)\n"
                "        color = min(bakedValue, dynamicValue);\n"
                "    else if (_combineOp == 3)\n"
                "        color = max(bakedValue, dynamicValue);\n"
                "    else\n"
                "        color = bakedValue * dynamicValue;\n"
                "}\n";

            const auto compileShader = [](GLenum shaderType, const char* source) -> GLuint {
                GLuint shader = glCreateShader(shaderType);
                glShaderSource(shader, 1, &source, nullptr);
                glCompileShader(shader);

                GLint compiled = GL_FALSE;
                glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
                if (compiled == GL_TRUE)
                    return shader;

                GLint logLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
                std::string log(static_cast<size_t>(std::max(logLength, 1)), '\0');
                glGetShaderInfoLog(shader, logLength, nullptr, log.data());
                std::cerr << "[render] Failed to compile baked composite shader: " << log << std::endl;
                glDeleteShader(shader);
                return 0;
            };

            const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
            const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
            if (vertexShader == 0 || fragmentShader == 0)
            {
                if (vertexShader != 0)
                    glDeleteShader(vertexShader);
                if (fragmentShader != 0)
                    glDeleteShader(fragmentShader);
                return static_cast<GLuint>(0);
            }

            GLuint createdProgram = glCreateProgram();
            glAttachShader(createdProgram, vertexShader);
            glAttachShader(createdProgram, fragmentShader);
            glLinkProgram(createdProgram);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);

            GLint linked = GL_FALSE;
            glGetProgramiv(createdProgram, GL_LINK_STATUS, &linked);
            if (linked == GL_TRUE)
                return createdProgram;

            GLint logLength = 0;
            glGetProgramiv(createdProgram, GL_INFO_LOG_LENGTH, &logLength);
            std::string log(static_cast<size_t>(std::max(logLength, 1)), '\0');
            glGetProgramInfoLog(createdProgram, logLength, nullptr, log.data());
            std::cerr << "[render] Failed to link baked composite program: " << log << std::endl;
            glDeleteProgram(createdProgram);
            return static_cast<GLuint>(0);
        }();

        return program;
    }

    static void appendIssue(std::vector<std::string>* issues, const std::string& issue)
    {
        if (issues != nullptr)
            issues->push_back(issue);
    }

    static void appendUniquePath(std::vector<std::string>& values, const std::string& rawPath)
    {
        const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(rawPath);
        if (normalizedPath.empty())
            return;

        if (std::find(values.begin(), values.end(), normalizedPath) == values.end())
            values.push_back(normalizedPath);
    }

    static std::string compiledRenderPassMarkerPath(const std::string& rawRenderPassPath)
    {
        const std::string normalizedRenderPassPath = asset::AssetManager::normalizeRelativePath(rawRenderPassPath);
        if (normalizedRenderPassPath.empty())
            return "";

        return ".pipeline_cache/" + normalizedRenderPassPath + ".compiled";
    }

    static std::filesystem::file_time_type safeLastWriteTime(const std::string& assetPath)
    {
        std::error_code errorCode;
        const std::filesystem::path diskPath(asset::AssetManager::runtimePath(assetPath));
        const std::filesystem::file_time_type writeTime = std::filesystem::last_write_time(diskPath, errorCode);
        return errorCode ? std::filesystem::file_time_type::min() : writeTime;
    }

    static std::size_t hashCombine(std::size_t seed, std::size_t value)
    {
        return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U));
    }

    static dataStruct::Material* resolveMaterial(const component::MeshRenderer& renderer)
    {
        const std::string materialAssetPath = asset::AssetManager::normalizeRelativePath(renderer.getMaterialAssetPath());
        if (!materialAssetPath.empty())
            return asset::AssetManager::instance().loadMaterial(materialAssetPath);

        return renderer.getMaterial();
    }

    static dataStruct::Material* resolveMaterial(const DrawItem& item)
    {
        if (!item.materialAssetPath.empty())
            return asset::AssetManager::instance().loadMaterial(item.materialAssetPath);

        return item.fallbackMaterial;
    }

    static std::size_t computeStructureHash(GameObject* const* gameObjects, size_t gameObjectCount)
    {
        std::size_t hash = gameObjectCount;
        for (size_t index = 0; index < gameObjectCount; ++index)
        {
            GameObject* gameObject = gameObjects[index];
            if (gameObject == nullptr)
            {
                hash = hashCombine(hash, 0);
                continue;
            }

            hash = hashCombine(hash, static_cast<std::size_t>(gameObject->getId()));

            component::MeshRenderer* meshRenderer = gameObject->getComponent<component::MeshRenderer>();
            if (meshRenderer == nullptr)
            {
                hash = hashCombine(hash, 1);
                continue;
            }

            hash = hashCombine(hash, std::hash<const void*>()(meshRenderer));
            hash = hashCombine(hash, std::hash<std::string>()(meshRenderer->getMeshAssetPath()));
            hash = hashCombine(hash, std::hash<std::string>()(meshRenderer->getMaterialAssetPath()));

            const dataStruct::Material* material = resolveMaterial(*meshRenderer);
            const std::string renderPassPath = material != nullptr ? material->getRuntimeDefinition().renderPassPath : std::string();
            hash = hashCombine(hash, std::hash<std::string>()(renderPassPath));
        }

        return hash;
    }

    static bool isDrawItemStatic(const DrawItem& item)
    {
        if (item.renderer == nullptr)
            return false;

        GameObject* owner = item.renderer->getOwner();
        if (owner == nullptr)
            return false;

        const physics::RigidBody* rigidBody = owner->getComponent<physics::RigidBody>();
        return rigidBody == nullptr || rigidBody->isStatic;
    }

    static void mergeRenderTargetMetadata(
        std::unordered_map<std::string, RenderTargetMetadata>& metadata,
        const asset::RenderTargetAssetReference& reference)
    {
        const std::string normalizedName = normalizeRenderTargetName(reference.name);
        if (normalizedName.empty() || isFinalRenderTargetName(normalizedName))
            return;

        RenderTargetMetadata& entry = metadata[normalizedName];
        if (reference.bakeable)
        {
            entry.bakeable = true;
            entry.bakeCombine = reference.bakeCombine;
        }
        if (reference.hasFormat)
        {
            entry.format = reference.format;
            entry.hasFormat = true;
        }
    }

    const RenderTargetMetadata* findRenderTargetMetadata(const std::string& rawName) const
    {
        const std::string normalizedName = normalizeRenderTargetName(rawName);
        const auto it = m_renderTargetMetadata.find(normalizedName);
        return it != m_renderTargetMetadata.end() ? &it->second : nullptr;
    }

    static std::string lowercaseCopy(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
        return value;
    }

    static std::string resolveBakedTextureAssetPath(const SceneRenderTargetSettings* settings, const std::string& targetGroupSuffix)
    {
        if (settings == nullptr)
            return "";

        const std::string normalizedAssetPath = asset::AssetManager::normalizeRelativePath(settings->bakedTextureAssetPath);
        if (normalizedAssetPath.empty())
            return "";
        if (targetGroupSuffix.empty())
            return normalizedAssetPath;

        const std::filesystem::path basePath(normalizedAssetPath);
        const std::string stem = basePath.stem().string();
        const std::string extension = basePath.extension().string();
        const std::filesystem::path suffixedFile = stem + targetGroupSuffix + extension;
        const std::filesystem::path parent = basePath.parent_path();
        return asset::AssetManager::normalizeRelativePath((parent / suffixedFile).generic_string());
    }

    static bool bakedAssetPathMatchesMetadata(const std::string& assetPath, const RenderTargetMetadata* metadata)
    {
        if (assetPath.empty() || metadata == nullptr || !metadata->hasFormat)
            return !assetPath.empty();

        const std::string extension = lowercaseCopy(std::filesystem::path(assetPath).extension().string());
        if (metadata->format == RenderTargetFormat::Float)
            return extension == ".rttex";
        return extension != ".rttex";
    }

    Texture2D* resolveCachedTextureAsset(const std::string& assetPath)
    {
        const std::string normalizedAssetPath = asset::AssetManager::normalizeRelativePath(assetPath);
        if (normalizedAssetPath.empty())
            return nullptr;

        const std::filesystem::file_time_type writeTime = safeLastWriteTime(normalizedAssetPath);
        auto it = m_cachedTextureAssets.find(normalizedAssetPath);
        if (it != m_cachedTextureAssets.end() && it->second.writeTime == writeTime)
            return it->second.texture.isEmpty() ? nullptr : &it->second.texture;

        CachedTextureAsset cached;
        const std::string extension = lowercaseCopy(std::filesystem::path(normalizedAssetPath).extension().string());
        if (extension == ".rttex")
        {
            int width = 0;
            int height = 0;
            std::vector<float> pixels;
            if (!asset::TextureAssetIO::loadRFloatTexture(asset::AssetManager::runtimePath(normalizedAssetPath), width, height, pixels))
                return nullptr;

            GLuint textureId = 0;
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, pixels.data());
            glBindTexture(GL_TEXTURE_2D, 0);
            cached.texture = Texture2D(textureId, 0, true);
        }
        else
        {
            const std::string runtimeTexturePath = asset::AssetManager::instance().resolveTextureRuntimePath(normalizedAssetPath, false);
            if (runtimeTexturePath.empty())
                return nullptr;
            cached.texture = Texture2D(runtimeTexturePath, 0);
        }

        cached.writeTime = writeTime;
        it = m_cachedTextureAssets.insert_or_assign(normalizedAssetPath, std::move(cached)).first;
        return it->second.texture.isEmpty() ? nullptr : &it->second.texture;
    }

    static GLint bakeCombineUniformValue(RenderTargetBakeCombineOp op)
    {
        switch (op)
        {
        case RenderTargetBakeCombineOp::Add:
            return 1;
        case RenderTargetBakeCombineOp::Min:
            return 2;
        case RenderTargetBakeCombineOp::Max:
            return 3;
        case RenderTargetBakeCombineOp::Multiply:
        default:
            return 0;
        }
    }

    GLuint composeBakedAndDynamicTexture(
        const std::string& cacheKey,
        GLuint bakedTextureId,
        GLuint dynamicTextureId,
        int width,
        int height,
        RenderTargetFormat format,
        RenderTargetBakeCombineOp combineOp)
    {
        if (bakedTextureId == 0)
            return dynamicTextureId;
        if (dynamicTextureId == 0)
            return bakedTextureId;

        const GLuint program = bakedCompositeProgram();
        if (program == 0)
            return dynamicTextureId;

        RenderTargetResource& resource = m_compositedRenderTargets[cacheKey];
        if (!resource.ensure(width, height, format))
            return dynamicTextureId;

        GLint previousFramebuffer = 0;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousFramebuffer);
        GLint previousViewport[4] = {0, 0, 1, 1};
        glGetIntegerv(GL_VIEWPORT, previousViewport);
        GLint previousProgram = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
        GLint previousActiveTexture = 0;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
        GLint previousVao = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);

        const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        const GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
        const GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
        const GLboolean scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
        GLboolean previousDepthMask = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);

        resource.bind();
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_SCISSOR_TEST);
        glDepthMask(GL_FALSE);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        glBindVertexArray(fullscreenTriangleVao());

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, bakedTextureId);
        glUniform1i(glGetUniformLocation(program, "_bakedTex"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, dynamicTextureId);
        glUniform1i(glGetUniformLocation(program, "_dynamicTex"), 1);
        glUniform1i(glGetUniformLocation(program, "_combineOp"), bakeCombineUniformValue(combineOp));
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(static_cast<GLenum>(previousActiveTexture));
        glBindVertexArray(static_cast<GLuint>(previousVao));
        glUseProgram(static_cast<GLuint>(previousProgram));
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));
        glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
        glDepthMask(previousDepthMask);

        if (blendEnabled)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
        if (depthEnabled)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        if (cullEnabled)
            glEnable(GL_CULL_FACE);
        else
            glDisable(GL_CULL_FACE);
        if (scissorEnabled)
            glEnable(GL_SCISSOR_TEST);
        else
            glDisable(GL_SCISSOR_TEST);

        return resource.colorTextureId();
    }

    bool dependenciesChanged() const
    {
        for (const auto& entry : m_trackedAssetWriteTimes)
        {
            if (safeLastWriteTime(entry.first) != entry.second)
                return true;
        }

        return false;
    }

    static std::string renderPhaseKey(const std::string& rawPath)
    {
        return asset::AssetManager::normalizeRelativePath(rawPath);
    }

    static void appendShaderDependencyPaths(std::vector<std::string>& dependencyPaths, const std::string& rawShaderPath)
    {
        const std::string shaderPath = asset::AssetManager::normalizeRelativePath(rawShaderPath);
        if (shaderPath.empty())
            return;

        appendUniquePath(dependencyPaths, shaderPath + "/vertex.glsl");
        appendUniquePath(dependencyPaths, shaderPath + "/fragment.glsl");
    }

    static std::string resolveUniformFactorySourceDependencyPath(const std::string& rawUniformFactoryPath, const std::string& rawSourcePath)
    {
        const std::string uniformFactoryPath = asset::AssetManager::normalizeRelativePath(rawUniformFactoryPath);
        const std::string sourcePath = asset::AssetManager::normalizeRelativePath(rawSourcePath);
        if (uniformFactoryPath.empty() || sourcePath.empty())
            return "";

        const std::filesystem::path assetDirectory = std::filesystem::path(uniformFactoryPath).parent_path();
        const std::string assetRelativeSourcePath = asset::AssetManager::normalizeRelativePath((assetDirectory / sourcePath).generic_string());
        const std::string rootRelativeSourcePath = sourcePath;

        std::error_code errorCode;
        if (!rootRelativeSourcePath.empty() && std::filesystem::exists(asset::AssetManager::runtimePath(rootRelativeSourcePath), errorCode) && !errorCode)
            return rootRelativeSourcePath;

        errorCode.clear();
        if (!assetRelativeSourcePath.empty() && std::filesystem::exists(asset::AssetManager::runtimePath(assetRelativeSourcePath), errorCode) && !errorCode)
            return assetRelativeSourcePath;

        return !assetRelativeSourcePath.empty() ? assetRelativeSourcePath : rootRelativeSourcePath;
    }

    static bool collectRenderPassDependencyPaths(
        const std::string& rawRenderPassPath,
        asset::RenderPassAssetDefinition*& renderPassOut,
        std::vector<std::string>& dependencyPaths,
        std::vector<std::string>* issues)
    {
        dependencyPaths.clear();
        renderPassOut = nullptr;

        const std::string renderPassPath = asset::AssetManager::normalizeRelativePath(rawRenderPassPath);
        if (renderPassPath.empty())
        {
            appendIssue(issues, "Render pass path is empty.");
            return false;
        }

        appendUniquePath(dependencyPaths, renderPassPath);

        if (!asset::AssetManager::instance().reloadRenderPassDefinition(renderPassPath, renderPassOut) || renderPassOut == nullptr)
        {
            appendIssue(issues, "Failed to load render pass asset: " + renderPassPath);
            return false;
        }

        std::unordered_set<std::string> visitedPhasePaths;
        std::function<bool(const std::string&, size_t)> registerPhaseDependency = [&](const std::string& rawPhasePath, size_t passIndex) -> bool {
            const std::string phasePath = renderPhaseKey(rawPhasePath);
            if (phasePath.empty())
            {
                appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " has an empty phase.");
                return false;
            }

            if (!visitedPhasePaths.insert(phasePath).second)
                return true;

            asset::RenderPhaseAssetDefinition* phase = nullptr;
            if (!asset::AssetManager::instance().reloadRenderPhaseDefinition(phasePath, phase) || phase == nullptr)
            {
                appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " references an invalid render phase: " + phasePath);
                return false;
            }

            appendUniquePath(dependencyPaths, phasePath);
            bool valid = true;
            for (const std::string& includePath : phase->includes)
                valid = registerPhaseDependency(includePath, passIndex) && valid;
            for (const std::string& beforePath : phase->before)
                valid = registerPhaseDependency(beforePath, passIndex) && valid;
            for (const std::string& afterPath : phase->after)
                valid = registerPhaseDependency(afterPath, passIndex) && valid;
            return valid;
        };

        bool valid = true;
        for (size_t passIndex = 0; passIndex < renderPassOut->passes.size(); ++passIndex)
        {
            const asset::RenderPassStepDefinition& pass = renderPassOut->passes[passIndex];
            valid = registerPhaseDependency(pass.phaseName, passIndex) && valid;

            const std::string shaderPath = asset::AssetManager::normalizeRelativePath(pass.shaderPath);
            if (shaderPath.empty())
            {
                appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " has an empty shader path.");
                valid = false;
            }
            else
            {
                appendShaderDependencyPaths(dependencyPaths, shaderPath);
            }

            if (!pass.uniformFactoryPath.empty())
            {
                const std::string uniformFactoryPath = asset::AssetManager::normalizeRelativePath(pass.uniformFactoryPath);
                if (uniformFactoryPath.empty())
                {
                    appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " has an invalid uniform factory path.");
                    valid = false;
                }
                else
                {
                    asset::UniformFactoryAssetDefinition* factory = nullptr;
                    if (!asset::AssetManager::instance().reloadUniformFactoryDefinition(uniformFactoryPath, factory) || factory == nullptr)
                    {
                        appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " references an invalid uniform factory: " + uniformFactoryPath);
                        valid = false;
                    }
                    appendUniquePath(dependencyPaths, uniformFactoryPath);

                    if (factory == nullptr || factory->sourcePath.empty())
                    {
                        appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " references a uniform factory without a source file: " + uniformFactoryPath);
                        valid = false;
                    }
                    else
                    {
                        appendUniquePath(dependencyPaths, resolveUniformFactorySourceDependencyPath(uniformFactoryPath, factory->sourcePath));
                    }
                }
            }
        }

        return valid;
    }

    static bool compiledRenderPassMarkerIsCurrent(const std::string& renderPassPath, const std::vector<std::string>& dependencyPaths)
    {
        const std::string markerPath = compiledRenderPassMarkerPath(renderPassPath);
        if (markerPath.empty())
            return false;

        const std::filesystem::file_time_type markerWriteTime = safeLastWriteTime(markerPath);
        if (markerWriteTime == std::filesystem::file_time_type::min())
            return false;

        for (const std::string& dependencyPath : dependencyPaths)
        {
            const std::filesystem::file_time_type dependencyWriteTime = safeLastWriteTime(dependencyPath);
            if (dependencyWriteTime == std::filesystem::file_time_type::min() || dependencyWriteTime > markerWriteTime)
                return false;
        }

        return true;
    }

    static bool shaderUniformMatchesPassKind(const Shader::UniformDescriptor& descriptor, asset::RenderPassUniformKind kind)
    {
        if (descriptor.size != 1)
            return false;

        switch (kind)
        {
        case asset::RenderPassUniformKind::Bool:
            return descriptor.glType == GL_BOOL;
        case asset::RenderPassUniformKind::Int:
            return descriptor.glType == GL_INT;
        case asset::RenderPassUniformKind::Float:
            return descriptor.glType == GL_FLOAT;
        case asset::RenderPassUniformKind::Vec3:
            return descriptor.glType == GL_FLOAT_VEC3;
        case asset::RenderPassUniformKind::Mat4:
            return descriptor.glType == GL_FLOAT_MAT4;
        case asset::RenderPassUniformKind::Texture:
        case asset::RenderPassUniformKind::RenderTarget:
            return descriptor.glType == GL_SAMPLER_2D;
        }

        return false;
    }

    static bool shaderUniformMatchesFactoryKind(const Shader::UniformDescriptor& descriptor, asset::UniformFactoryOutputKind kind)
    {
        if (descriptor.size != 1)
            return false;

        switch (kind)
        {
        case asset::UniformFactoryOutputKind::Bool:
            return descriptor.glType == GL_BOOL;
        case asset::UniformFactoryOutputKind::Int:
            return descriptor.glType == GL_INT;
        case asset::UniformFactoryOutputKind::Float:
            return descriptor.glType == GL_FLOAT;
        case asset::UniformFactoryOutputKind::Vec3:
            return descriptor.glType == GL_FLOAT_VEC3;
        case asset::UniformFactoryOutputKind::Mat4:
            return descriptor.glType == GL_FLOAT_MAT4;
        }

        return false;
    }

    static bool validatePassUniform(
        Shader& shader,
        const asset::RenderPassUniformDefinition& uniform,
        const std::string& renderPassPath,
        size_t passIndex,
        std::vector<std::string>* issues)
    {
        Shader::UniformDescriptor descriptor;
        if (!shader.tryGetUniformDescriptor(uniform.name, descriptor, true))
        {
            appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " references a missing shader uniform: " + uniform.name);
            return false;
        }

        if (!shaderUniformMatchesPassKind(descriptor, uniform.kind))
        {
            appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " has a type mismatch for uniform: " + uniform.name);
            return false;
        }

        return true;
    }

    static bool validateUniformFactory(
        Shader& shader,
        const asset::UniformFactoryAssetDefinition& factory,
        asset::RenderPassIterator iterator,
        const std::string& renderPassPath,
        size_t passIndex,
        std::vector<std::string>* issues)
    {
        bool valid = true;
        if (factory.requiresLight && iterator != asset::RenderPassIterator::Light)
        {
            appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " uses a light-dependent factory without a light iterator.");
            valid = false;
        }

        for (const asset::UniformFactoryOutputDefinition& output : factory.outputs)
        {
            if (output.name.empty())
            {
                appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " contains a factory output with an empty name.");
                valid = false;
                continue;
            }

            Shader::UniformDescriptor descriptor;
            if (!shader.tryGetUniformDescriptor(output.name, descriptor, true))
            {
                appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " factory output does not match any active shader uniform: " + output.name);
                valid = false;
                continue;
            }

            if (!shaderUniformMatchesFactoryKind(descriptor, output.kind))
            {
                appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " factory output has a type mismatch for shader uniform: " + output.name);
                valid = false;
            }
        }

        return valid;
    }

    static bool writeCompiledRenderPassMarker(const std::string& rawRenderPassPath, const std::vector<std::string>& dependencyPaths)
    {
        const std::string markerPath = compiledRenderPassMarkerPath(rawRenderPassPath);
        if (markerPath.empty())
            return false;

        const std::filesystem::path diskPath(asset::AssetManager::runtimePath(markerPath));
        std::error_code errorCode;
        std::filesystem::create_directories(diskPath.parent_path(), errorCode);
        if (errorCode)
            return false;

        std::ofstream output(diskPath, std::ios::trunc);
        if (!output.is_open())
            return false;

        output << asset::AssetManager::normalizeRelativePath(rawRenderPassPath) << '\n';
        for (const std::string& dependencyPath : dependencyPaths)
            output << dependencyPath << '\n';
        return static_cast<bool>(output);
    }

    static std::string renderTargetLogicalKey(const asset::RenderTargetAssetReference& target)
    {
        return normalizeRenderTargetName(target.name) + '|' + (target.shared ? '1' : '0');
    }

    static std::string appendTargetGroupSuffix(const std::string& key, const std::string& targetGroupSuffix)
    {
        return targetGroupSuffix.empty() ? key : key + targetGroupSuffix;
    }

    static std::string resolveBatchTargetInstanceKey(const Batch& batch, const std::string& targetGroupSuffix)
    {
        if (!batch.targetInstanceKey.empty())
            return appendTargetGroupSuffix(batch.targetInstanceKey, targetGroupSuffix);

        return appendTargetGroupSuffix(renderTargetLogicalKey(batch.pass.target), targetGroupSuffix);
    }

    static const SceneRenderTargetSettings* findRenderTargetSettings(const std::vector<SceneRenderTargetSettings>& settings, const std::string& rawName)
    {
        const std::string normalizedName = normalizeRenderTargetName(rawName);
        for (const SceneRenderTargetSettings& entry : settings)
        {
            if (normalizeRenderTargetName(entry.name) == normalizedName)
                return &entry;
        }

        return nullptr;
    }

    static std::string renderPassBatchSignature(const asset::RenderPassStepDefinition& pass)
    {
        std::ostringstream stream;
        stream << pass.phaseName << '|'
               << asset::AssetManager::normalizeRelativePath(pass.shaderPath) << '|'
               << pass.target.name << '|'
               << (pass.target.shared ? '1' : '0') << '|'
             << (pass.target.bakeable ? '1' : '0') << '|'
             << static_cast<int>(pass.target.bakeCombine) << '|'
               << (pass.clearColor ? '1' : '0') << '|'
               << static_cast<int>(pass.depthAction) << '|'
               << (pass.blend.enabled ? '1' : '0') << '|'
               << (pass.blend.separateAlpha ? '1' : '0') << '|'
               << static_cast<int>(pass.iterator) << '|'
               << asset::AssetManager::normalizeRelativePath(pass.uniformFactoryPath);

        if (pass.blend.enabled)
        {
            stream << '|'
                   << static_cast<int>(pass.blend.rgb.op) << ':'
                   << static_cast<int>(pass.blend.rgb.src) << ':'
                   << static_cast<int>(pass.blend.rgb.dst) << '|'
                   << static_cast<int>(pass.blend.a.op) << ':'
                   << static_cast<int>(pass.blend.a.src) << ':'
                   << static_cast<int>(pass.blend.a.dst);
        }

        for (const asset::RenderPassUniformDefinition& uniform : pass.uniforms)
        {
            stream << '|'
                   << uniform.name << ':'
                   << static_cast<int>(uniform.kind) << ':';

            switch (uniform.kind)
            {
            case asset::RenderPassUniformKind::Bool:
                stream << (uniform.boolValue ? '1' : '0');
                break;
            case asset::RenderPassUniformKind::Int:
                stream << uniform.intValue;
                break;
            case asset::RenderPassUniformKind::Float:
                stream << uniform.floatValue;
                break;
            case asset::RenderPassUniformKind::Vec3:
                stream << uniform.vec3Value.x << ',' << uniform.vec3Value.y << ',' << uniform.vec3Value.z;
                break;
            case asset::RenderPassUniformKind::Mat4:
                for (int column = 0; column < 4; ++column)
                {
                    for (int row = 0; row < 4; ++row)
                    {
                        if (column != 0 || row != 0)
                            stream << ',';
                        stream << uniform.mat4Value[column][row];
                    }
                }
                break;
            case asset::RenderPassUniformKind::Texture:
                stream << uniform.assetPath;
                break;
            case asset::RenderPassUniformKind::RenderTarget:
                stream << uniform.renderTargetValue.name << ':'
                       << (uniform.renderTargetValue.shared ? '1' : '0') << ':'
                       << (uniform.renderTargetValue.bakeable ? '1' : '0') << ':'
                       << static_cast<int>(uniform.renderTargetValue.bakeCombine);
                break;
            }
        }

        return stream.str();
    }

    static std::string batchIterationGroupKey(const Batch& batch)
    {
        if (batch.iterator == asset::RenderPassIterator::None)
            return "";

        const std::string uniformFactoryPath = asset::AssetManager::normalizeRelativePath(batch.pass.uniformFactoryPath);
        if (uniformFactoryPath.empty())
            return "";

        asset::UniformFactoryAssetDefinition* factory = nullptr;
        if (!asset::AssetManager::instance().reloadUniformFactoryDefinition(uniformFactoryPath, factory) || factory == nullptr)
            return std::to_string(static_cast<int>(batch.iterator)) + '|' + uniformFactoryPath;

        return std::to_string(static_cast<int>(batch.iterator)) + '|'
            + asset::AssetManager::normalizeRelativePath(factory->sourcePath) + '|'
            + factory->iterationEntryName;
    }

    static GLenum toBlendFactor(asset::RenderBlendFactor factor)
    {
        switch (factor)
        {
        case asset::RenderBlendFactor::Zero:
            return GL_ZERO;
        case asset::RenderBlendFactor::One:
            return GL_ONE;
        case asset::RenderBlendFactor::SrcColor:
            return GL_SRC_COLOR;
        case asset::RenderBlendFactor::OneMinusSrcColor:
            return GL_ONE_MINUS_SRC_COLOR;
        case asset::RenderBlendFactor::DstColor:
            return GL_DST_COLOR;
        case asset::RenderBlendFactor::OneMinusDstColor:
            return GL_ONE_MINUS_DST_COLOR;
        case asset::RenderBlendFactor::SrcAlpha:
            return GL_SRC_ALPHA;
        case asset::RenderBlendFactor::OneMinusSrcAlpha:
            return GL_ONE_MINUS_SRC_ALPHA;
        case asset::RenderBlendFactor::DstAlpha:
            return GL_DST_ALPHA;
        case asset::RenderBlendFactor::OneMinusDstAlpha:
            return GL_ONE_MINUS_DST_ALPHA;
        }

        return GL_ONE;
    }

    static GLenum toBlendEquation(asset::RenderBlendOp op)
    {
        switch (op)
        {
        case asset::RenderBlendOp::Add:
            return GL_FUNC_ADD;
        case asset::RenderBlendOp::Subtract:
            return GL_FUNC_SUBTRACT;
        case asset::RenderBlendOp::ReverseSubtract:
            return GL_FUNC_REVERSE_SUBTRACT;
        case asset::RenderBlendOp::Min:
            return GL_MIN;
        case asset::RenderBlendOp::Max:
            return GL_MAX;
        case asset::RenderBlendOp::Mul:
            return GL_FUNC_ADD;
        }

        return GL_FUNC_ADD;
    }

    static void applyUniformDefinition(dataStruct::Material& material, const asset::MaterialUniformDefinition& uniform)
    {
        switch (uniform.kind)
        {
        case asset::MaterialUniformKind::Bool:
            material.addBoolUniform(uniform.name, uniform.boolValue);
            break;
        case asset::MaterialUniformKind::Int:
            material.addIntUniform(uniform.name, uniform.intValue);
            break;
        case asset::MaterialUniformKind::Float:
            material.addFloatUniform(uniform.name, uniform.floatValue);
            break;
        case asset::MaterialUniformKind::Vec3:
            material.addVec3Uniform(uniform.name, uniform.vec3Value);
            break;
        case asset::MaterialUniformKind::Texture:
            if (!uniform.textureAssetPath.empty())
            {
                const std::string resolvedTexturePath = asset::AssetManager::instance().resolveTextureRuntimePath(uniform.textureAssetPath);
                if (!resolvedTexturePath.empty())
                    material.addTextureAsset(uniform.name, uniform.textureAssetPath, resolvedTexturePath);
            }
            break;
        }
    }

    static void applyPassUniformDefinition(
        dataStruct::Material& material,
        const asset::RenderPassUniformDefinition& uniform,
        const std::function<GLuint(const asset::RenderTargetAssetReference&)>& resolveRenderTargetTexture)
    {
        switch (uniform.kind)
        {
        case asset::RenderPassUniformKind::Bool:
            material.addBoolUniform(uniform.name, uniform.boolValue);
            break;
        case asset::RenderPassUniformKind::Int:
            material.addIntUniform(uniform.name, uniform.intValue);
            break;
        case asset::RenderPassUniformKind::Float:
            material.addFloatUniform(uniform.name, uniform.floatValue);
            break;
        case asset::RenderPassUniformKind::Vec3:
            material.addVec3Uniform(uniform.name, uniform.vec3Value);
            break;
        case asset::RenderPassUniformKind::Mat4:
            material.addMat4Uniform(uniform.name, uniform.mat4Value);
            break;
        case asset::RenderPassUniformKind::Texture:
            if (!uniform.assetPath.empty())
            {
                const std::string resolvedTexturePath = asset::AssetManager::instance().resolveTextureRuntimePath(uniform.assetPath);
                if (!resolvedTexturePath.empty())
                    material.addTextureAsset(uniform.name, uniform.assetPath, resolvedTexturePath);
            }
            break;
        case asset::RenderPassUniformKind::RenderTarget:
        {
            const GLuint textureId = resolveRenderTargetTexture(uniform.renderTargetValue);
            if (textureId != 0)
                material.addExternalTexture(uniform.name, textureId);
            break;
        }
        }
    }

    static void applyLightInput(dataStruct::Material& material, const LightInput& light)
    {
        material.addIntUniform("_lightType", light.type == LightType::Directional ? 0 : 1);
        material.addVec3Uniform("_lightPos", light.position);
        material.addVec3Uniform("_lightDir", light.direction);
        material.addVec3Uniform("_lightColor", light.color);
        material.addFloatUniform("_lightIntensity", light.intensity);
    }

    static render::UniformFactoryExecutionContext buildUniformFactoryExecutionContext(
        const Camera& camera,
        const GameObject& gameObject,
        const component::MeshRenderer& meshRenderer,
        const Transform& transform,
        const dataStruct::Material& sourceMaterial,
        const LightInput* light,
        int currentIteration,
        int iterationCount)
    {
        render::UniformFactoryExecutionContext context;
        context.scene = gameObject.getScene();
        context.camera = &camera;
        context.gameObject = &gameObject;
        context.meshRenderer = &meshRenderer;
        context.transform = &transform;
        context.sourceMaterial = &sourceMaterial;
        context.light = light;
        context.currentIteration = currentIteration;
        context.iterationCount = iterationCount;
        return context;
    }

    static bool queryUniformFactoryIterationCount(
        const std::string& rawUniformFactoryPath,
        const Camera& camera,
        const GameObject& gameObject,
        const component::MeshRenderer& meshRenderer,
        const Transform& transform,
        const dataStruct::Material& sourceMaterial,
        const LightInput* light,
        int& iterationCountOut)
    {
        const std::string uniformFactoryPath = asset::AssetManager::normalizeRelativePath(rawUniformFactoryPath);
        if (uniformFactoryPath.empty())
        {
            iterationCountOut = 1;
            return true;
        }

        gameplay::bootstrap();

        asset::UniformFactoryAssetDefinition* factory = nullptr;
        if (!asset::AssetManager::instance().reloadUniformFactoryDefinition(uniformFactoryPath, factory) || factory == nullptr)
            return false;

        const render::UniformFactoryExecutionContext context = buildUniformFactoryExecutionContext(
            camera,
            gameObject,
            meshRenderer,
            transform,
            sourceMaterial,
            light,
            0,
            0);
        if (!gameplay::queryRenderUniformFactoryIterationCount(uniformFactoryPath, context, iterationCountOut))
            return false;

        iterationCountOut = std::max(iterationCountOut, 0);
        return true;
    }

    static bool queryUniformFactoryIterationGroup(
        const std::string& rawUniformFactoryPath,
        const Camera& camera,
        const GameObject& gameObject,
        const component::MeshRenderer& meshRenderer,
        const Transform& transform,
        const dataStruct::Material& sourceMaterial,
        const LightInput* light,
        int currentIteration,
        int iterationCount,
        bool bakedGroup,
        std::string& groupOut)
    {
        groupOut.clear();

        const std::string uniformFactoryPath = asset::AssetManager::normalizeRelativePath(rawUniformFactoryPath);
        if (uniformFactoryPath.empty())
            return true;

        gameplay::bootstrap();

        const render::UniformFactoryExecutionContext context = buildUniformFactoryExecutionContext(
            camera,
            gameObject,
            meshRenderer,
            transform,
            sourceMaterial,
            light,
            currentIteration,
            iterationCount);

        return bakedGroup
            ? gameplay::queryRenderUniformFactoryBakedIterationGroup(uniformFactoryPath, context, groupOut)
            : gameplay::queryRenderUniformFactoryIterationGroup(uniformFactoryPath, context, groupOut);
    }

    static bool applyUniformFactory(
        dataStruct::Material& material,
        const std::string& rawUniformFactoryPath,
        const Camera& camera,
        const GameObject& gameObject,
        const component::MeshRenderer& meshRenderer,
        const Transform& transform,
        const dataStruct::Material& sourceMaterial,
        const LightInput* light,
        int currentIteration,
        int iterationCount)
    {
        const std::string uniformFactoryPath = asset::AssetManager::normalizeRelativePath(rawUniformFactoryPath);
        if (uniformFactoryPath.empty())
            return true;

        gameplay::bootstrap();

        asset::UniformFactoryAssetDefinition* factory = nullptr;
        if (!asset::AssetManager::instance().reloadUniformFactoryDefinition(uniformFactoryPath, factory) || factory == nullptr)
            return false;

        const render::UniformFactoryExecutionContext context = buildUniformFactoryExecutionContext(
            camera,
            gameObject,
            meshRenderer,
            transform,
            sourceMaterial,
            light,
            currentIteration,
            iterationCount);

        const render::UniformFactoryWriter writer(material);
        return gameplay::runRenderUniformFactory(uniformFactoryPath, context, writer);
    }

    static bool queryBatchIterationCount(
        const Camera& camera,
        const Batch& batch,
        int& iterationCountOut)
    {
        if (batch.iterator == asset::RenderPassIterator::None || batch.pass.uniformFactoryPath.empty())
        {
            iterationCountOut = 1;
            return true;
        }

        if (batch.items.empty())
        {
            iterationCountOut = 0;
            return true;
        }

        const DrawItem& item = batch.items.front();
        dataStruct::Material* sourceMaterial = resolveMaterial(item);
        if (item.renderer == nullptr || item.transform == nullptr || sourceMaterial == nullptr)
            return false;

        GameObject* owner = item.renderer->getOwner();
        if (owner == nullptr)
            return false;

        return queryUniformFactoryIterationCount(
            batch.pass.uniformFactoryPath,
            camera,
            *owner,
            *item.renderer,
            *item.transform,
            *sourceMaterial,
            nullptr,
            iterationCountOut);
    }

    static bool queryBatchTargetGroupSuffix(
        const Camera& camera,
        const Batch& batch,
        int currentIteration,
        int iterationCount,
        bool bakedGroup,
        std::string& groupOut)
    {
        groupOut.clear();

        if (batch.iterator == asset::RenderPassIterator::None || batch.pass.uniformFactoryPath.empty())
            return true;

        if (batch.items.empty())
            return true;

        const DrawItem& item = batch.items.front();
        dataStruct::Material* sourceMaterial = resolveMaterial(item);
        if (item.renderer == nullptr || item.transform == nullptr || sourceMaterial == nullptr)
            return false;

        GameObject* owner = item.renderer->getOwner();
        if (owner == nullptr)
            return false;

        return queryUniformFactoryIterationGroup(
            batch.pass.uniformFactoryPath,
            camera,
            *owner,
            *item.renderer,
            *item.transform,
            *sourceMaterial,
            nullptr,
            currentIteration,
            iterationCount,
            bakedGroup,
            groupOut);
    }

    static bool needsTemporaryMaterial(const Batch& batch, const dataStruct::Material& material)
    {
        if (batch.iterator != asset::RenderPassIterator::None || !batch.pass.uniforms.empty() || !batch.pass.uniformFactoryPath.empty())
            return true;

        Shader* shader = material.getShader();
        const std::string shaderAssetPath = shader != nullptr ? asset::AssetManager::normalizeRelativePath(shader->getAssetPath()) : std::string();
        return shaderAssetPath != asset::AssetManager::normalizeRelativePath(batch.pass.shaderPath);
    }

    static bool executeDrawItem(
        const Camera& camera,
        const Batch& batch,
        const DrawItem& item,
        const std::function<GLuint(const asset::RenderTargetAssetReference&)>& resolveRenderTargetTexture,
        const LightInput* light,
        int currentIteration,
        int iterationCount)
    {
        dataStruct::Material* sourceMaterial = resolveMaterial(item);
        if (item.renderer == nullptr || item.transform == nullptr || sourceMaterial == nullptr)
            return false;

        item.renderer->run();
        if (!needsTemporaryMaterial(batch, *sourceMaterial))
        {
            item.renderer->renderWithMaterial(camera, *item.transform, *sourceMaterial);
            return true;
        }

        Shader* shader = asset::AssetManager::instance().loadShader(batch.pass.shaderPath);
        if (shader == nullptr)
            return false;
        GameObject* owner = item.renderer->getOwner();
        if (owner == nullptr)
            return false;

        dataStruct::Material material(shader);
        material.setRuntimePreviewSyncEnabled(false);
        material.setRuntimeDefinitionHeader(sourceMaterial->getRuntimeDefinition().kind, batch.pass.shaderPath, sourceMaterial->getRuntimeDefinition().renderPassPath);

        const asset::MaterialAssetDefinition& sourceDefinition = sourceMaterial->getRuntimeDefinition();
        for (const asset::MaterialUniformDefinition& uniform : sourceDefinition.uniforms)
            applyUniformDefinition(material, uniform);
        for (const asset::RenderPassUniformDefinition& uniform : batch.pass.uniforms)
            applyPassUniformDefinition(material, uniform, resolveRenderTargetTexture);

        if (!applyUniformFactory(
                material,
                batch.pass.uniformFactoryPath,
                camera,
                *owner,
                *item.renderer,
                *item.transform,
                *sourceMaterial,
                light,
                currentIteration,
                iterationCount))
        {
            return false;
        }
        if (light != nullptr && batch.pass.uniformFactoryPath.empty())
            applyLightInput(material, *light);

        item.renderer->renderWithMaterial(camera, *item.transform, material);
        return true;
    }

    static void configureBlendState(const asset::RenderBlendStateDefinition& blend)
    {
        if (!blend.enabled)
        {
            glDisable(GL_BLEND);
            return;
        }

        glEnable(GL_BLEND);
        glBlendEquationSeparate(toBlendEquation(blend.rgb.op), toBlendEquation(blend.a.op));
        glBlendFuncSeparate(
            toBlendFactor(blend.rgb.src),
            toBlendFactor(blend.rgb.dst),
            toBlendFactor(blend.a.src),
            toBlendFactor(blend.a.dst));
    }

    bool rebuild(GameObject* const* gameObjects, size_t gameObjectCount)
    {
        m_phaseOrder.clear();
        m_batches.clear();
        m_renderTargetMetadata.clear();
        m_trackedAssetWriteTimes.clear();

        std::unordered_map<std::string, asset::RenderPhaseAssetDefinition> phases;
        std::vector<std::string> phaseInsertionOrder;
        std::unordered_map<std::string, size_t> batchIndexBySignature;
        std::unordered_map<std::string, asset::RenderPassAssetDefinition*> validatedRenderPasses;
        std::unordered_set<std::string> failedRenderPasses;
        bool validationFailed = false;

        std::function<bool(const std::string&)> registerPhase = [&](const std::string& rawPhasePath) -> bool {
            const std::string phaseAssetPath = renderPhaseKey(rawPhasePath);
            if (phaseAssetPath.empty())
                return false;

            if (phases.find(phaseAssetPath) != phases.end())
                return true;

            asset::RenderPhaseAssetDefinition* definition = nullptr;
            if (!asset::AssetManager::instance().reloadRenderPhaseDefinition(phaseAssetPath, definition) || definition == nullptr)
            {
                std::cerr << "[render] Failed to resolve render phase: " << phaseAssetPath << std::endl;
                return false;
            }

            phases[phaseAssetPath] = *definition;
            phaseInsertionOrder.push_back(phaseAssetPath);
            m_trackedAssetWriteTimes[phaseAssetPath] = safeLastWriteTime(phaseAssetPath);

            for (const std::string& includePath : definition->includes)
                registerPhase(includePath);
            for (const std::string& beforePath : definition->before)
                registerPhase(beforePath);
            for (const std::string& afterPath : definition->after)
                registerPhase(afterPath);
            return true;
        };

        for (size_t index = 0; index < gameObjectCount; ++index)
        {
            GameObject* gameObject = gameObjects[index];
            if (gameObject == nullptr)
                continue;

            component::MeshRenderer* meshRenderer = gameObject->getComponent<component::MeshRenderer>();
            if (meshRenderer == nullptr)
                continue;

            const std::string materialAssetPath = asset::AssetManager::normalizeRelativePath(meshRenderer->getMaterialAssetPath());
            dataStruct::Material* material = resolveMaterial(*meshRenderer);
            if (material == nullptr)
                continue;

            const std::string renderPassPath = asset::AssetManager::normalizeRelativePath(material->getRuntimeDefinition().renderPassPath);
            if (renderPassPath.empty())
                continue;

            asset::RenderPassAssetDefinition* renderPass = nullptr;
            auto renderPassIt = validatedRenderPasses.find(renderPassPath);
            if (renderPassIt == validatedRenderPasses.end())
            {
                std::vector<std::string> dependencyPaths;
                std::vector<std::string> issues;
                if (!validateCompiledRenderPass(renderPassPath, &dependencyPaths, &issues) ||
                    !asset::AssetManager::instance().reloadRenderPassDefinition(renderPassPath, renderPass) || renderPass == nullptr)
                {
                    if (failedRenderPasses.insert(renderPassPath).second)
                    {
                        if (issues.empty())
                            issues.push_back("Failed to resolve a valid built render pass: " + renderPassPath);
                        for (const std::string& issue : issues)
                            std::cerr << "[render] " << issue << std::endl;
                    }

                    for (const std::string& dependencyPath : dependencyPaths)
                        m_trackedAssetWriteTimes[dependencyPath] = safeLastWriteTime(dependencyPath);
                    const std::string markerPath = compiledRenderPassMarkerPath(renderPassPath);
                    if (!markerPath.empty())
                        m_trackedAssetWriteTimes[markerPath] = safeLastWriteTime(markerPath);
                    validationFailed = true;
                    continue;
                }

                validatedRenderPasses[renderPassPath] = renderPass;
                for (const std::string& dependencyPath : dependencyPaths)
                    m_trackedAssetWriteTimes[dependencyPath] = safeLastWriteTime(dependencyPath);
                const std::string markerPath = compiledRenderPassMarkerPath(renderPassPath);
                if (!markerPath.empty())
                    m_trackedAssetWriteTimes[markerPath] = safeLastWriteTime(markerPath);
                renderPassIt = validatedRenderPasses.find(renderPassPath);
            }

            renderPass = renderPassIt != validatedRenderPasses.end() ? renderPassIt->second : nullptr;
            if (renderPass == nullptr)
            {
                validationFailed = true;
                continue;
            }

            for (const asset::RenderPassStepDefinition& pass : renderPass->passes)
            {
                mergeRenderTargetMetadata(m_renderTargetMetadata, pass.target);
                for (const asset::RenderPassUniformDefinition& uniform : pass.uniforms)
                {
                    if (uniform.kind == asset::RenderPassUniformKind::RenderTarget)
                        mergeRenderTargetMetadata(m_renderTargetMetadata, uniform.renderTargetValue);
                }

                if (!registerPhase(pass.phaseName))
                    continue;

                if (phases.find(renderPhaseKey(pass.phaseName)) == phases.end())
                    continue;

                const std::string signature = renderPassBatchSignature(pass);
                auto batchIt = batchIndexBySignature.find(signature);
                if (batchIt == batchIndexBySignature.end())
                {
                    Batch batch;
                    batch.phaseAssetPath = renderPhaseKey(pass.phaseName);
                    batch.signature = signature;
                    batch.pass = pass;
                    batch.iterator = pass.iterator;
                    m_batches.push_back(std::move(batch));
                    batchIt = batchIndexBySignature.emplace(signature, m_batches.size() - 1).first;
                }

                m_batches[batchIt->second].items.push_back({meshRenderer, &gameObject->transform, materialAssetPath, material});
            }
        }

        if (validationFailed)
            return false;

        std::unordered_map<std::string, size_t> indegree;
        std::unordered_map<std::string, std::vector<std::string>> adjacency;
        for (const auto& entry : phases)
            indegree[entry.first] = 0;

        auto addEdge = [&](const std::string& fromRaw, const std::string& toRaw) {
            const std::string from = renderPhaseKey(fromRaw);
            const std::string to = renderPhaseKey(toRaw);
            if (from.empty() || to.empty() || from == to)
                return;

            std::vector<std::string>& edges = adjacency[from];
            if (std::find(edges.begin(), edges.end(), to) != edges.end())
                return;
            edges.push_back(to);
            indegree[to]++;
        };

        for (const auto& entry : phases)
        {
            const asset::RenderPhaseAssetDefinition& phase = entry.second;
            for (const std::string& before : phase.before)
                addEdge(entry.first, before);
            for (const std::string& after : phase.after)
                addEdge(after, entry.first);
        }

        std::vector<std::string> ready;
        for (const std::string& phaseAssetPath : phaseInsertionOrder)
        {
            if (indegree[phaseAssetPath] == 0)
                ready.push_back(phaseAssetPath);
        }

        m_phaseOrder.clear();
        while (!ready.empty())
        {
            const std::string phaseAssetPath = ready.front();
            ready.erase(ready.begin());
            m_phaseOrder.push_back(phaseAssetPath);

            for (const std::string& next : adjacency[phaseAssetPath])
            {
                if (indegree[next] == 0)
                    continue;
                indegree[next]--;
                if (indegree[next] == 0)
                    ready.push_back(next);
            }
        }

        if (m_phaseOrder.size() != phases.size())
        {
            std::cerr << "[render] Render phase cycle detected, falling back to discovery order." << std::endl;
            m_phaseOrder = phaseInsertionOrder;
        }

        std::stable_sort(m_batches.begin(), m_batches.end(), [&](const Batch& lhs, const Batch& rhs) {
            auto phaseIndex = [&](const std::string& phaseAssetPath) {
                for (size_t phaseOrderIndex = 0; phaseOrderIndex < m_phaseOrder.size(); ++phaseOrderIndex)
                {
                    if (m_phaseOrder[phaseOrderIndex] == phaseAssetPath)
                        return phaseOrderIndex;
                }
                return std::numeric_limits<size_t>::max();
            };

            const size_t lhsIndex = phaseIndex(lhs.phaseAssetPath);
            const size_t rhsIndex = phaseIndex(rhs.phaseAssetPath);
            if (lhsIndex != rhsIndex)
                return lhsIndex < rhsIndex;

            return lhs.signature < rhs.signature;
        });

        std::unordered_map<std::string, size_t> uniqueTargetCounts;
        std::vector<std::string> liveTargetInstanceKeys;
        for (Batch& batch : m_batches)
        {
            const std::string normalizedTargetName = normalizeRenderTargetName(batch.pass.target.name);
            if (normalizedTargetName.empty() || isFinalRenderTargetName(normalizedTargetName))
            {
                batch.targetLogicalKey.clear();
                batch.targetInstanceKey.clear();
                continue;
            }

            batch.targetLogicalKey = renderTargetLogicalKey(batch.pass.target);
            if (batch.pass.target.shared)
            {
                batch.targetInstanceKey = batch.targetLogicalKey;
            }
            else
            {
                const size_t targetUseCount = uniqueTargetCounts[batch.targetLogicalKey]++;
                batch.targetInstanceKey = batch.targetLogicalKey + "#" + std::to_string(targetUseCount);
            }

            liveTargetInstanceKeys.push_back(batch.targetInstanceKey);
        }

        for (auto it = m_renderTargets.begin(); it != m_renderTargets.end(); )
        {
            if (std::find(liveTargetInstanceKeys.begin(), liveTargetInstanceKeys.end(), it->first) == liveTargetInstanceKeys.end())
                it = m_renderTargets.erase(it);
            else
                ++it;
        }

        m_structureHash = computeStructureHash(gameObjects, gameObjectCount);
        m_compiled = true;
        m_failed = false;
        return true;
    }

    bool bindBatchTarget(const Batch& batch, const std::string& targetInstanceKey, const std::vector<SceneRenderTargetSettings>& renderTargets, const GLint viewport[4], const GLint finalFramebuffer)
    {
        const std::string normalizedTargetName = normalizeRenderTargetName(batch.pass.target.name);
        if (normalizedTargetName.empty() || isFinalRenderTargetName(normalizedTargetName))
        {
            glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(finalFramebuffer));
            glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
            return true;
        }

        const SceneRenderTargetSettings* settings = findRenderTargetSettings(renderTargets, normalizedTargetName);
        const int width = settings != nullptr && settings->width > 0
            ? settings->width
            : (batch.pass.target.width > 0 ? batch.pass.target.width : viewport[2]);
        const int height = settings != nullptr && settings->height > 0
            ? settings->height
            : (batch.pass.target.height > 0 ? batch.pass.target.height : viewport[3]);
        const RenderTargetFormat format = settings != nullptr
            ? settings->format
            : (batch.pass.target.hasFormat ? batch.pass.target.format : RenderTargetFormat::Rgba);

        RenderTargetResource& resource = m_renderTargets[targetInstanceKey];
        if (!resource.ensure(width, height, format))
        {
            std::cerr << "[render] Failed to allocate render target: " << normalizedTargetName << std::endl;
            return false;
        }

        resource.bind();
        return true;
    }

public:
    void invalidate()
    {
        m_compiled = false;
        m_failed = false;
        m_structureHash = 0;
        m_compositedRenderTargets.clear();
    }

    static bool validateCompiledRenderPass(
        const std::string& rawRenderPassPath,
        std::vector<std::string>* dependencyPathsOut = nullptr,
        std::vector<std::string>* issues = nullptr)
    {
        asset::RenderPassAssetDefinition* renderPass = nullptr;
        std::vector<std::string> dependencyPaths;
        const bool collected = collectRenderPassDependencyPaths(rawRenderPassPath, renderPass, dependencyPaths, issues);
        if (dependencyPathsOut != nullptr)
            *dependencyPathsOut = dependencyPaths;
        if (!collected)
            return false;

        const std::string renderPassPath = asset::AssetManager::normalizeRelativePath(rawRenderPassPath);
        if (!compiledRenderPassMarkerIsCurrent(renderPassPath, dependencyPaths))
        {
            appendIssue(issues, "Render pass requires a rebuild before it can be used: " + renderPassPath);
            return false;
        }

        return true;
    }

    static bool buildCompiledRenderPass(const std::string& rawRenderPassPath, std::vector<std::string>* issues = nullptr)
    {
        asset::RenderPassAssetDefinition* renderPass = nullptr;
        std::vector<std::string> dependencyPaths;
        if (!collectRenderPassDependencyPaths(rawRenderPassPath, renderPass, dependencyPaths, issues) || renderPass == nullptr)
            return false;

        const std::string renderPassPath = asset::AssetManager::normalizeRelativePath(rawRenderPassPath);
        bool valid = true;
        for (size_t passIndex = 0; passIndex < renderPass->passes.size(); ++passIndex)
        {
            const asset::RenderPassStepDefinition& pass = renderPass->passes[passIndex];
            const std::string shaderPath = asset::AssetManager::normalizeRelativePath(pass.shaderPath);
            if (shaderPath.empty())
            {
                appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " has an empty shader path.");
                valid = false;
                continue;
            }

            Shader shader(asset::AssetManager::runtimePath(shaderPath + "/vertex.glsl"), asset::AssetManager::runtimePath(shaderPath + "/fragment.glsl"));
            shader.setAssetPath(shaderPath);
            if (shader.getProgramId() == 0)
            {
                appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " failed to compile shader: " + shaderPath);
                valid = false;
                continue;
            }

            for (const asset::RenderPassUniformDefinition& uniform : pass.uniforms)
            {
                if (!validatePassUniform(shader, uniform, renderPassPath, passIndex, issues))
                    valid = false;
            }

            if (pass.iterator != asset::RenderPassIterator::None && pass.uniformFactoryPath.empty())
            {
                appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " declares an iterator but no uniform factory.");
                valid = false;
            }

            if (!pass.uniformFactoryPath.empty())
            {
                asset::UniformFactoryAssetDefinition* factory = nullptr;
                if (!asset::AssetManager::instance().reloadUniformFactoryDefinition(pass.uniformFactoryPath, factory) || factory == nullptr)
                {
                    appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " failed to load uniform factory: " + pass.uniformFactoryPath);
                    valid = false;
                }
                else if (factory->sourcePath.empty())
                {
                    appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " references a uniform factory without a source file: " + pass.uniformFactoryPath);
                    valid = false;
                }
                else if (factory->entryName.empty())
                {
                    appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " references a uniform factory without an entry point: " + pass.uniformFactoryPath);
                    valid = false;
                }
                else if (factory->iterationEntryName.empty())
                {
                    appendIssue(issues, renderPassPath + " pass #" + std::to_string(passIndex) + " references a uniform factory without an iteration entry point: " + pass.uniformFactoryPath);
                    valid = false;
                }
                else if (!validateUniformFactory(shader, *factory, pass.iterator, renderPassPath, passIndex, issues))
                {
                    valid = false;
                }
            }
        }

        if (!valid)
            return false;

        return writeCompiledRenderPassMarker(renderPassPath, dependencyPaths);
    }

private:
    bool executeInternal(
        const Camera& camera,
        GameObject* const* gameObjects,
        size_t gameObjectCount,
        const std::vector<SceneRenderTargetSettings>& renderTargets,
        const std::vector<LightInput>& lights,
        const ExecutionOptions& options)
    {
        const std::size_t nextStructureHash = computeStructureHash(gameObjects, gameObjectCount);
        const bool structureChanged = nextStructureHash != m_structureHash;
        const bool dependencyStateChanged = dependenciesChanged();
        if (!m_compiled || structureChanged || dependencyStateChanged)
        {
            if (m_failed && !structureChanged && !dependencyStateChanged)
                return false;

            if (!rebuild(gameObjects, gameObjectCount))
            {
                m_structureHash = nextStructureHash;
                m_failed = true;
                return false;
            }
        }

        if (options.bakedOutputs != nullptr)
            options.bakedOutputs->clear();

        GLint initialViewport[4] = {0, 0, 1, 1};
        glGetIntegerv(GL_VIEWPORT, initialViewport);
        GLint initialFramebuffer = 0;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &initialFramebuffer);

        std::unordered_map<std::string, std::string> producedRenderTargetInstances;
        std::unordered_set<std::string> initializedRenderTargets;
        std::unordered_set<std::string> liveRenderTargetInstanceKeys;
        std::unordered_set<std::string> liveCompositeTargetKeys;
        bool renderedAnything = false;
        bool fatalError = false;

        const auto shouldRenderItem = [&](const DrawItem& item, bool dynamicOnlyForBatch) {
            if (options.drawFilter == DrawFilter::StaticOnly && !isDrawItemStatic(item))
                return false;
            if (dynamicOnlyForBatch && isDrawItemStatic(item))
                return false;
            return true;
        };

        const auto renderBatch = [&](const Batch& batch, int currentIteration, int iterationCount) {
            const std::string normalizedTargetName = normalizeRenderTargetName(batch.pass.target.name);
            const bool writesFinalTarget = normalizedTargetName.empty() || isFinalRenderTargetName(normalizedTargetName);
            std::string targetGroupSuffix;
            if (!queryBatchTargetGroupSuffix(camera, batch, currentIteration, iterationCount, false, targetGroupSuffix))
            {
                fatalError = true;
                return;
            }

            std::string bakedTargetGroupSuffix;
            if (!queryBatchTargetGroupSuffix(camera, batch, currentIteration, iterationCount, true, bakedTargetGroupSuffix))
            {
                fatalError = true;
                return;
            }
            const std::string groupedTargetLogicalKey = writesFinalTarget
                ? std::string()
                : appendTargetGroupSuffix(batch.targetLogicalKey, targetGroupSuffix);
            const std::string groupedTargetInstanceKey = writesFinalTarget
                ? std::string()
                : resolveBatchTargetInstanceKey(batch, targetGroupSuffix);

            const SceneRenderTargetSettings* targetSettings = writesFinalTarget ? nullptr : findRenderTargetSettings(renderTargets, normalizedTargetName);
            const RenderTargetMetadata* targetMetadata = writesFinalTarget ? nullptr : findRenderTargetMetadata(normalizedTargetName);
            std::string bakedTargetAssetPath =
                options.allowBakedInputs && targetMetadata != nullptr && targetMetadata->bakeable
                    ? resolveBakedTextureAssetPath(targetSettings, bakedTargetGroupSuffix)
                    : std::string();
            if (!bakedAssetPathMatchesMetadata(bakedTargetAssetPath, targetMetadata))
                bakedTargetAssetPath.clear();
            const bool dynamicOnlyForBatch =
                options.dynamicOnlyOnBakeableTargets && targetMetadata != nullptr && targetMetadata->bakeable && !bakedTargetAssetPath.empty();

            bool batchHasDrawableItems = false;
            for (const DrawItem& item : batch.items)
            {
                if (shouldRenderItem(item, dynamicOnlyForBatch))
                {
                    batchHasDrawableItems = true;
                    break;
                }
            }

            if (!batchHasDrawableItems)
                return;

            if (!bindBatchTarget(batch, groupedTargetInstanceKey, renderTargets, initialViewport, initialFramebuffer))
                return;

            if (!groupedTargetInstanceKey.empty())
                liveRenderTargetInstanceKeys.insert(groupedTargetInstanceKey);

            const std::string targetUseKey = writesFinalTarget ? std::string("__final__") : groupedTargetInstanceKey;
            GLbitfield clearMask = 0;
            if (initializedRenderTargets.insert(targetUseKey).second)
                clearMask |= GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
            if (batch.pass.clearColor)
                clearMask |= GL_COLOR_BUFFER_BIT;
            if (batch.pass.depthAction == asset::RenderDepthAction::Clear)
                clearMask |= GL_DEPTH_BUFFER_BIT;
            if (clearMask != 0)
            {
                glDepthMask(GL_TRUE);
                glClear(clearMask);
            }

            configureBlendState(batch.pass.blend);

            const auto resolveRenderTargetTexture = [&](const asset::RenderTargetAssetReference& reference) -> GLuint {
                const std::string referencedTargetName = normalizeRenderTargetName(reference.name);
                if (referencedTargetName.empty() || isFinalRenderTargetName(referencedTargetName))
                    return 0;

                GLuint dynamicTextureId = 0;
                int dynamicWidth = 0;
                int dynamicHeight = 0;
                RenderTargetFormat dynamicFormat = RenderTargetFormat::Rgba;

                const std::string logicalKey = renderTargetLogicalKey(reference);
                auto producedIt = producedRenderTargetInstances.find(appendTargetGroupSuffix(logicalKey, targetGroupSuffix));
                if (producedIt == producedRenderTargetInstances.end() && !targetGroupSuffix.empty())
                    producedIt = producedRenderTargetInstances.find(logicalKey);
                if (producedIt != producedRenderTargetInstances.end())
                {
                    const auto resourceIt = m_renderTargets.find(producedIt->second);
                    if (resourceIt != m_renderTargets.end())
                    {
                        dynamicTextureId = resourceIt->second.colorTextureId();
                        dynamicWidth = resourceIt->second.width();
                        dynamicHeight = resourceIt->second.height();
                        dynamicFormat = resourceIt->second.format();
                    }
                }

                GLuint bakedTextureId = 0;
                const RenderTargetMetadata* referencedMetadata = findRenderTargetMetadata(referencedTargetName);
                const SceneRenderTargetSettings* referencedSettings = findRenderTargetSettings(renderTargets, referencedTargetName);
                if (options.allowBakedInputs && referencedMetadata != nullptr && referencedMetadata->bakeable)
                {
                    const std::string bakedAssetPath = resolveBakedTextureAssetPath(referencedSettings, bakedTargetGroupSuffix);
                    if (bakedAssetPathMatchesMetadata(bakedAssetPath, referencedMetadata))
                    {
                        Texture2D* texture = resolveCachedTextureAsset(bakedAssetPath);
                        if (texture != nullptr)
                            bakedTextureId = texture->textureId();
                    }
                }

                if (dynamicTextureId != 0 && bakedTextureId != 0 && referencedMetadata != nullptr && referencedMetadata->bakeable)
                {
                    const std::string compositeKey = referencedTargetName + bakedTargetGroupSuffix;
                    liveCompositeTargetKeys.insert(compositeKey);
                    const GLuint compositeTextureId = composeBakedAndDynamicTexture(
                        compositeKey,
                        bakedTextureId,
                        dynamicTextureId,
                        dynamicWidth,
                        dynamicHeight,
                        dynamicFormat,
                        referencedMetadata->bakeCombine);
                    if (compositeTextureId != 0)
                        return compositeTextureId;
                }

                if (dynamicTextureId != 0)
                    return dynamicTextureId;
                if (bakedTextureId != 0)
                    return bakedTextureId;
                return 0;
            };

            for (const DrawItem& item : batch.items)
            {
                if (!shouldRenderItem(item, dynamicOnlyForBatch))
                    continue;

                if (executeDrawItem(camera, batch, item, resolveRenderTargetTexture, nullptr, currentIteration, iterationCount))
                    renderedAnything = true;
            }

            if (!groupedTargetInstanceKey.empty())
                producedRenderTargetInstances[groupedTargetLogicalKey] = groupedTargetInstanceKey;

            if (options.bakedOutputs != nullptr && batch.pass.target.bakeable && !groupedTargetInstanceKey.empty())
            {
                const auto resourceIt = m_renderTargets.find(groupedTargetInstanceKey);
                if (resourceIt != m_renderTargets.end())
                {
                    BakedRenderTargetResult result;
                    result.targetName = normalizedTargetName;
                    result.targetGroupSuffix = bakedTargetGroupSuffix;
                    result.bakeCombine = batch.pass.target.bakeCombine;
                    result.format = resourceIt->second.format();
                    result.width = resourceIt->second.width();
                    result.height = resourceIt->second.height();
                    if (result.format == RenderTargetFormat::Float)
                    {
                        if (resourceIt->second.readColorFloat(result.pixelsFloat))
                            options.bakedOutputs->push_back(std::move(result));
                    }
                    else if (resourceIt->second.readColorRgba8(result.pixelsRgba8))
                    {
                        options.bakedOutputs->push_back(std::move(result));
                    }
                }
            }

            glDisable(GL_BLEND);
        };

        for (size_t batchIndex = 0; batchIndex < m_batches.size();)
        {
            const Batch& batch = m_batches[batchIndex];
            const std::string iterationGroupKey = batchIterationGroupKey(batch);
            if (iterationGroupKey.empty())
            {
                renderBatch(batch, 0, 1);
                if (fatalError)
                {
                    m_failed = true;
                    return false;
                }
                ++batchIndex;
                continue;
            }

            size_t batchGroupEnd = batchIndex + 1;
            while (batchGroupEnd < m_batches.size() && batchIterationGroupKey(m_batches[batchGroupEnd]) == iterationGroupKey)
                ++batchGroupEnd;

            int iterationCount = 0;
            if (!queryBatchIterationCount(camera, batch, iterationCount))
            {
                m_failed = true;
                return false;
            }

            for (int currentIteration = 0; currentIteration < iterationCount; ++currentIteration)
            {
                for (size_t groupedBatchIndex = batchIndex; groupedBatchIndex < batchGroupEnd; ++groupedBatchIndex)
                {
                    renderBatch(m_batches[groupedBatchIndex], currentIteration, iterationCount);
                    if (fatalError)
                    {
                        m_failed = true;
                        return false;
                    }
                }
            }

            batchIndex = batchGroupEnd;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(initialFramebuffer));
        glViewport(initialViewport[0], initialViewport[1], initialViewport[2], initialViewport[3]);

        for (auto it = m_renderTargets.begin(); it != m_renderTargets.end();)
        {
            if (liveRenderTargetInstanceKeys.find(it->first) == liveRenderTargetInstanceKeys.end())
                it = m_renderTargets.erase(it);
            else
                ++it;
        }

        for (auto it = m_compositedRenderTargets.begin(); it != m_compositedRenderTargets.end();)
        {
            if (liveCompositeTargetKeys.find(it->first) == liveCompositeTargetKeys.end())
                it = m_compositedRenderTargets.erase(it);
            else
                ++it;
        }

        return renderedAnything;
    }

public:
    bool execute(
        const Camera& camera,
        GameObject* const* gameObjects,
        size_t gameObjectCount,
        const std::vector<SceneRenderTargetSettings>& renderTargets,
        const std::vector<LightInput>& lights)
    {
        ExecutionOptions options;
        options.drawFilter = DrawFilter::All;
        options.allowBakedInputs = true;
        options.dynamicOnlyOnBakeableTargets = true;
        return executeInternal(camera, gameObjects, gameObjectCount, renderTargets, lights, options);
    }

    bool bake(
        const Camera& camera,
        GameObject* const* gameObjects,
        size_t gameObjectCount,
        const std::vector<SceneRenderTargetSettings>& renderTargets,
        const std::vector<LightInput>& lights,
        std::vector<BakedRenderTargetResult>& outputs)
    {
        ExecutionOptions options;
        options.drawFilter = DrawFilter::StaticOnly;
        options.allowBakedInputs = false;
        options.dynamicOnlyOnBakeableTargets = false;
        options.bakedOutputs = &outputs;
        return executeInternal(camera, gameObjects, gameObjectCount, renderTargets, lights, options);
    }
};
}