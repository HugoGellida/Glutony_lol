#pragma once

#include <GL/glew.h>

#include "../asset/AssetManager.hpp"
#include "../gameobject/GameObject.hpp"
#include "../gameobject/component/MeshRenderer.hpp"
#include "../shader/Material.hpp"
#include "RenderLightData.hpp"
#include "RenderTargetResource.hpp"
#include "SceneRenderTargetSettings.hpp"

#include <algorithm>
#include <filesystem>
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
    struct DrawItem
    {
        component::MeshRenderer* renderer = nullptr;
        Transform* transform = nullptr;
        std::string materialAssetPath;
        dataStruct::Material* fallbackMaterial = nullptr;
    };

    struct Batch
    {
        std::string phaseAssetPath;
        std::string signature;
        asset::RenderPassStepDefinition pass;
        asset::RenderPhaseIterator iterator = asset::RenderPhaseIterator::None;
        std::string targetLogicalKey;
        std::string targetInstanceKey;
        std::vector<DrawItem> items;
    };

    std::vector<std::string> m_phaseOrder;
    std::vector<Batch> m_batches;
    std::unordered_map<std::string, RenderTargetResource> m_renderTargets;
    std::unordered_map<std::string, std::filesystem::file_time_type> m_trackedAssetWriteTimes;
    std::size_t m_structureHash = 0;
    bool m_compiled = false;

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

    static std::string renderTargetLogicalKey(const asset::RenderTargetAssetReference& target)
    {
        return normalizeRenderTargetName(target.name) + '|' + (target.shared ? '1' : '0');
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

    static std::string renderPassBatchSignature(const asset::RenderPassStepDefinition& pass, asset::RenderPhaseIterator iterator)
    {
        std::ostringstream stream;
        stream << pass.phaseName << '|'
               << asset::AssetManager::normalizeRelativePath(pass.shaderPath) << '|'
               << pass.target.name << '|'
               << (pass.target.shared ? '1' : '0') << '|'
               << (pass.clearColor ? '1' : '0') << '|'
               << static_cast<int>(pass.depthAction) << '|'
               << (pass.blend.enabled ? '1' : '0') << '|'
               << (pass.blend.separateAlpha ? '1' : '0') << '|'
               << static_cast<int>(iterator);

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
            case asset::RenderPassUniformKind::Texture:
                stream << uniform.assetPath;
                break;
            case asset::RenderPassUniformKind::RenderTarget:
                stream << uniform.renderTargetValue.name << ':' << (uniform.renderTargetValue.shared ? '1' : '0');
                break;
            }
        }

        return stream.str();
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
                material.addTexture(uniform.name, asset::AssetManager::runtimePath(uniform.textureAssetPath));
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
        case asset::RenderPassUniformKind::Texture:
            if (!uniform.assetPath.empty())
                material.addTexture(uniform.name, asset::AssetManager::runtimePath(uniform.assetPath));
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

    static bool needsTemporaryMaterial(const Batch& batch, const dataStruct::Material& material)
    {
        if (batch.iterator != asset::RenderPhaseIterator::None || !batch.pass.uniforms.empty())
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
        const LightInput* light)
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

        dataStruct::Material material(shader);
        material.setRuntimePreviewSyncEnabled(false);
        material.setRuntimeDefinitionHeader(sourceMaterial->getRuntimeDefinition().kind, batch.pass.shaderPath, sourceMaterial->getRuntimeDefinition().renderPassPath);

        const asset::MaterialAssetDefinition& sourceDefinition = sourceMaterial->getRuntimeDefinition();
        for (const asset::MaterialUniformDefinition& uniform : sourceDefinition.uniforms)
            applyUniformDefinition(material, uniform);
        for (const asset::RenderPassUniformDefinition& uniform : batch.pass.uniforms)
            applyPassUniformDefinition(material, uniform, resolveRenderTargetTexture);
        if (light != nullptr)
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
        m_trackedAssetWriteTimes.clear();

        std::unordered_map<std::string, asset::RenderPhaseAssetDefinition> phases;
        std::vector<std::string> phaseInsertionOrder;
        std::unordered_map<std::string, size_t> batchIndexBySignature;

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
            if (!asset::AssetManager::instance().reloadRenderPassDefinition(renderPassPath, renderPass) || renderPass == nullptr)
            {
                std::cerr << "[render] Failed to resolve render pass: " << renderPassPath << std::endl;
                continue;
            }

            m_trackedAssetWriteTimes[renderPassPath] = safeLastWriteTime(renderPassPath);

            for (const asset::RenderPassStepDefinition& pass : renderPass->passes)
            {
                if (!registerPhase(pass.phaseName))
                    continue;

                const auto phaseIt = phases.find(renderPhaseKey(pass.phaseName));
                if (phaseIt == phases.end())
                    continue;

                const std::string signature = renderPassBatchSignature(pass, phaseIt->second.additionalIterator);
                auto batchIt = batchIndexBySignature.find(signature);
                if (batchIt == batchIndexBySignature.end())
                {
                    Batch batch;
                    batch.phaseAssetPath = renderPhaseKey(pass.phaseName);
                    batch.signature = signature;
                    batch.pass = pass;
                    batch.iterator = phaseIt->second.additionalIterator;
                    m_batches.push_back(std::move(batch));
                    batchIt = batchIndexBySignature.emplace(signature, m_batches.size() - 1).first;
                }

                m_batches[batchIt->second].items.push_back({meshRenderer, &gameObject->transform, materialAssetPath, material});
            }
        }

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
        return true;
    }

    bool bindBatchTarget(const Batch& batch, const std::vector<SceneRenderTargetSettings>& renderTargets, const GLint viewport[4], const GLint finalFramebuffer)
    {
        const std::string normalizedTargetName = normalizeRenderTargetName(batch.pass.target.name);
        if (normalizedTargetName.empty() || isFinalRenderTargetName(normalizedTargetName))
        {
            glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(finalFramebuffer));
            glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
            return true;
        }

        const SceneRenderTargetSettings* settings = findRenderTargetSettings(renderTargets, normalizedTargetName);
        const int width = settings != nullptr && settings->width > 0 ? settings->width : viewport[2];
        const int height = settings != nullptr && settings->height > 0 ? settings->height : viewport[3];
        const RenderTargetFormat format = settings != nullptr ? settings->format : RenderTargetFormat::Rgba;

        RenderTargetResource& resource = m_renderTargets[batch.targetInstanceKey];
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
        m_structureHash = 0;
    }

    bool execute(
        const Camera& camera,
        GameObject* const* gameObjects,
        size_t gameObjectCount,
        const std::vector<SceneRenderTargetSettings>& renderTargets,
        const std::vector<LightInput>& lights)
    {
        const std::size_t nextStructureHash = computeStructureHash(gameObjects, gameObjectCount);
        if (!m_compiled || nextStructureHash != m_structureHash || dependenciesChanged())
        {
            if (!rebuild(gameObjects, gameObjectCount))
                return false;
        }

        GLint initialViewport[4] = {0, 0, 1, 1};
        glGetIntegerv(GL_VIEWPORT, initialViewport);
        GLint initialFramebuffer = 0;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &initialFramebuffer);

        std::unordered_map<std::string, std::string> producedRenderTargetInstances;
        std::unordered_set<std::string> initializedRenderTargets;
        bool renderedAnything = false;
        for (const Batch& batch : m_batches)
        {
            if (!bindBatchTarget(batch, renderTargets, initialViewport, initialFramebuffer))
                continue;

            const std::string normalizedTargetName = normalizeRenderTargetName(batch.pass.target.name);
            const std::string targetUseKey =
                normalizedTargetName.empty() || isFinalRenderTargetName(normalizedTargetName)
                    ? std::string("__final__")
                    : (!batch.targetInstanceKey.empty() ? batch.targetInstanceKey : renderTargetLogicalKey(batch.pass.target));

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
                const std::string normalizedTargetName = normalizeRenderTargetName(reference.name);
                if (normalizedTargetName.empty() || isFinalRenderTargetName(normalizedTargetName))
                    return 0;

                const std::string logicalKey = renderTargetLogicalKey(reference);
                const auto producedIt = producedRenderTargetInstances.find(logicalKey);
                if (producedIt == producedRenderTargetInstances.end())
                    return 0;

                const auto resourceIt = m_renderTargets.find(producedIt->second);
                if (resourceIt == m_renderTargets.end())
                    return 0;

                return resourceIt->second.colorTextureId();
            };

            if (batch.iterator == asset::RenderPhaseIterator::Light)
            {
                for (const LightInput& light : lights)
                {
                    for (const DrawItem& item : batch.items)
                    {
                        if (executeDrawItem(camera, batch, item, resolveRenderTargetTexture, &light))
                            renderedAnything = true;
                    }
                }
            }
            else
            {
                for (const DrawItem& item : batch.items)
                {
                    if (executeDrawItem(camera, batch, item, resolveRenderTargetTexture, nullptr))
                        renderedAnything = true;
                }
            }

            if (!batch.targetInstanceKey.empty())
                producedRenderTargetInstances[batch.targetLogicalKey] = batch.targetInstanceKey;

            glDisable(GL_BLEND);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(initialFramebuffer));
        glViewport(initialViewport[0], initialViewport[1], initialViewport[2], initialViewport[3]);

        return renderedAnything;
    }
};
}