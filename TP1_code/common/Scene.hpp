#pragma once

#include "Camera.hpp"
#include "gameobject/GameObject.hpp"
#include "common/shader/Material.hpp"
#include "common/shader/LitMaterial.hpp"
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
#include "ui/UIRenderer.hpp"

#include <memory>
#include <vector>


class Scene
{
private:
    GameObject ** m_gameObjects = nullptr;
    size_t m_meshsCount=0;
    component::Mesh ** m_meshs=nullptr;
    
    size_t m_gameObjectCount = 0;
    dataStruct::Material * m_materials;
    Shader * m_shaders;
    PhysicsSystem ph;

    Camera m_camera;
    inputProcessor::InputProcessor m_inputProcessor;
    bool m_fpsControl = false;
    bool m_orbitMode = false;
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

    void updateUiRenderers(double deltaTime)
    {
        for (const std::unique_ptr<UIRenderer>& renderer : m_uiRenderers)
            renderer->update(deltaTime);
    }

public:
    Scene()
    {
        using namespace component;

        m_shaders = new Shader("./built-in_shaders/lit/vertex.glsl", "./built-in_shaders/lit/fragment.glsl");

        m_meshs = new component::Mesh*[3];
        m_meshsCount = 3;
        m_meshs[0] = fileLoader::loadModelFile("./built-in_mesh/cube_n.obj");
        m_meshs[1] = new Plane(glm::vec3(0, 0, 0), 10, 2);
        m_meshs[2] = fileLoader::loadModelFile("./built-in_mesh/unit_sphere_n.off");


        this -> m_materials = new dataStruct::LitMaterial(m_shaders);
        
        m_gameObjects = new GameObject*[8];
        m_gameObjectCount = 0;
        {
            m_gameObjects[m_gameObjectCount] = new GameObject("Cube");
            m_gameObjects[m_gameObjectCount] -> addComponent(m_meshs[0]);
            m_gameObjects[m_gameObjectCount] -> addComponent(new MeshRenderer(m_gameObjects[m_gameObjectCount] -> getComponent<Mesh>(), m_materials));
            m_gameObjects[m_gameObjectCount] -> setPosition(glm::vec3(0.0, 1.0, 0.0));
            m_gameObjects[m_gameObjectCount] -> addComponent(new physics::RigidBody(m_gameObjects[m_gameObjectCount]));
            physics::RigidBody & rb = *m_gameObjects[m_gameObjectCount] -> getComponent<physics::RigidBody>();
            rb.useGravity = true;
            rb.m_linearVelocity = glm::vec3(0.0, 0.0, 0.0);
            rb.staticFriction = 1.0f;
            rb.dynamicFriction = 1.0f;
            rb.bounciness = 1.0f;
            rb.mass = 0.2f;
            rb.RecomputeInverseMass();
            m_gameObjects[m_gameObjectCount] -> addComponent(new physics::BoxCollider(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.5, 0.5, 0.5), glm::vec3(1.0, 1.0, 1.0)));
        }
        m_gameObjectCount++;
        {
            m_gameObjects[m_gameObjectCount] = new GameObject("Sphere");
            m_gameObjects[m_gameObjectCount] -> addComponent(m_meshs[2]);
            m_gameObjects[m_gameObjectCount] -> addComponent(new MeshRenderer(m_gameObjects[m_gameObjectCount] -> getComponent<Mesh>(), m_materials));
            m_gameObjects[m_gameObjectCount] -> setPosition(glm::vec3(0.0, 5.0, 0.0));
            m_gameObjects[m_gameObjectCount] -> setScale(glm::vec3(0.1, 0.1, 0.1));
            m_gameObjects[m_gameObjectCount] -> addComponent(new physics::RigidBody(m_gameObjects[m_gameObjectCount]));
            physics::RigidBody & rb = *m_gameObjects[m_gameObjectCount] -> getComponent<physics::RigidBody>();
            rb.useGravity = true;
            rb.m_linearVelocity = glm::vec3(0.001, 0.0, 0.0);
            rb.staticFriction = 1.0f;
            rb.dynamicFriction = 1.0f;
            rb.bounciness = 0.2f;
            rb.mass = 0.1f;
            rb.RecomputeInverseMass();
            m_gameObjects[m_gameObjectCount] -> addComponent(new physics::SphereCollider(glm::vec3(0.0, 0.0, 0.0), 0.1f));
        }
        m_gameObjectCount++;
        {
            m_gameObjects[m_gameObjectCount] = new GameObject("Sphere");
            m_gameObjects[m_gameObjectCount] -> addComponent(m_meshs[2]);
            m_gameObjects[m_gameObjectCount] -> addComponent(new MeshRenderer(m_gameObjects[m_gameObjectCount] -> getComponent<Mesh>(), m_materials));
            m_gameObjects[m_gameObjectCount] -> setPosition(glm::vec3(0.0, 5.0, 0.0));
            m_gameObjects[m_gameObjectCount] -> setScale(glm::vec3(0.1, 0.1, 0.1));
            m_gameObjects[m_gameObjectCount] -> addComponent(new physics::RigidBody(m_gameObjects[m_gameObjectCount]));
            physics::RigidBody & rb = *m_gameObjects[m_gameObjectCount] -> getComponent<physics::RigidBody>();
            rb.useGravity = true;
            rb.m_linearVelocity = glm::vec3(0.001, 0.0, 0.0);
            rb.staticFriction = 1.0f;
            rb.dynamicFriction = 1.0f;
            rb.bounciness = 0.2f;
            rb.mass = 0.1f;
            rb.RecomputeInverseMass();
            m_gameObjects[m_gameObjectCount] -> addComponent(new physics::SphereCollider(glm::vec3(0.0, 0.0, 0.0), 0.1f));
        }
        m_gameObjectCount++;
        {
            m_gameObjects[m_gameObjectCount] = new GameObject("Plane");
            m_gameObjects[m_gameObjectCount] -> addComponent(m_meshs[1]);
            m_gameObjects[m_gameObjectCount] -> addComponent(new MeshRenderer(m_gameObjects[m_gameObjectCount] -> getComponent<Mesh>(), m_materials));
            m_gameObjects[m_gameObjectCount] -> setPosition(glm::vec3(0, -1, 0));
            m_gameObjects[m_gameObjectCount] -> setScale(glm::vec3(1, 1, 1));
            m_gameObjects[m_gameObjectCount] -> addComponent(new physics::RigidBody(m_gameObjects[m_gameObjectCount]));
            physics::RigidBody & rb = *m_gameObjects[m_gameObjectCount] -> getComponent<physics::RigidBody>();
            rb.useGravity = false;
            rb.isStatic = true;
            rb.mass = 0.0f;
            rb.bounciness = 1.0f;
            rb.RecomputeInverseMass();
            m_gameObjects[m_gameObjectCount] -> addComponent(new physics::PlaneCollider(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0)));
        } 
        m_gameObjectCount++;
        {
            m_gameObjects[m_gameObjectCount] = new GameObject("Plane");
            m_gameObjects[m_gameObjectCount] -> addComponent(m_meshs[1]);
            m_gameObjects[m_gameObjectCount] -> addComponent(new MeshRenderer(m_gameObjects[m_gameObjectCount] -> getComponent<Mesh>(), m_materials));
            m_gameObjects[m_gameObjectCount] -> setPosition(glm::vec3(0, 4, 5));
            m_gameObjects[m_gameObjectCount] -> setScale(glm::vec3(1, 1, 1));
            m_gameObjects[m_gameObjectCount] -> setRotation(glm::vec3(-90.0f, 0.0f, 0.0f));
            m_gameObjects[m_gameObjectCount] -> addComponent(new physics::RigidBody(m_gameObjects[m_gameObjectCount]));
            physics::RigidBody & rb = *m_gameObjects[m_gameObjectCount] -> getComponent<physics::RigidBody>();
            rb.useGravity = false;
            rb.isStatic = true;
            rb.mass = 0.0f;
            rb.RecomputeInverseMass();
            m_gameObjects[m_gameObjectCount] -> addComponent(new physics::PlaneCollider(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0)));
        } 
        m_gameObjectCount++;
        {
            m_gameObjects[m_gameObjectCount] = new GameObject("Plane");
            m_gameObjects[m_gameObjectCount] -> addComponent(m_meshs[1]);
            m_gameObjects[m_gameObjectCount] -> addComponent(new MeshRenderer(m_gameObjects[m_gameObjectCount] -> getComponent<Mesh>(), m_materials));
            m_gameObjects[m_gameObjectCount] -> setPosition(glm::vec3(0, 4, -5));
            m_gameObjects[m_gameObjectCount] -> setScale(glm::vec3(1, 1, 1));
            m_gameObjects[m_gameObjectCount] -> setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
            m_gameObjects[m_gameObjectCount] -> addComponent(new physics::RigidBody(m_gameObjects[m_gameObjectCount]));
            physics::RigidBody & rb = *m_gameObjects[m_gameObjectCount] -> getComponent<physics::RigidBody>();
            rb.useGravity = false;
            rb.isStatic = true;
            rb.mass = 0.0f;
            rb.RecomputeInverseMass();
            m_gameObjects[m_gameObjectCount] -> addComponent(new physics::PlaneCollider(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0)));
        } 
        m_gameObjectCount++;
        {
            m_gameObjects[m_gameObjectCount] = new GameObject("Plane");
            m_gameObjects[m_gameObjectCount] -> addComponent(m_meshs[1]);
            m_gameObjects[m_gameObjectCount] -> addComponent(new MeshRenderer(m_gameObjects[m_gameObjectCount] -> getComponent<Mesh>(), m_materials));
            m_gameObjects[m_gameObjectCount] -> setPosition(glm::vec3(5, 4, 0));
            m_gameObjects[m_gameObjectCount] -> setScale(glm::vec3(1, 1, 1));
            m_gameObjects[m_gameObjectCount] -> setRotation(glm::vec3(0.0f, 0.0f, 90.0f));
            m_gameObjects[m_gameObjectCount] -> addComponent(new physics::RigidBody(m_gameObjects[m_gameObjectCount]));
            physics::RigidBody & rb = *m_gameObjects[m_gameObjectCount] -> getComponent<physics::RigidBody>();
            rb.useGravity = false;
            rb.isStatic = true;
            rb.mass = 0.0f;
            rb.RecomputeInverseMass();
            m_gameObjects[m_gameObjectCount] -> addComponent(new physics::PlaneCollider(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0)));
        } 
        m_gameObjectCount++;
        {
            m_gameObjects[m_gameObjectCount] = new GameObject("Plane");
            m_gameObjects[m_gameObjectCount] -> addComponent(m_meshs[1]);
            m_gameObjects[m_gameObjectCount] -> addComponent(new MeshRenderer(m_gameObjects[m_gameObjectCount] -> getComponent<Mesh>(), m_materials));
            m_gameObjects[m_gameObjectCount] -> setPosition(glm::vec3(-5, 4, 0));
            m_gameObjects[m_gameObjectCount] -> setScale(glm::vec3(1, 1, 1));
            m_gameObjects[m_gameObjectCount] -> setRotation(glm::vec3(0.0f, 0.0f, -90.0f));
            m_gameObjects[m_gameObjectCount] -> addComponent(new physics::RigidBody(m_gameObjects[m_gameObjectCount]));
            physics::RigidBody & rb = *m_gameObjects[m_gameObjectCount] -> getComponent<physics::RigidBody>();
            rb.useGravity = false;
            rb.isStatic = true;
            rb.mass = 0.0f;
            rb.RecomputeInverseMass();
            m_gameObjects[m_gameObjectCount] -> addComponent(new physics::PlaneCollider(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0)));
        } 
        m_gameObjectCount++;
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

    void update(double deltaTime, GLFWwindow * window, bool inputEnabled = true, bool editorMode = false, double mouseAnchorX = 0.0, double mouseAnchorY = 0.0)
    {
        PhysicEngine::getInstance() -> Step(deltaTime);



        for (size_t i = 0; i < m_gameObjectCount; i++) 
            m_gameObjects[i] -> update(deltaTime);
        m_inputProcessor.update(window, inputEnabled && m_fpsControl, mouseAnchorX, mouseAnchorY);
        float sensitivity = 0.1f;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        
        if (!inputEnabled)
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
            updateCamera(move, glm::vec3(m_inputProcessor.getMouseDeltaY() * sensitivity, m_inputProcessor.getMouseDeltaX() * sensitivity, 0.0f));
            if (editorMode)
                glfwSetCursorPos(window, mouseAnchorX, mouseAnchorY);
            else
            {
                int scrWidth, scrHeight;
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

    void renderScene() const
    {
        for (size_t i = 0; i < m_gameObjectCount; i++)
        {
            m_gameObjects[i] -> getComponent<MeshRenderer>()->run(); 
            m_gameObjects[i] -> getComponent<MeshRenderer>()->render(m_camera, m_gameObjects[i] -> transform);
        }
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

    bool isFpsControlEnabled() const
    {
        return m_fpsControl;
    }

    bool isOrbitModeEnabled() const
    {
        return m_orbitMode;
    }

    ~Scene()
    {
        clearUiRenderers();
        delete[] m_gameObjects;
        delete m_materials;
        delete m_shaders;
    }
};