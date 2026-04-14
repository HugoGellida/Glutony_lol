#pragma once

#include "Camera.hpp"
#include "gameobject/GameObject.hpp"
#include "common/shader/Material.hpp"
#include "common/shader/LitMaterial.hpp"
#include "common/shader/UnlitMaterial.hpp"
#include <common/shader/Shader.hpp>
#include "gameobject/component/Mesh.hpp"
#include "gameobject/component/MeshRenderer.hpp"
#include "gameobject/component/PointLight.hpp"
#include "gameobject/component/ScriptComponent.hpp"
#include "gameobject/component/MeshNoiseDeformPerlinHeight.hpp"
#include "physics/RigidBody.hpp"
#include "InputProccessor.hpp"
#include "FileLoader.hpp"
#include "physics/SphereCollider.hpp"
#include "physics/PhysicEngine.hpp"
#include "physics/BoxCollider.hpp"
#include "physics/PlaneCollider.hpp"
#include "geometry/Plane.hpp"
#include "asset/AssetManager.hpp"
#include "render/SceneRenderTargetSettings.hpp"
#include "render/RenderLightData.hpp"
#include "render/RenderPipeline.hpp"

#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>


class Scene
{
private:
    GameObject ** m_gameObjects = nullptr;
    size_t m_gameObjectCapacity = 0;
    size_t m_meshsCount=0;
    component::Mesh ** m_meshs=nullptr;
    
    size_t m_gameObjectCount = 0;
    int m_nextGameObjectId = 1;
    dataStruct::Material * m_materials;
    Shader * m_shaders;
    Shader * m_selectionHighlightShader = nullptr;
    dataStruct::UnlitMaterial* m_selectionHighlightMaterial = nullptr;
    PhysicsSystem ph;

    Camera m_camera;
    inputProcessor::InputProcessor m_inputProcessor;
    bool m_fpsControl = false;
    bool m_orbitMode = false;
    bool m_physicsSimulationEnabled = false;
    GameObject* m_selectedGameObject = nullptr;
    std::string m_sceneScriptAssetPath;
    std::string m_dataAssetPath;
    std::vector<render::SceneRenderTargetSettings> m_savedRenderTargetSettings;
    render::DirectionalLightSettings m_directionalLight;
    glm::vec3 m_orbitPos = glm::vec3(0.0f, 10.0f, -10.0f);
    float m_orbitYangle = 0.0f;
    float m_orbitSpeed = 20.0f;

    float m_anim_angle = 0.0f;
    asset::SceneAssetRegistry m_sceneAssetRegistry;
    render::RenderPipeline m_renderPipeline;

    static std::string proceduralPlaneAssetPath()
    {
        return "procedural/plane";
    }

    render::SceneRenderTargetSettings* findSavedRenderTargetSettings(const std::string& rawName)
    {
        const std::string normalizedName = render::normalizeRenderTargetName(rawName);
        for (render::SceneRenderTargetSettings& settings : m_savedRenderTargetSettings)
        {
            if (render::normalizeRenderTargetName(settings.name) == normalizedName)
                return &settings;
        }

        return nullptr;
    }

    const render::SceneRenderTargetSettings* findSavedRenderTargetSettings(const std::string& rawName) const
    {
        const std::string normalizedName = render::normalizeRenderTargetName(rawName);
        for (const render::SceneRenderTargetSettings& settings : m_savedRenderTargetSettings)
        {
            if (render::normalizeRenderTargetName(settings.name) == normalizedName)
                return &settings;
        }

        return nullptr;
    }

    component::Mesh* useMeshAsset(const std::string& relativePath)
    {
        const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(relativePath);
        m_sceneAssetRegistry.registerAsset(asset::AssetType::Mesh, normalizedPath);
        return asset::AssetManager::instance().loadMesh(normalizedPath);
    }

    Shader* useShaderAsset(const std::string& relativePath)
    {
        const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(relativePath);
        m_sceneAssetRegistry.registerAsset(asset::AssetType::Shader, normalizedPath);
        return asset::AssetManager::instance().loadShader(normalizedPath);
    }

    dataStruct::Material* useMaterialAsset(const std::string& relativePath)
    {
        const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(relativePath);
        m_sceneAssetRegistry.registerAsset(asset::AssetType::Material, normalizedPath);
        return asset::AssetManager::instance().loadMaterial(normalizedPath);
    }

    void refreshMaterialsForChangedShaders()
    {
        const std::vector<std::string> materialPaths = asset::AssetManager::instance().collectMaterialsNeedingShaderRefresh();
        for (const std::string& materialPath : materialPaths)
            refreshMaterialAsset(materialPath);
    }

    void refreshMeshesForChangedSources()
    {
        asset::AssetManager::instance().refreshLoadedMeshesIfSourcesChanged();
    }

    component::Mesh* useProceduralPlaneAsset()
    {
        const std::string assetPath = proceduralPlaneAssetPath();
        m_sceneAssetRegistry.registerAsset(asset::AssetType::Mesh, assetPath);

        if (m_meshs != nullptr && m_meshsCount > 1 && m_meshs[1] != nullptr)
            return m_meshs[1];

        component::Mesh* plane = new Plane(glm::vec3(0, 0, 0), 10, 2);
        plane->setAssetPath(assetPath);
        plane->ownerCount++;

        if (m_meshs != nullptr && m_meshsCount > 1)
            m_meshs[1] = plane;

        return plane;
    }

    void renderGameObject(GameObject* gameObject)
    {
        if (gameObject == nullptr)
            return;

        MeshRenderer* meshRenderer = gameObject->getComponent<MeshRenderer>();
        if (meshRenderer == nullptr)
            return;

        meshRenderer->run();
        meshRenderer->render(m_camera, gameObject->transform);
    }

    void renderSelectedHighlight()
    {
        if (m_selectedGameObject == nullptr || m_selectionHighlightMaterial == nullptr)
            return;

        MeshRenderer* meshRenderer = m_selectedGameObject->getComponent<MeshRenderer>();
        if (meshRenderer == nullptr)
            return;

        m_selectionHighlightMaterial->setMainColor(glm::vec3(1.0f, 0.58f, 0.14f));
        meshRenderer->run();
        meshRenderer->renderOverlayWithMaterial(m_camera, m_selectedGameObject->transform, *m_selectionHighlightMaterial);
    }

    void ensureGameObjectCapacity(size_t requiredCapacity)
    {
        if (requiredCapacity <= m_gameObjectCapacity)
            return;

        size_t newCapacity = (m_gameObjectCapacity == 0) ? 8 : m_gameObjectCapacity;
        while (newCapacity < requiredCapacity)
            newCapacity *= 2;

        GameObject** newGameObjects = new GameObject*[newCapacity];
        for (size_t index = 0; index < m_gameObjectCount; ++index)
            newGameObjects[index] = m_gameObjects[index];
        for (size_t index = m_gameObjectCount; index < newCapacity; ++index)
            newGameObjects[index] = nullptr;

        delete[] m_gameObjects;
        m_gameObjects = newGameObjects;
        m_gameObjectCapacity = newCapacity;
    }

    void detachGameObjectFromHierarchy(GameObject* gameObject)
    {
        if (gameObject == nullptr)
            return;

        for (size_t index = 0; index < m_gameObjectCount; ++index)
        {
            GameObject* candidate = m_gameObjects[index];
            if (candidate == nullptr || candidate == gameObject)
                continue;

            for (size_t childIndex = 0; childIndex < candidate->transform.getChildCount(); ++childIndex)
            {
                if (candidate->transform.getChild(childIndex) == gameObject)
                {
                    candidate->transform.detachChild(&gameObject->transform);
                    gameObject->transform.removeParent();
                    return;
                }
            }
        }
    }

    void destroyAllGameObjects()
    {
        for (size_t index = 0; index < m_gameObjectCount; ++index)
        {
            delete m_gameObjects[index];
            m_gameObjects[index] = nullptr;
        }

        m_gameObjectCount = 0;
        m_selectedGameObject = nullptr;
    }

public:
    void setFpsControlEnabled(bool enabled, GLFWwindow* window = nullptr, bool lockMouseToAnchor = false, double mouseAnchorX = 0.0, double mouseAnchorY = 0.0)
    {
        m_fpsControl = enabled;
        if (window == nullptr)
            return;

        int width = 0;
        int height = 0;
        glfwGetWindowSize(window, &width, &height);
        const double resetX = lockMouseToAnchor ? mouseAnchorX : static_cast<double>(width / 2);
        const double resetY = lockMouseToAnchor ? mouseAnchorY : static_cast<double>(height / 2);

        glfwSetInputMode(window, GLFW_CURSOR, enabled ? (lockMouseToAnchor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_DISABLED) : GLFW_CURSOR_NORMAL);
        glfwSetCursorPos(window, resetX, resetY);
        m_inputProcessor.resetMouseState(resetX, resetY);
    }

    void enterPlayMode(GLFWwindow* window, bool captureMouse, bool lockMouseToAnchor = false, double mouseAnchorX = 0.0, double mouseAnchorY = 0.0)
    {
        setPhysicsSimulationEnabled(true);
        setFpsControlEnabled(captureMouse, window, lockMouseToAnchor, mouseAnchorX, mouseAnchorY);
    }

    inputProcessor::InputProcessor& getInputProcessor()
    {
        return m_inputProcessor;
    }

    const inputProcessor::InputProcessor& getInputProcessor() const
    {
        return m_inputProcessor;
    }

    Scene()
    {
        using namespace component;

        (void)component::MeshRenderer::componentDescriptor();
        (void)component::PointLight::componentDescriptor();
        (void)component::ScriptComponent::componentDescriptor();
        (void)physics::RigidBody::componentDescriptor();
        (void)physics::SphereCollider::componentDescriptor();
        (void)physics::PlaneCollider::componentDescriptor();
        (void)physics::BoxCollider::componentDescriptor();

        m_shaders = useShaderAsset("built-in/shaders/lit");

        m_meshs = new component::Mesh*[3];
        m_meshsCount = 3;
        m_meshs[0] = useMeshAsset("built-in/mesh/cube_n.obj");
        m_meshs[1] = useProceduralPlaneAsset();
        m_meshs[2] = useMeshAsset("built-in/mesh/unit_sphere_n.off");

        this->m_materials = useMaterialAsset("built-in/materials/lit_default.mat");
        m_selectionHighlightShader = useShaderAsset("built-in/shaders/unlit");
        m_selectionHighlightMaterial = dynamic_cast<dataStruct::UnlitMaterial*>(useMaterialAsset("built-in/materials/selection_highlight.mat"));
        if (m_selectionHighlightMaterial != nullptr)
            m_selectionHighlightMaterial->setRuntimePreviewSyncEnabled(false);
        m_camera = Camera();

        m_inputProcessor.registerKey(GLFW_KEY_W, inputProcessor::KeyState::HOLD);
        m_inputProcessor.registerKey(GLFW_KEY_S, inputProcessor::KeyState::HOLD);
        m_inputProcessor.registerKey(GLFW_KEY_A, inputProcessor::KeyState::HOLD);
        m_inputProcessor.registerKey(GLFW_KEY_D, inputProcessor::KeyState::HOLD);
        m_inputProcessor.registerKey(GLFW_KEY_E, inputProcessor::KeyState::HOLD);
        m_inputProcessor.registerKey(GLFW_KEY_Q, inputProcessor::KeyState::HOLD);
        m_inputProcessor.registerKey(GLFW_KEY_G, inputProcessor::KeyState::ONCE);
        m_inputProcessor.registerKey(GLFW_KEY_SEMICOLON, inputProcessor::KeyState::ONCE);
        m_inputProcessor.registerKey(GLFW_KEY_P, inputProcessor::KeyState::ONCE);
        m_inputProcessor.registerKey(GLFW_KEY_C, inputProcessor::KeyState::ONCE);
        m_inputProcessor.registerKey(GLFW_KEY_UP, inputProcessor::KeyState::ONCE);
        m_inputProcessor.registerKey(GLFW_KEY_DOWN, inputProcessor::KeyState::ONCE);
        m_inputProcessor.registerKey(GLFW_KEY_V, inputProcessor::KeyState::ONCE);
        m_inputProcessor.registerKey(GLFW_KEY_Z, inputProcessor::KeyState::ONCE);
        m_inputProcessor.registerKey(GLFW_KEY_Y, inputProcessor::KeyState::HOLD);
    }

    void update(
        double deltaTime,
        GLFWwindow* window,
        bool inputEnabled = true,
        bool lockMouseToAnchor = false,
        double mouseAnchorX = 0.0,
        double mouseAnchorY = 0.0)
    {
        refreshMeshesForChangedSources();
        refreshMaterialsForChangedShaders();

        if (m_physicsSimulationEnabled)
            PhysicEngine::getInstance()->Step(deltaTime);

        for (size_t i = 0; i < m_gameObjectCount; i++)
            m_gameObjects[i]->update(deltaTime);

        m_inputProcessor.update(window, inputEnabled && m_fpsControl, mouseAnchorX, mouseAnchorY);
        const float sensitivity = 0.1f;

        if (!inputEnabled)
            return;

        if (m_fpsControl)
        {
            glm::vec3 move = glm::vec3(0, 0, 0);
            unsigned int axisCount = 0;

            if (m_inputProcessor.queryKey(window, GLFW_KEY_W))
            {
                move += glm::vec3(0, 0, -1);
                axisCount++;
            }
            if (m_inputProcessor.queryKey(window, GLFW_KEY_S))
            {
                move += glm::vec3(0, 0, 1);
                axisCount++;
            }
            if (m_inputProcessor.queryKey(window, GLFW_KEY_A))
            {
                move += glm::vec3(-1, 0, 0);
                axisCount++;
            }
            if (m_inputProcessor.queryKey(window, GLFW_KEY_D))
            {
                move += glm::vec3(1, 0, 0);
                axisCount++;
            }
            if (m_inputProcessor.queryKey(window, GLFW_KEY_E))
            {
                move += glm::vec3(0, 1, 0);
                axisCount++;
            }
            if (m_inputProcessor.queryKey(window, GLFW_KEY_Q))
            {
                move += glm::vec3(0, -1, 0);
                axisCount++;
            }

            if (axisCount > 0)
                move = static_cast<float>(deltaTime) * (move / static_cast<float>(axisCount));

            updateCamera(move, glm::vec3(-m_inputProcessor.getMouseDeltaY() * sensitivity, m_inputProcessor.getMouseDeltaX() * sensitivity, 0.0f));
            if (lockMouseToAnchor)
                glfwSetCursorPos(window, mouseAnchorX, mouseAnchorY);
            else
            {
                int scrWidth = 0;
                int scrHeight = 0;
                glfwGetWindowSize(window, &scrWidth, &scrHeight);
                glfwSetCursorPos(window, scrWidth / 2, scrHeight / 2);
            }
        }

        if (m_inputProcessor.queryKey(window, GLFW_KEY_G))
        {
            if (m_orbitMode)
            {
                m_orbitMode = false;
                std::cout << "orbitmode disabled" << std::endl;
            }

            setFpsControlEnabled(!m_fpsControl, window, lockMouseToAnchor, mouseAnchorX, mouseAnchorY);
            std::cout << "FPS Camera control " << (m_fpsControl ? "enabled" : "disabled") << std::endl;
        }

        if (m_inputProcessor.queryKey(window, GLFW_KEY_Y))
            m_gameObjects[0]->getComponent<physics::RigidBody>()->Impulse(glm::vec3(9.81f, 2.0f * 9.81f, 0.0f) * static_cast<float>(deltaTime));

        if (m_inputProcessor.queryKey(window, GLFW_KEY_UP))
        {
            m_orbitSpeed += 5.0f;
            std::cout << "Orbit speed : " << m_orbitSpeed << std::endl;
        }
        if (m_inputProcessor.queryKey(window, GLFW_KEY_DOWN))
        {
            m_orbitSpeed -= 5.0f;
            std::cout << "Orbit speed : " << m_orbitSpeed << std::endl;
        }

        if (m_inputProcessor.queryKey(window, GLFW_KEY_C))
        {
            if (m_fpsControl)
            {
                setFpsControlEnabled(false, window, lockMouseToAnchor, mouseAnchorX, mouseAnchorY);
                std::cout << "FPS Camera control disabled" << std::endl;
            }

            m_orbitMode = !m_orbitMode;
            std::cout << "orbitmode : " << (m_orbitMode ? "enabled" : "disabled") << std::endl;
            if (m_orbitMode)
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        if (m_inputProcessor.queryKey(window, GLFW_KEY_Z))
        {
            for (size_t i = 0; i < m_gameObjectCount; i++)
            {
                if (MeshRenderer* meshRenderer = m_gameObjects[i]->getComponent<MeshRenderer>())
                    meshRenderer->toggleWireframe();
            }
        }

        if (m_orbitMode)
        {
            m_orbitYangle += m_orbitSpeed * static_cast<float>(deltaTime);
            m_orbitYangle = m_orbitYangle > 360.0f ? m_orbitYangle - 360.0f : m_orbitYangle;
            m_orbitYangle = m_orbitYangle < 0.0f ? m_orbitYangle + 360.0f : m_orbitYangle;
            const glm::vec3 nCamPos = glm::quat(glm::radians(glm::vec3(0.0f, m_orbitYangle, 0.0f))) * m_orbitPos;
            m_camera.m_position = nCamPos;
            m_camera.m_orientation = glm::vec3(-45.0f, m_orbitYangle + 180.0f, 0.0f);
        }
    }

    void renderScene()
    {
        m_renderPipeline.execute(m_camera, m_gameObjects, m_gameObjectCount, m_savedRenderTargetSettings, collectLightInputs());
    }

    void renderSceneWithSelectionHighlight()
    {
        renderScene();
        renderSelectedHighlight();
    }

    size_t getGameObjectCount() const
    {
        return m_gameObjectCount;
    }

    GameObject* addGameObject(const std::string& name)
    {
        ensureGameObjectCapacity(m_gameObjectCount + 1);

        GameObject* gameObject = new GameObject(name);
        gameObject->setScene(this);
        gameObject->setId(m_nextGameObjectId++);
        m_gameObjects[m_gameObjectCount++] = gameObject;
        m_renderPipeline.invalidate();
        return gameObject;
    }

    GameObject* addGameObject(const std::string& name, int id)
    {
        ensureGameObjectCapacity(m_gameObjectCount + 1);

        GameObject* gameObject = new GameObject(name);
        gameObject->setScene(this);
        gameObject->setId(id);
        if (id >= m_nextGameObjectId)
            m_nextGameObjectId = id + 1;
        m_gameObjects[m_gameObjectCount++] = gameObject;
        m_renderPipeline.invalidate();
        return gameObject;
    }

    bool removeGameObject(int id)
    {
        for (size_t index = 0; index < m_gameObjectCount; ++index)
        {
            GameObject* gameObject = m_gameObjects[index];
            if (gameObject == nullptr || gameObject->getId() != id)
                continue;

            if (m_selectedGameObject == gameObject)
                m_selectedGameObject = nullptr;

            detachGameObjectFromHierarchy(gameObject);
            delete gameObject;

            for (size_t moveIndex = index + 1; moveIndex < m_gameObjectCount; ++moveIndex)
                m_gameObjects[moveIndex - 1] = m_gameObjects[moveIndex];

            m_gameObjects[m_gameObjectCount - 1] = nullptr;
            m_gameObjectCount--;
            m_renderPipeline.invalidate();
            return true;
        }

        return false;
    }

    GameObject* getGameObjectById(int id) const
    {
        for (size_t index = 0; index < m_gameObjectCount; ++index)
        {
            GameObject* gameObject = m_gameObjects[index];
            if (gameObject != nullptr && gameObject->getId() == id)
                return gameObject;
        }

        return nullptr;
    }

    GameObject* getGameObject(size_t index) const
    {
        return index < m_gameObjectCount ? m_gameObjects[index] : nullptr;
    }

    GameObject* getSelectedGameObject() const
    {
        return m_selectedGameObject;
    }

    void setSelectedGameObject(GameObject* gameObject)
    {
        m_selectedGameObject = gameObject;
    }

    void setSelectedGameObjectById(int id)
    {
        m_selectedGameObject = getGameObjectById(id);
    }

    void toggleSelectedGameObject(GameObject* gameObject)
    {
        if (gameObject == nullptr)
        {
            m_selectedGameObject = nullptr;
            return;
        }

        m_selectedGameObject = (m_selectedGameObject == gameObject) ? nullptr : gameObject;
    }

    void setSceneScriptAssetPath(const std::string& assetPath)
    {
        m_sceneScriptAssetPath = asset::AssetManager::normalizeRelativePath(assetPath);
    }

    const std::string& getSceneScriptAssetPath() const
    {
        return m_sceneScriptAssetPath;
    }

    void setDataAssetPath(const std::string& assetPath)
    {
        m_dataAssetPath = asset::AssetManager::normalizeRelativePath(assetPath);
    }

    const std::string& getDataAssetPath() const
    {
        return m_dataAssetPath;
    }

    const render::DirectionalLightSettings& getDirectionalLightSettings() const
    {
        return m_directionalLight;
    }

    bool setDirectionalLightSettings(const render::DirectionalLightSettings& settings)
    {
        if (m_directionalLight.enabled == settings.enabled &&
            m_directionalLight.direction == settings.direction &&
            m_directionalLight.color == settings.color &&
            m_directionalLight.intensity == settings.intensity)
            return false;

        m_directionalLight = settings;
        return true;
    }

    const std::vector<render::SceneRenderTargetSettings>& getSavedRenderTargetSettings() const
    {
        return m_savedRenderTargetSettings;
    }

    void setSavedRenderTargetSettings(const std::vector<render::SceneRenderTargetSettings>& settings)
    {
        m_savedRenderTargetSettings.clear();
        for (const render::SceneRenderTargetSettings& entry : settings)
            upsertRenderTargetSettings(entry);
        m_renderPipeline.invalidate();
    }

    render::SceneRenderTargetSettings resolveRenderTargetSettings(const std::string& rawName) const
    {
        render::SceneRenderTargetSettings resolved;
        resolved.name = render::normalizeRenderTargetName(rawName);
        if (const render::SceneRenderTargetSettings* savedSettings = findSavedRenderTargetSettings(resolved.name))
            return *savedSettings;
        return resolved;
    }

    static render::SceneRenderTargetSettings applyDeclaredRenderTargetSettings(
        const render::SceneRenderTargetSettings& baseSettings,
        const asset::RenderTargetAssetReference& declaration)
    {
        render::SceneRenderTargetSettings resolved = baseSettings;
        if (declaration.width > 0)
            resolved.width = declaration.width;
        if (declaration.height > 0)
            resolved.height = declaration.height;
        if (declaration.hasFormat)
            resolved.format = declaration.format;
        return resolved;
    }

    bool upsertRenderTargetSettings(const render::SceneRenderTargetSettings& rawSettings)
    {
        render::SceneRenderTargetSettings normalizedSettings = rawSettings;
        normalizedSettings.name = render::normalizeRenderTargetName(rawSettings.name);
        normalizedSettings.width = std::max(0, rawSettings.width);
        normalizedSettings.height = std::max(0, rawSettings.height);
        normalizedSettings.bakedTextureAssetPath = asset::AssetManager::normalizeRelativePath(rawSettings.bakedTextureAssetPath);
        if (normalizedSettings.name.empty() || render::isFinalRenderTargetName(normalizedSettings.name))
            return false;

        render::SceneRenderTargetSettings* existing = findSavedRenderTargetSettings(normalizedSettings.name);
        if (existing != nullptr)
        {
            if (existing->width == normalizedSettings.width &&
                existing->height == normalizedSettings.height &&
                existing->format == normalizedSettings.format &&
                existing->bakedTextureAssetPath == normalizedSettings.bakedTextureAssetPath)
                return false;

            *existing = normalizedSettings;
        }
        else
        {
            m_savedRenderTargetSettings.push_back(normalizedSettings);
        }

        std::sort(m_savedRenderTargetSettings.begin(), m_savedRenderTargetSettings.end(), [](const render::SceneRenderTargetSettings& lhs, const render::SceneRenderTargetSettings& rhs) {
            return lhs.name < rhs.name;
        });
        m_renderPipeline.invalidate();
        return true;
    }
        bool bakeRenderTargets(std::vector<render::RenderPipeline::BakedRenderTargetResult>& outputs)
        {
            refreshMaterialsForChangedShaders();
            refreshMeshesForChangedSources();
            return m_renderPipeline.bake(m_camera, m_gameObjects, m_gameObjectCount, collectVisibleRenderTargetSettings(), collectLightInputs(), outputs);
        }

    std::vector<render::SceneRenderTargetSettings> collectVisibleRenderTargetSettings() const
    {
        std::vector<render::SceneRenderTargetSettings> settings = m_savedRenderTargetSettings;
        const auto appendTargetIfMissing = [&settings, this](const asset::RenderTargetAssetReference& target) {
            const std::string& rawName = target.name;
            const std::string normalizedName = render::normalizeRenderTargetName(rawName);
            if (normalizedName.empty() || render::isFinalRenderTargetName(normalizedName))
                return;

            for (const render::SceneRenderTargetSettings& entry : settings)
            {
                if (render::normalizeRenderTargetName(entry.name) == normalizedName)
                    return;
            }

            settings.push_back(applyDeclaredRenderTargetSettings(resolveRenderTargetSettings(normalizedName), target));
        };

        for (size_t index = 0; index < m_gameObjectCount; ++index)
        {
            GameObject* gameObject = m_gameObjects[index];
            if (gameObject == nullptr)
                continue;

            MeshRenderer* meshRenderer = gameObject->getComponent<MeshRenderer>();
            if (meshRenderer == nullptr)
                continue;

            dataStruct::Material* material = nullptr;
            const std::string materialAssetPath = asset::AssetManager::normalizeRelativePath(meshRenderer->getMaterialAssetPath());
            if (!materialAssetPath.empty())
                material = asset::AssetManager::instance().loadMaterial(materialAssetPath);
            else
                material = meshRenderer->getMaterial();
            if (material == nullptr)
                continue;

            const std::string renderPassPath = asset::AssetManager::normalizeRelativePath(material->getRuntimeDefinition().renderPassPath);
            if (renderPassPath.empty())
                continue;

            asset::RenderPassAssetDefinition* renderPass = nullptr;
            if (!asset::AssetManager::instance().reloadRenderPassDefinition(renderPassPath, renderPass) || renderPass == nullptr)
                continue;

            for (const asset::RenderPassStepDefinition& pass : renderPass->passes)
            {
                appendTargetIfMissing(pass.target);
                for (const asset::RenderPassUniformDefinition& uniform : pass.uniforms)
                {
                    if (uniform.kind == asset::RenderPassUniformKind::RenderTarget)
                        appendTargetIfMissing(uniform.renderTargetValue);
                }
            }
        }

        std::sort(settings.begin(), settings.end(), [](const render::SceneRenderTargetSettings& lhs, const render::SceneRenderTargetSettings& rhs) {
            return lhs.name < rhs.name;
        });
        return settings;
    }

    std::vector<render::LightInput> collectLightInputs() const
    {
        std::vector<render::LightInput> lights;
        if (m_directionalLight.enabled)
        {
            render::LightInput directionalLight;
            directionalLight.type = render::LightType::Directional;
            directionalLight.direction = m_directionalLight.direction;
            directionalLight.color = m_directionalLight.color;
            directionalLight.intensity = m_directionalLight.intensity;
            lights.push_back(directionalLight);
        }

        for (size_t index = 0; index < m_gameObjectCount; ++index)
        {
            GameObject* gameObject = m_gameObjects[index];
            if (gameObject == nullptr)
                continue;

            component::PointLight* pointLight = gameObject->getComponent<component::PointLight>();
            if (pointLight == nullptr || !pointLight->isEnabled())
                continue;

            render::LightInput light;
            light.type = render::LightType::Point;
            light.position = gameObject->transform.getPosition();
            light.color = pointLight->getColor();
            light.intensity = pointLight->getIntensity();
            lights.push_back(light);
        }

        return lights;
    }

    std::vector<std::string> collectReferencedRenderPassAssets() const
    {
        std::vector<std::string> renderPassPaths;
        std::unordered_set<std::string> seenRenderPassPaths;

        for (size_t index = 0; index < m_gameObjectCount; ++index)
        {
            GameObject* gameObject = m_gameObjects[index];
            if (gameObject == nullptr)
                continue;

            MeshRenderer* meshRenderer = gameObject->getComponent<MeshRenderer>();
            if (meshRenderer == nullptr)
                continue;

            dataStruct::Material* material = nullptr;
            const std::string materialAssetPath = asset::AssetManager::normalizeRelativePath(meshRenderer->getMaterialAssetPath());
            if (!materialAssetPath.empty())
                material = asset::AssetManager::instance().loadMaterial(materialAssetPath);
            else
                material = meshRenderer->getMaterial();

            if (material == nullptr)
                continue;

            const std::string renderPassPath = asset::AssetManager::normalizeRelativePath(material->getRuntimeDefinition().renderPassPath);
            if (renderPassPath.empty() || !seenRenderPassPaths.insert(renderPassPath).second)
                continue;

            renderPassPaths.push_back(renderPassPath);
        }

        std::sort(renderPassPaths.begin(), renderPassPaths.end());
        return renderPassPaths;
    }

    void updateCamera(glm::vec3 deltaPos, glm::vec3 deltaEuler)
    {
        glm::vec4 posRel = (Transform::rotationMatrix(glm::vec3(0.0f, m_camera.m_orientation.y, 0.0f)) * glm::vec4(deltaPos.x, deltaPos.y, deltaPos.z, 1.0f));
        m_camera.m_position+= glm::vec3(posRel.x, posRel.y, posRel.z) * 10.0f;
        m_camera.m_orientation+=deltaEuler;

        
        // clamp euler.
        m_camera.m_orientation.y += m_camera.m_orientation.y > 360 ? -360.0f : m_camera.m_orientation.y <= 0 ? +360.0f : 0.0f;
        m_camera.m_orientation.x = m_camera.m_orientation.x > 90 ? 90 : m_camera.m_orientation.x <= -90 ? -90.0f : m_camera.m_orientation.x;
        m_camera.m_orientation.z += m_camera.m_orientation.z > 360 ? -360.0f : m_camera.m_orientation.z <= 0 ? +360.0f : 0.0f;
    }

    void updateCamSettings(float aspectRatio)
    {
        m_camera.m_aspectRatio = aspectRatio;
    }

    Camera& getCamera()
    {
        return m_camera;
    }

    const Camera& getCamera() const
    {
        return m_camera;
    }

    component::Mesh* resolveMeshAsset(const std::string& relativePath)
    {
        const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(relativePath);
        if (normalizedPath == proceduralPlaneAssetPath())
            return useProceduralPlaneAsset();
        return useMeshAsset(normalizedPath);
    }

    Shader* resolveShaderAsset(const std::string& relativePath)
    {
        return useShaderAsset(relativePath);
    }

    dataStruct::Material* resolveMaterialAsset(const std::string& relativePath)
    {
        return useMaterialAsset(relativePath);
    }

    bool validateRenderPipelines(std::vector<std::string>* issues = nullptr) const
    {
        bool valid = true;
        for (const std::string& renderPassPath : collectReferencedRenderPassAssets())
        {
            std::vector<std::string> renderPassIssues;
            if (!render::RenderPipeline::validateCompiledRenderPass(renderPassPath, nullptr, &renderPassIssues))
            {
                valid = false;
                if (issues != nullptr)
                    issues->insert(issues->end(), renderPassIssues.begin(), renderPassIssues.end());
            }
        }

        return valid;
    }

    bool buildRenderPipelines(std::vector<std::string>* issues = nullptr)
    {
        bool valid = true;
        for (const std::string& renderPassPath : collectReferencedRenderPassAssets())
        {
            std::vector<std::string> renderPassIssues;
            if (!render::RenderPipeline::buildCompiledRenderPass(renderPassPath, &renderPassIssues))
            {
                valid = false;
                if (issues != nullptr)
                    issues->insert(issues->end(), renderPassIssues.begin(), renderPassIssues.end());
            }
        }

        if (valid)
            m_renderPipeline.invalidate();
        return valid;
    }

    bool refreshMaterialAsset(const std::string& relativePath)
    {
        const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(relativePath);
        if (normalizedPath.empty())
            return false;

        dataStruct::Material* material = nullptr;
        if (!asset::AssetManager::instance().reloadMaterial(normalizedPath, material) || material == nullptr)
            return false;

        if (normalizedPath == asset::AssetManager::normalizeRelativePath("built-in/materials/lit_default.mat"))
            m_materials = material;
        if (normalizedPath == asset::AssetManager::normalizeRelativePath("built-in/materials/selection_highlight.mat"))
            m_selectionHighlightMaterial = dynamic_cast<dataStruct::UnlitMaterial*>(material);

        for (size_t index = 0; index < m_gameObjectCount; ++index)
        {
            GameObject* gameObject = m_gameObjects[index];
            if (gameObject == nullptr)
                continue;

            MeshRenderer* meshRenderer = gameObject->getComponent<MeshRenderer>();
            if (meshRenderer == nullptr)
                continue;

            if (asset::AssetManager::normalizeRelativePath(meshRenderer->getMaterialAssetPath()) != normalizedPath)
                continue;

            meshRenderer->setMaterial(material);
        }

        m_renderPipeline.invalidate();

        return true;
    }

    void clearGameObjects()
    {
        destroyAllGameObjects();
        m_renderPipeline.invalidate();
    }

    void clearSceneAssetRegistry()
    {
        m_sceneAssetRegistry.clear();
    }

    void setNextGameObjectId(int nextId)
    {
        m_nextGameObjectId = std::max(1, nextId);
    }

    int getNextGameObjectId() const
    {
        return m_nextGameObjectId;
    }

    bool isFpsControlEnabled() const
    {
        return m_fpsControl;
    }

    bool isOrbitModeEnabled() const
    {
        return m_orbitMode;
    }

    void setPhysicsSimulationEnabled(bool enabled)
    {
        m_physicsSimulationEnabled = enabled;
    }

    bool isPhysicsSimulationEnabled() const
    {
        return m_physicsSimulationEnabled;
    }

    const asset::SceneAssetRegistry& getSceneAssetRegistry() const
    {
        return m_sceneAssetRegistry;
    }

    ~Scene()
    {
        destroyAllGameObjects();
        if (m_meshs != nullptr && m_meshsCount > 1 && m_meshs[1] != nullptr)
        {
            if (m_meshs[1]->ownerCount > 0)
                m_meshs[1]->ownerCount--;
            if (m_meshs[1]->ownerCount == 0)
                delete m_meshs[1];
            m_meshs[1] = nullptr;
        }
        delete[] m_gameObjects;
        delete[] m_meshs;
    }
};