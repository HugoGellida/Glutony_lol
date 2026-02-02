#pragma once

#include "Camera.hpp"
#include "gameobject/GameObject.hpp"
#include "common/shader/Material.hpp"
#include <common/shader/Shader.hpp>
#include "gameobject/component/Mesh.hpp"
#include "gameobject/component/MeshRenderer.hpp"

class Scene
{
private:
    GameObject * m_gameObjects;
    dataStruct::Material * m_materials;
    Shader * m_shaders;
    Camera m_camera;
public:
    Scene()
    {
        using namespace component;
        // test

        m_shaders = new Shader("./vertex_shader.glsl", "./fragment_shader.glsl");

        this -> m_materials = new dataStruct::Material(m_shaders);
        



        m_gameObjects = new GameObject();
        m_gameObjects -> addComponent(new Plane(glm::vec3(0, 0, 0), 10.0));
        component::Mesh * m = m_gameObjects -> getComponent<Plane>();
        m_gameObjects -> setPosition(glm::vec3(0, 0, -1));
        m_gameObjects -> setRotation(glm::vec3(0, 0, 0));
        
        m_gameObjects -> setScale(glm::vec3(1.0, 1.0, 1.0));

        m_gameObjects -> addComponent(new MeshRenderer(m, (m_materials)));
        m_camera = Camera();
    }

    void update(double deltaTime)
    {
        m_gameObjects -> update(deltaTime);
        // TODO update cam
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