#pragma once
#include "Shader.hpp"
#include "../Camera.hpp"
#include "../asset/MaterialAssetIO.hpp"
#include <common/gameobject/Transform.hpp>

#include <algorithm>
#include <utility>
#include <vector>


namespace dataStruct
{
    // AHAH ENCORE DU POLY DANS UNE LISTE, gud luk, future me!
    class AMaterial
    {
    protected:
        Shader * m_shader;
        template <typename T> static void push_back(T ** & arr, T * val, uint & capacity)
        {
            T ** n_arr = new T*[++capacity];
            for (uint i = 0; i < capacity - 1; i++)
                n_arr[i] = arr[i];
            n_arr[capacity - 1] = val;
            delete[] arr;
            arr = n_arr;
        }
    public:
        AMaterial(Shader * shader)
        {
            m_shader = shader;
        }
        // used to declare uniforms
        virtual void init() = 0;
        // used to sync uniforms
        virtual void sync() = 0;
        virtual void bind(Camera const & cam, Transform & transform) = 0;
        virtual ~AMaterial(){}
    };
    

    class Material : public AMaterial
    {
    private:
        glm::mat4 * mvp;
        glm::mat4 * mvpOrtho;
        glm::mat4 * m_model;
        std::string m_assetPath;
        std::vector<std::pair<std::string, int>> m_dynamicIntUniforms;
        std::vector<std::pair<std::string, float>> m_dynamicFloatUniforms;
        std::vector<std::pair<std::string, glm::vec3>> m_dynamicVec3Uniforms;
        std::vector<std::pair<std::string, glm::mat4>> m_dynamicMat4Uniforms;
        asset::MaterialAssetDefinition m_runtimeDefinition;
        bool m_runtimeDefinitionDirty = false;
        bool m_runtimePreviewSyncEnabled = true;

        static std::string normalizeAssetPathValue(const std::string& rawPath)
        {
            std::string normalized = rawPath;
            std::replace(normalized.begin(), normalized.end(), '\\', '/');

            while (normalized.rfind("./", 0) == 0)
                normalized.erase(0, 2);

            while (!normalized.empty() && normalized.front() == '/')
                normalized.erase(normalized.begin());

            return normalized;
        }

        template <typename T>
        static void upsertNamedValue(std::vector<std::pair<std::string, T>>& values, const std::string& key, const T& value)
        {
            for (std::pair<std::string, T>& entry : values)
            {
                if (entry.first != key)
                    continue;

                entry.second = value;
                return;
            }

            values.emplace_back(key, value);
        }

        asset::MaterialUniformDefinition* findRuntimeUniformDefinition(const std::string& uniformLocation, asset::MaterialUniformKind kind)
        {
            for (asset::MaterialUniformDefinition& uniform : m_runtimeDefinition.uniforms)
            {
                if (uniform.name == uniformLocation && uniform.kind == kind)
                    return &uniform;
            }

            return nullptr;
        }

        asset::MaterialUniformDefinition& getOrCreateRuntimeUniformDefinition(const std::string& uniformLocation, asset::MaterialUniformKind kind)
        {
            if (asset::MaterialUniformDefinition* uniform = findRuntimeUniformDefinition(uniformLocation, kind))
                return *uniform;

            asset::MaterialUniformDefinition uniform;
            uniform.name = uniformLocation;
            uniform.kind = kind;
            m_runtimeDefinition.uniforms.push_back(uniform);
            return m_runtimeDefinition.uniforms.back();
        }

        void markRuntimeDefinitionDirty()
        {
            if (m_runtimePreviewSyncEnabled)
                m_runtimeDefinitionDirty = true;
        }

        size_t textureSlotForLocation(const std::string& uniformLocation, bool& alreadyPresent) const
        {
            for (size_t index = 0; index < textures.size(); ++index)
            {
                if (textures[index].getLocation() != uniformLocation)
                    continue;

                alreadyPresent = true;
                return index;
            }

            alreadyPresent = false;
            return textures.size();
        }

        void assignTextureUniform(const std::string& uniformLocation, const std::string& resolvedTexturePath)
        {
            if (resolvedTexturePath.empty())
                return;

            bool updated = false;
            const size_t textureSlot = textureSlotForLocation(uniformLocation, updated);
            UniformTex2D uniform(uniformLocation, Texture2D(resolvedTexturePath, static_cast<GLuint>(textureSlot)));

            if (updated)
                textures[textureSlot] = std::move(uniform);
            else
                textures.push_back(std::move(uniform));
        }

        void assignTextureAssetUniform(const std::string& uniformLocation, const std::string& textureAssetPath)
        {
            if (textureAssetPath.empty())
                return;

            bool updated = false;
            const size_t textureSlot = textureSlotForLocation(uniformLocation, updated);
            UniformTex2D uniform(uniformLocation, Texture2D::fromAssetPath(textureAssetPath, static_cast<GLuint>(textureSlot)));

            if (updated)
                textures[textureSlot] = std::move(uniform);
            else
                textures.push_back(std::move(uniform));
        }
    protected:

        uint m_uni1f_stride = 0;
        uint m_uniMat4f_stride = 0;
        uint m_uniVec3f_stride = 0;
        
        Uniform1f ** m_uni1f = nullptr;
        UniformMat4x4f ** m_uniMat4f = nullptr;
        UniformVec3f ** m_uniVec3f = nullptr;
        std::vector<UniformTex2D> textures = std::vector<UniformTex2D>(0);
        
    public:
        


        Material() = delete;
        Material(Shader * shader) : AMaterial(shader){init();}
        void init() override
        {
            mvp = new glm::mat4 {glm::mat4(
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0
            )};
            mvpOrtho = new glm::mat4 {glm::mat4(
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0
            )};
            m_model = new glm::mat4 {glm::mat4(
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0
            )};

            push_back(m_uniMat4f, new UniformMat4x4f("MVP", mvp), m_uniMat4f_stride);
            push_back(m_uniMat4f, new UniformMat4x4f("MVP_ORTHO", mvpOrtho), m_uniMat4f_stride);
            push_back(m_uniMat4f, new UniformMat4x4f("MODEL", m_model), m_uniMat4f_stride);
        }
        
        void addTexture(const std::string& uniformLocation, const std::string& texturePath)
        {
            assignTextureUniform(uniformLocation, texturePath);

            asset::MaterialUniformDefinition& uniform = getOrCreateRuntimeUniformDefinition(uniformLocation, asset::MaterialUniformKind::Texture);
            const std::string normalizedTexturePath = normalizeAssetPathValue(texturePath);
            if (uniform.textureAssetPath != normalizedTexturePath)
            {
                uniform.textureAssetPath = normalizedTexturePath;
                markRuntimeDefinitionDirty();
            }
        }

        void addTextureAsset(const std::string& uniformLocation, const std::string& textureAssetPath)
        {
            const std::string normalizedTexturePath = normalizeAssetPathValue(textureAssetPath);
            assignTextureAssetUniform(uniformLocation, normalizedTexturePath);

            asset::MaterialUniformDefinition& uniform = getOrCreateRuntimeUniformDefinition(uniformLocation, asset::MaterialUniformKind::Texture);
            if (uniform.textureAssetPath != normalizedTexturePath)
            {
                uniform.textureAssetPath = normalizedTexturePath;
                markRuntimeDefinitionDirty();
            }
        }

        void addExternalTexture(const std::string& uniformLocation, GLuint textureId)
        {
            bool updated = false;
            const size_t textureSlot = textureSlotForLocation(uniformLocation, updated);
            UniformTex2D uniform(uniformLocation, Texture2D(textureId, static_cast<GLuint>(textureSlot), false));

            if (updated)
                textures[textureSlot] = std::move(uniform);
            else
                textures.push_back(std::move(uniform));
        }

        void addBoolUniform(const std::string& uniformLocation, bool value)
        {
            const int storedValue = value ? 1 : 0;
            upsertNamedValue(m_dynamicIntUniforms, uniformLocation, storedValue);

            asset::MaterialUniformDefinition& uniform = getOrCreateRuntimeUniformDefinition(uniformLocation, asset::MaterialUniformKind::Bool);
            if (uniform.boolValue != value)
            {
                uniform.boolValue = value;
                markRuntimeDefinitionDirty();
            }
        }

        void addIntUniform(const std::string& uniformLocation, int value)
        {
            upsertNamedValue(m_dynamicIntUniforms, uniformLocation, value);

            asset::MaterialUniformDefinition& uniform = getOrCreateRuntimeUniformDefinition(uniformLocation, asset::MaterialUniformKind::Int);
            if (uniform.intValue != value)
            {
                uniform.intValue = value;
                markRuntimeDefinitionDirty();
            }
        }

        void addFloatUniform(const std::string& uniformLocation, float value)
        {
            upsertNamedValue(m_dynamicFloatUniforms, uniformLocation, value);

            asset::MaterialUniformDefinition& uniform = getOrCreateRuntimeUniformDefinition(uniformLocation, asset::MaterialUniformKind::Float);
            if (uniform.floatValue != value)
            {
                uniform.floatValue = value;
                markRuntimeDefinitionDirty();
            }
        }

        void addVec3Uniform(const std::string& uniformLocation, const glm::vec3& value)
        {
            upsertNamedValue(m_dynamicVec3Uniforms, uniformLocation, value);

            asset::MaterialUniformDefinition& uniform = getOrCreateRuntimeUniformDefinition(uniformLocation, asset::MaterialUniformKind::Vec3);
            if (uniform.vec3Value != value)
            {
                uniform.vec3Value = value;
                markRuntimeDefinitionDirty();
            }
        }

        void addMat4Uniform(const std::string& uniformLocation, const glm::mat4& value)
        {
            upsertNamedValue(m_dynamicMat4Uniforms, uniformLocation, value);
        }

        void sync() override
        {
            m_shader -> setActive();
            const GLuint programId = m_shader->getProgramId();
            for (uint i = 0; i < m_uni1f_stride; i++)
                m_shader -> Upload((IUniform *)m_uni1f[i]);
            for (uint i = 0; i < m_uniMat4f_stride; i++)
                m_shader -> Upload((IUniform *)m_uniMat4f[i]);
            for (uint i = 0; i < textures.size(); i++)
                m_shader -> Upload((IUniform *)&(textures[i]));
            for (uint i = 0; i < m_uniVec3f_stride; i++)
                m_shader -> Upload((IUniform *)m_uniVec3f[i]);
            for (const auto& uniform : m_dynamicIntUniforms)
            {
                const GLint location = glGetUniformLocation(programId, uniform.first.c_str());
                if (location != -1)
                    glUniform1i(location, uniform.second);
            }
            for (const auto& uniform : m_dynamicFloatUniforms)
            {
                const GLint location = glGetUniformLocation(programId, uniform.first.c_str());
                if (location != -1)
                    glUniform1f(location, uniform.second);
            }
            for (const auto& uniform : m_dynamicVec3Uniforms)
            {
                const GLint location = glGetUniformLocation(programId, uniform.first.c_str());
                if (location != -1)
                    glUniform3f(location, uniform.second.x, uniform.second.y, uniform.second.z);
            }
            for (const auto& uniform : m_dynamicMat4Uniforms)
            {
                const GLint location = glGetUniformLocation(programId, uniform.first.c_str());
                if (location != -1)
                    glUniformMatrix4fv(location, 1, GL_FALSE, &(uniform.second[0][0]));
            }
        }
        
        void bind(Camera const & cam, Transform & transform) override
        {
            // TODO rebuild mvp here!
            *m_model = transform.getModelWorld();
            *mvp = cam.projectionMatrix() * cam.inverseTransform() * (*m_model);
            *mvpOrtho = glm::transpose(glm::inverse(*m_model));
            sync();
        }

        Shader* getShader() const
        {
            return m_shader;
        }

        void setAssetPath(const std::string& assetPath)
        {
            m_assetPath = assetPath;
        }

        const std::string& getAssetPath() const
        {
            return m_assetPath;
        }

        void setRuntimeDefinitionHeader(asset::MaterialAssetKind kind, const std::string& shaderPath, const std::string& renderPassPath = "")
        {
            m_runtimeDefinition.kind = kind;
            m_runtimeDefinition.shaderPath = normalizeAssetPathValue(shaderPath);
            m_runtimeDefinition.renderPassPath = normalizeAssetPathValue(renderPassPath);
            m_runtimeDefinition.uniforms.clear();
            m_runtimeDefinitionDirty = false;
        }

        void setRuntimePreviewSyncEnabled(bool enabled)
        {
            m_runtimePreviewSyncEnabled = enabled;
            if (!enabled)
                m_runtimeDefinitionDirty = false;
        }

        bool consumeRuntimeDefinition(asset::MaterialAssetDefinition& definitionOut)
        {
            if (!m_runtimePreviewSyncEnabled || !m_runtimeDefinitionDirty)
                return false;

            definitionOut = m_runtimeDefinition;
            m_runtimeDefinitionDirty = false;
            return true;
        }

        const asset::MaterialAssetDefinition& getRuntimeDefinition() const
        {
            return m_runtimeDefinition;
        }

        ~Material()
        {
            for (uint i = 0; i < m_uni1f_stride; i++)
                delete (m_uni1f[i]);
            delete[] m_uni1f;
            for (uint i = 0; i < m_uniMat4f_stride; i++)
                delete (m_uniMat4f[i]);
            delete[] m_uniMat4f;
            delete mvp;
            delete mvpOrtho;
            delete m_model;
        }
    };
}
