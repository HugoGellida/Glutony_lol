#pragma once

#include "Camera.hpp"
#include "gameobject/GameObject.hpp"
#include "common/shader/Material.hpp"
#include "common/shader/LitMaterial.hpp"
#include "common/shader/UnlitMaterial.hpp"
#include <common/shader/Shader.hpp>
#include "gameobject/component/Mesh.hpp"
#include "gameobject/component/MeshRenderer.hpp"
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
#include "ui/UIRenderer.hpp"
#include "utils/Raycast.hpp"

#include <memory>
#include <string>
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
    glm::vec3 m_orbitPos = glm::vec3(0.0f, 10.0f, -10.0f);
    float m_orbitYangle = 0.0f;
    float m_orbitSpeed = 20.0f;

    float m_anim_angle = 0.0f;
    Rml::Context* m_uiContext = nullptr;
    int m_uiViewportX = 0;
    int m_uiViewportY = 0;
    int m_uiViewportWidth = 0;
    int m_uiViewportHeight = 0;
    std::vector<std::unique_ptr<UIRenderer>> m_uiRenderers;
    asset::SceneAssetRegistry m_sceneAssetRegistry;

    static std::string proceduralPlaneAssetPath()
    {
        return "procedural/plane";
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

    void updateUiRenderers(double deltaTime)
    {
        for (const std::unique_ptr<UIRenderer>& renderer : m_uiRenderers)
            renderer->update(deltaTime);
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
    Scene()
    {
        using namespace component;

        (void)component::MeshRenderer::componentDescriptor();
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


        this -> m_materials = useMaterialAsset("built-in/materials/lit_default.mat");
        m_selectionHighlightShader = useShaderAsset("built-in/shaders/unlit");
        m_selectionHighlightMaterial = dynamic_cast<dataStruct::UnlitMaterial*>(useMaterialAsset("built-in/materials/selection_highlight.mat"));
        m_camera = Camera();


        // input setup
        m_inputProcessor.registerKey(GLFW_KEY_W, inputProcessor::KeyState::HOLD);
        m_inputProcessor.registerKey(GLFW_KEY_S, inputProcessor::KeyState::HOLD);
        m_inputProcessor.registerKey(GLFW_KEY_A, inputProcessor::KeyState::HOLD);
        m_inputProcessor.registerKey(GLFW_KEY_D, inputProcessor::KeyState::HOLD);
        m_inputProcessor.registerKey(GLFW_KEY_E, inputProcessor::KeyState::HOLD);
        m_inputProcessor.registerKey(GLFW_KEY_Q, inputProcessor::KeyState::HOLD);
        m_inputProcessor.registerKey(GLFW_KEY_G, inputProcessor::KeyState::ONCE);
        m_inputProcessor.registerKey(GLFW_KEY_SEMICOLON, inputProcessor::KeyState::ONCE); // M
        m_inputProcessor.registerKey(GLFW_KEY_P, inputProcessor::KeyState::ONCE);
        m_inputProcessor.registerKey(GLFW_KEY_C, inputProcessor::KeyState::ONCE);
        m_inputProcessor.registerKey(GLFW_KEY_UP, inputProcessor::KeyState::ONCE);
        m_inputProcessor.registerKey(GLFW_KEY_DOWN, inputProcessor::KeyState::ONCE);
        m_inputProcessor.registerKey(GLFW_KEY_V, inputProcessor::KeyState::ONCE);
        m_inputProcessor.registerKey(GLFW_KEY_Z, inputProcessor::KeyState::ONCE); // w
        m_inputProcessor.registerKey(GLFW_KEY_Y, inputProcessor::KeyState::HOLD);
    }

    void update(
        double deltaTime,
        GLFWwindow * window,
        bool inputEnabled = true,
        bool editorMode = false,
        double mouseAnchorX = 0.0,
        double mouseAnchorY = 0.0,
        bool sceneClickPending = false,
        double sceneClickX = 0.0,
        double sceneClickY = 0.0)
    {
        if (m_physicsSimulationEnabled)
            PhysicEngine::getInstance() -> Step(deltaTime);



        for (size_t i = 0; i < m_gameObjectCount; i++) 
            m_gameObjects[i] -> update(deltaTime);
        m_inputProcessor.update(window, inputEnabled && m_fpsControl, mouseAnchorX, mouseAnchorY);
        float sensitivity = 0.1f;
        
        if (!inputEnabled && !sceneClickPending)
        {
            updateUiRenderers(deltaTime);
            return;
        }

        if (m_fpsControl)
        {
            //Camera zoom in and out
            float cameraSpeed = 2.5 * deltaTime;
            glm::vec3 move = glm::vec3(0, 0, 0);
            unsigned int a = 0;
            // CAMERA
            if (m_inputProcessor.queryKey(window, GLFW_KEY_W))
            {
                move+=glm::vec3(0, 0, -1);
                a++;
            }
            if (m_inputProcessor.queryKey(window, GLFW_KEY_S))
            {
                move+=glm::vec3(0, 0, 1);
                a++;
            }
            if (m_inputProcessor.queryKey(window, GLFW_KEY_A))
            {
                move+=glm::vec3(-1, 0, 0);
                a++;
            }
            if (m_inputProcessor.queryKey(window, GLFW_KEY_D))
            {
                move+=glm::vec3(1, 0, 0);
                a++;
            }
            if (m_inputProcessor.queryKey(window, GLFW_KEY_E))
            {
                move+=glm::vec3(0, 1, 0);
                a++;
            }
            if (m_inputProcessor.queryKey(window, GLFW_KEY_Q))
            {
                move+=glm::vec3(0, -1, 0);
                a++;
            }

            if (a>0)
                move = ((float)deltaTime) * (move / (float)a);
            updateCamera(move, glm::vec3(-m_inputProcessor.getMouseDeltaY() * sensitivity, m_inputProcessor.getMouseDeltaX() * sensitivity, 0.0f));
            if (editorMode)
                glfwSetCursorPos(window, mouseAnchorX, mouseAnchorY);
            else
            {
                int scrWidth, scrHeight;
                glfwGetWindowSize(window, &scrWidth, &scrHeight);
                glfwSetCursorPos(window, scrWidth / 2, scrHeight / 2);
            }
        }

        if (sceneClickPending && !m_fpsControl)
        {
            Raycast::Ray ray = Raycast::getRayFromClick(editorMode, m_uiViewportX, m_uiViewportY, m_uiViewportWidth, m_uiViewportHeight, sceneClickX, sceneClickY, m_camera, window);
            float m = MAXFLOAT;
            GameObject* selectedGameObject = nullptr;
            for (size_t i = 0; i < m_gameObjectCount; i++)
            {
                float t = MAXFLOAT;
                if (m_gameObjects[i] -> getComponent<Mesh>() != nullptr)
                {
                    Raycast::raycastTransformedAABB(m_gameObjects[i]->transform, m_gameObjects[i] -> getComponent<Mesh>() -> getAABB(), ray, &t);
                } 
                else if (m_gameObjects[i] -> getComponent<physics::RigidBody>() != nullptr)
                    Raycast::raycastAABB(m_gameObjects[i] -> getComponent<physics::RigidBody>() -> getAABB(), ray, &t);
                if (t < m && t > 0.0)
                {
                    selectedGameObject = m_gameObjects[i];
                    m = t;
                }
            }

            setSelectedGameObject(selectedGameObject);
        }

        if (m_inputProcessor.queryKey(window, GLFW_KEY_G))
        {
            if (m_orbitMode)
            {
                m_orbitMode = false;
                std::cout << "orbitmode disabled" << std::endl;
            }
            m_fpsControl = !m_fpsControl;
            std::cout << "FPS Camera control " << (m_fpsControl ? "enabled" : "disabled") << std::endl;
            if (m_fpsControl)
                glfwSetInputMode(window, GLFW_CURSOR, editorMode ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_DISABLED);
            else
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        if (m_inputProcessor.queryKey(window, GLFW_KEY_Y))
            m_gameObjects[0] -> getComponent<physics::RigidBody>() -> Impulse(glm::vec3(9.81, 2 * 9.81f, 0) * (float)deltaTime);

        

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
                m_fpsControl = false;
                std::cout << "FPS Camera control disabled" << std::endl;
            }
            m_orbitMode = !m_orbitMode;
            std::cout << "orbitmode : " << (m_orbitMode ? "enabled" : "disabled") << std::endl;
            if (m_orbitMode)
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                
            }
        }

        if (m_inputProcessor.queryKey(window, GLFW_KEY_Z))
        {
            for (size_t i = 0; i < m_gameObjectCount; i++)
                m_gameObjects[i] -> getComponent<MeshRenderer>()->toggleWireframe();
        }

        if (m_orbitMode)
        {
            m_orbitYangle += m_orbitSpeed * ((float)deltaTime);
            m_orbitYangle = m_orbitYangle > 360.0f ? m_orbitYangle - 360.0f : m_orbitYangle;
            m_orbitYangle = m_orbitYangle < 0.0f ? m_orbitYangle + 360.0f : m_orbitYangle;
            glm::vec3 nCamPos = glm::quat(glm::radians(glm::vec3(0.0, m_orbitYangle, 0.0f))) * m_orbitPos;
            m_camera.m_position = nCamPos;
            m_camera.m_orientation = glm::vec3(-45.0f, m_orbitYangle + 180.0f, 0.0f);

        }


       /*  m_anim_angle += 10.0f * deltaTime;
        m_anim_angle = m_anim_angle > 360.0f ? m_anim_angle - 360.0f : m_anim_angle;

        m_gameObjects[0] -> transform.setRotation(glm::vec3(0.0f, m_anim_angle + 30.0f, 20.0f));
        m_gameObjects[1] -> transform.setRotation(glm::vec3(10.0f, -m_anim_angle, 40.0f)); */

        updateUiRenderers(deltaTime);

    }

    void renderScene()
    {
        for (size_t i = 0; i < m_gameObjectCount; i++)
        {
            renderGameObject(m_gameObjects[i]);
        }
    }

    void renderSceneWithSelectionHighlight()
    {
        for (size_t i = 0; i < m_gameObjectCount; i++)
            renderGameObject(m_gameObjects[i]);

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

    void setUiContext(Rml::Context* context)
    {
        m_uiContext = context;
        for (const std::unique_ptr<UIRenderer>& renderer : m_uiRenderers)
            renderer->initialize(m_uiContext);
    }

    UIRenderer* pushUiRenderer(std::unique_ptr<UIRenderer> renderer)
    {
        if (!renderer)
            return nullptr;

        renderer->initialize(m_uiContext);
        m_uiRenderers.push_back(std::move(renderer));
        return m_uiRenderers.back().get();
    }

    void clearUiRenderers()
    {
        for (const std::unique_ptr<UIRenderer>& renderer : m_uiRenderers)
            renderer->shutdown();
        m_uiRenderers.clear();
    }

    bool hasUiRenderers() const
    {
        return !m_uiRenderers.empty();
    }

    void setUiViewportRect(int x, int y, int width, int height)
    {
        m_uiViewportX = x;
        m_uiViewportY = y;
        m_uiViewportWidth = width;
        m_uiViewportHeight = height;
    }

    void renderUi()
    {
        if (m_uiViewportWidth <= 0 || m_uiViewportHeight <= 0)
        {
            for (const std::unique_ptr<UIRenderer>& renderer : m_uiRenderers)
                renderer->setVisible(false);
            return;
        }

        for (size_t index = 0; index < m_uiRenderers.size(); ++index)
        {
            UIRenderer& renderer = *m_uiRenderers[index];
            renderer.layoutFullscreen(m_uiViewportX, m_uiViewportY, m_uiViewportWidth, m_uiViewportHeight, static_cast<int>(index));
            renderer.pullToFront();
        }
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

    void clearGameObjects()
    {
        destroyAllGameObjects();
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
        clearUiRenderers();
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