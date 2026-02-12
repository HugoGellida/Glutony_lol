#pragma once

#include "Camera.hpp"
#include "gameobject/GameObject.hpp"
#include "common/shader/Material.hpp"
#include <common/shader/Shader.hpp>
#include "gameobject/component/Mesh.hpp"
#include "gameobject/component/MeshRenderer.hpp"
#include "gameobject/component/MeshNoiseDeformPerlinHeight.hpp"
#include "InputProccessor.hpp"


class Scene
{
private:
    GameObject * m_gameObjects = nullptr;
    dataStruct::Material * m_materials;
    Shader * m_shaders;
    Camera m_camera;
    inputProcessor::InputProcessor m_inputProcessor;
    bool m_fpsControl = false;
    int plane_res = 16;
    bool m_orbitMode = false;
    glm::vec3 m_orbitPos = glm::vec3(0.0f, 10.0f, -10.0f);
    float m_orbitYangle = 0.0f;
    float m_orbitSpeed = 20.0f;

    void buildPlane(int res = 16)
    {
        if (m_gameObjects != nullptr)
            delete m_gameObjects;
        m_gameObjects = new GameObject();
        m_gameObjects -> addComponent(new Plane(glm::vec3(0, 0, 0), 10.0, res));
        component::Mesh * m = m_gameObjects -> getComponent<Plane>();
        m_gameObjects -> setPosition(glm::vec3(0, 0, 0));
        m_gameObjects -> setRotation(glm::vec3(0, 0, 0));
        
        m_gameObjects -> setScale(glm::vec3(1.0, 1.0, 1.0));
        //m_gameObjects -> addComponent(new MeshNoisePerlinHeight(m));
        m_gameObjects -> addComponent(new MeshRenderer(m, (m_materials)));
    }

public:
    Scene()
    {
        using namespace component;
        // test

        m_shaders = new Shader("./vertex_shader.glsl", "./fragment_shader.glsl");

        this -> m_materials = new dataStruct::Material(m_shaders);
        m_materials->addTexture("height_map", "./img/heightmap-1024x1024.png");
        m_materials->addTexture("t0", "./img/grass.png");
        m_materials->addTexture("t1", "./img/rock.png");
        m_materials->addTexture("t2", "./img/snowrocks.png");
        
        buildPlane(plane_res);

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
    }

    void update(double deltaTime, GLFWwindow * window)
    {
        m_gameObjects -> update(deltaTime);
        m_inputProcessor.update(window);
        float sensitivity = 0.1f;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
        
        

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
            glfwSetCursorPos(window, 1024/2, 768/2);
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
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            else
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        if (m_inputProcessor.queryKey(window, GLFW_KEY_SEMICOLON))
        {
            plane_res /= 2;
            plane_res = std::max(4, plane_res);
            buildPlane(plane_res);
            std::cout << "Plane resolution set to " << plane_res << "x" << plane_res << std::endl;
        }
        if (m_inputProcessor.queryKey(window, GLFW_KEY_P))
        {
            plane_res *= 2;
            plane_res = std::min(plane_res, 2048);
            buildPlane(plane_res);
            std::cout << "Plane resolution set to " << plane_res << "x" << plane_res << std::endl;
        }

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

        if (m_inputProcessor.queryKey(window, GLFW_KEY_V))
        {
            m_gameObjects -> getComponent<MeshRenderer>()->toggleWireframe();
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
    }

    void renderScene() const
    {
        m_gameObjects -> getComponent<MeshRenderer>()->run();
        m_gameObjects -> getComponent<MeshRenderer>()->render(m_camera, m_gameObjects -> transform);
    }

    void updateCamera(glm::vec3 deltaPos, glm::vec3 deltaEuler)
    {
        glm::vec4 posRel = (Transform::rotationMatrix(glm::vec3(0.0f, m_camera.m_orientation.y, 0.0f)) * glm::vec4(deltaPos.x, deltaPos.y, deltaPos.z, 1.0f));
        m_camera.m_position+= glm::vec3(posRel.x, posRel.y, posRel.z);
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

    ~Scene()
    {
        delete m_gameObjects;
        delete m_materials;
        delete m_shaders;
    }
};