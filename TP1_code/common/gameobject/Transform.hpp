#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class GameObject;



class Transform
{
private:
    GameObject * m_gameObject = nullptr;
    glm::vec3 m_position;
    glm::vec3 m_rotation;
    glm::vec3 m_scale = glm::vec3(1, 1, 1);
    glm::mat4 m_transformationMatrix = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                                               0.0f, 1.0f, 0.0f, 0.0f,
                                               0.0f, 0.0f, 1.0f, 0.0f,
                                               0.0f, 0.0f, 0.0f, 1.0f);
    glm::mat4 m_rotMat = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                                     0.0f, 1.0f, 0.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f, 0.0f,
                                     0.0f, 0.0f, 0.0f, 1.0f);
    Transform * m_parent = nullptr;
    Transform ** m_childs = nullptr;
    size_t m_childCount = 0;
    
    void _addChild(Transform * child)
    {
        Transform * newChilds[m_childCount + 1];
        for (size_t i = 0; i < m_childCount; i++)
        {
            newChilds[i] = m_childs[i];
        }
        newChilds[m_childCount] = child;
        delete[] m_childs;
        m_childs = new Transform*[m_childCount + 1]; 
        for (size_t i = 0; i < m_childCount + 1; i++) {
            m_childs[i] = newChilds[i];
        }
        m_childCount++;
    }
    void _setParent(Transform * transform)
    {
        this -> m_parent = transform;
    }

public:
    Transform(){}

    void setGameObject(GameObject * gameObject);
    
    static glm::mat4 rotationMatrix(glm::vec3 rot)
    {
        glm::vec3 r = glm::radians(rot);
        // NOTE: glm::mat4(a0..a15) expects column-major.
        // The values below are re-ordered so the intended row-major matrices
        // are stored correctly without relying on glUniformMatrix4fv transpose.

        // Row-major target:
        // [ cos -sin  0 0 ]
        // [ sin  cos  0 0 ]
        // [  0    0  1 0 ]
        // [  0    0  0 1 ]
        glm::mat4 rz = glm::mat4(
            cos(r.z),  sin(r.z), 0, 0,
            -sin(r.z), cos(r.z), 0, 0,
            0,         0,        1, 0,
            0,         0,        0, 1
        );

        // Row-major target:
        // [ cos 0 sin 0 ]
        // [  0  1  0  0 ]
        // [ -sin 0 cos 0]
        // [  0  0  0  1 ]
        glm::mat4 ry = glm::mat4(
            cos(r.y), 0, -sin(r.y), 0,
            0,        1,  0,        0,
            sin(r.y), 0,  cos(r.y), 0,
            0,        0,  0,        1
        );

        // Row-major target:
        // [ 1 0    0   0 ]
        // [ 0 cos -sin 0 ]
        // [ 0 sin  cos 0 ]
        // [ 0 0    0   1 ]
        glm::mat4 rx = glm::mat4(
            1, 0,        0,       0,
            0, cos(r.x), sin(r.x), 0,
            0, -sin(r.x), cos(r.x), 0,
            0, 0,        0,       1
        );
        return rz * ry * rx;
    }

    static glm::mat4 translationMatrix(glm::vec3 pos)
    {
        // Row-major target:
        // [1 0 0 tx]
        // [0 1 0 ty]
        // [0 0 1 tz]
        // [0 0 0  1]
        return glm::mat4(
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            pos.x, pos.y, pos.z, 1
        );
    }

    static glm::mat4 scaleMatrix(glm::vec3 scale)
    {
        // Row-major target:
        // [sx 0  0  0]
        // [0  sy 0  0]
        // [0  0  sz 0]
        // [0  0  0  1]
        return glm::mat4(
            scale.x, 0, 0, 0,
            0, scale.y, 0, 0,
            0, 0, scale.z, 0,
            0, 0, 0, 1
        );
    }


    void rebuildMatrix()
    {
        m_rotMat = rotationMatrix(m_rotation);
        m_transformationMatrix = translationMatrix(m_position) * rotationMatrix(m_rotation) * scaleMatrix(m_scale);
    }

    void setRotation(float const & x, float const & y, float const & z)
    {
        m_rotation.x = x;
        m_rotation.y = y;
        m_rotation.z = z;
        rebuildMatrix();
    }
    void setRotation(glm::vec3 const & rot)
    {
        m_rotation = rot;
        rebuildMatrix();
    }
    void setPosition(float const & x, float const & y, float const & z)
    {
        m_position.x = x;
        m_position.y = y;
        m_position.z = z;
        rebuildMatrix();
    }
    void setPosition(glm::vec3 const & pos)
    {
        m_position = pos;
        rebuildMatrix();
    }
    void translate(float const & x, float const & y, float const & z)
    {
        m_position.x += x;
        m_position.y += y;
        m_position.z += z;
        rebuildMatrix();
    }
    void translate(glm::vec3 const & t)
    {
        m_position += t;
        rebuildMatrix();
    }
    void setScale(glm::vec3 const & scale)
    {
        this -> m_scale = scale;
        rebuildMatrix();
    }
    void addChild(Transform * child)
    {
        child -> _setParent(this);
        Transform * newChilds[m_childCount + 1];
        for (size_t i = 0; i < m_childCount; i++)
        {
            newChilds[i] = m_childs[i];
        }
        newChilds[m_childCount] = child;
        delete[] m_childs;
        m_childs = new Transform*[m_childCount + 1]; 
        for (size_t i = 0; i < m_childCount + 1; i++) {
            m_childs[i] = newChilds[i];
        }
        m_childCount++;
    }

    void setParent(Transform * transform)
    {
        this -> m_parent = transform;
        transform -> _addChild(this);
    }

    void removeParent()
    {
        this -> m_parent = nullptr;
    }

    void detachChilds()
    {
        for (size_t i = 0; i < m_childCount; i++)
        {
            m_childs[i] -> removeParent();
        }
        m_childCount = 0;
        delete[] m_childs;
        m_childs = nullptr;
    }

    void detachChild(Transform * child)
    {
        Transform * new_childs[m_childCount - 1];
        uint write_count = 0;
        for (size_t i = 0; i < m_childCount; i++)
        {
            if (m_childs[i] != child)
            {
                new_childs[write_count] = m_childs[i];
                write_count++;
            }
        }
        delete[] m_childs;
        m_childs = new Transform*[m_childCount - 1];
        for (size_t i = 0; i < m_childCount - 1; i++)
        {
            m_childs[i] = new_childs[i];
        }
        m_childCount--;
    }

    size_t getChildCount() const { 
        return m_childCount; 
    }

    GameObject * getChild(size_t i);

    GameObject * getGameObject() const;

    glm::mat4 getModelWorld()
    {
        return (this->m_parent != nullptr) ? m_parent->getModelWorld() * m_transformationMatrix : m_transformationMatrix;
    }
    // TODO support scaling --' (fix matrix inversion).
    glm::mat4 getNormalMat()
    {
        return (this -> m_parent != nullptr) ? m_parent -> getNormalMat() * m_rotMat : m_rotMat; 
    }
};