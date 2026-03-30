#pragma once

#include "component/Component.hpp"
#include "Transform.hpp"
#include <string>
#include <iostream>
using namespace component;


class GameObject
{
private:
    Component ** m_component = nullptr;
    unsigned int m_componentStride = 0;
    std::string m_name = "GameObject";

public:
    Transform transform;
    GameObject(){
        transform.setGameObject(this);
    }
    GameObject(std::string name) : m_name(name) {
        transform.setGameObject(this);
    }


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
            if (T * casted = dynamic_cast<T *>(m_component[i]))
                return casted;
        return nullptr;
    }

    size_t getComponentCount() const
    {
        return m_componentStride;
    }

    Component* getComponentAt(size_t index)
    {
        if (index >= m_componentStride)
            return nullptr;

        return m_component[index];
    }

    const Component* getComponentAt(size_t index) const
    {
        if (index >= m_componentStride)
            return nullptr;

        return m_component[index];
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

    void setName(std::string name)
    {
        this -> m_name = name;
    }

    const std::string& getName() const
    {
        return m_name;
    }

    void addChild(GameObject * child)
    {
        this -> transform.addChild(&child -> transform);
    }

    void setParent(GameObject * parent)
    {
        this -> transform.setParent(&parent -> transform);
    }

    void printHierarchy(int depth = 0)
    {
        std::string indent = "";
        for (int i = 0; i < depth; i++)
        {
            indent += " ";
        }
        std::cout << indent << m_name 
            << " Components: " << std::endl;
        for (size_t i = 0; i < m_componentStride; i++)
        {
            std::cout << indent << "  - " << typeid(*(m_component[i])).name() << std::endl;;;;
        }
        if (transform.getChildCount() > 0) { 
            std::cout << indent << " Childs: " << std::endl; 
            for (size_t i = 0; i < transform.getChildCount(); i++) {
                 transform.getChild(i) -> printHierarchy(depth + 1); 
                } 
        }
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
        transform.removeParent();
        transform.detachChilds();
    }
};
