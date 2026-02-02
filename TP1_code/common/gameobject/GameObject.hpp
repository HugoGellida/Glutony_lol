#pragma once

#include "component/Component.hpp"
#include "Transform.hpp"

using namespace component;


class GameObject
{
private:
    Component ** m_component = nullptr;
    unsigned int m_componentStride = 0;
public:
    Transform transform;
    GameObject(){}
    
    void addComponent(Component * component)
    {
        Component ** new_comp_arr = new Component*[++m_componentStride];
        for (unsigned int i = 0; i < m_componentStride - 1; i++)
            new_comp_arr[i] = m_component[i];
        new_comp_arr[m_componentStride - 1] = component;
        delete[] m_component;
        this -> m_component = new_comp_arr;
    }

    template <typename T>
    T * getComponent()
    {
        for (uint i = 0; i < m_componentStride; i++)
            if (typeid(T) == typeid(*(m_component[i])))
                return (T *)m_component[i];
        throw "Not found";
    }

    void setPosition(glm::vec3 wPos)
    {
        transform.setPosition(wPos);
    }

    void setRotation(glm::vec3 euAngle)
    {
        transform.setRotation(euAngle);
    }
    void setScale(glm::vec3 const & scale)
    {
        transform.setScale(scale);
    }
    
    void update(double deltaTime)
    {
        for (unsigned int i = 0; i < m_componentStride; i++)
            m_component[i] -> run();
    }


    // TODO deep copy!
    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;

    ~GameObject()
    {
        for (unsigned int i = 0; i < m_componentStride; i++)
        {
            delete m_component[i];
        }
        delete[] m_component;
    }
};
