#include "Transform.hpp"

#include "GameObject.hpp"

void Transform::setGameObject(GameObject * gameObject)
{
    m_gameObject = gameObject;
}

GameObject * Transform::getChild(size_t i)
{
    return m_childs[i]->m_gameObject;
}

const GameObject * Transform::getChild(size_t i) const
{
    return m_childs[i]->m_gameObject;
}

GameObject * Transform::getGameObject() const
{
    return m_gameObject;
}
