#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "common/physics/AABB.hpp"
#include <vector>

class GameObject;



class Transform
{
private:
    GameObject * m_gameObject = nullptr;
    glm::vec3 m_position;
    glm::vec3 m_rotation;
    glm::quat m_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
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
        m_rotMat = glm::mat4_cast(m_orientation);
        m_transformationMatrix = translationMatrix(m_position) * m_rotMat * scaleMatrix(m_scale);
    }

    void setRotation(float const & x, float const & y, float const & z)
    {
        m_rotation.x = x;
        m_rotation.y = y;
        m_rotation.z = z;
        m_orientation = glm::normalize(glm::quat(glm::radians(m_rotation)));
        rebuildMatrix();
    }
    void setRotation(glm::vec3 const & rot)
    {
        m_rotation = rot;
        m_orientation = glm::normalize(glm::quat(glm::radians(m_rotation)));
        rebuildMatrix();
    }
    void setOrientation(glm::quat const & orientation)
    {
        m_orientation = glm::normalize(orientation);
        m_rotation = glm::degrees(glm::eulerAngles(m_orientation));
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
    const GameObject * getChild(size_t i) const;

    GameObject * getGameObject() const;

    Transform* getParent()
    {
        return m_parent;
    }

    const Transform* getParent() const
    {
        return m_parent;
    }

    glm::mat4 getModelWorld() const
    {
        return (this->m_parent != nullptr) ? m_parent->getModelWorld() * m_transformationMatrix : m_transformationMatrix;
    }

    glm::vec3 getWorldPos(glm::vec3 localPos) const
    {
        glm::vec4 res = getModelWorld() * glm::vec4(localPos.x, localPos.y, localPos.z, 1);
        return glm::vec3(res.x, res.y, res.z);
    }

    glm::vec3 getWorldNormal(glm::vec3 localNormal) const
    {
        glm::vec4 res = getModelWorld() * glm::vec4(localNormal.x, localNormal.y, localNormal.z, 0);
        glm::vec3 rN = glm::normalize(glm::vec3(res.x, res.y, res.z));
        return rN;
    }

    const glm::vec3 & getPosition() const
    {
        return m_position;
    }

    const glm::vec3 & getRotation() const
    {
        return m_rotation;
    }

    const glm::vec3 & getScale() const
    {
        return m_scale;
    }

    glm::vec3 getWorldScale() const
    {
        if (m_parent == nullptr)
            return m_scale;

        const glm::vec3 parentScale = m_parent->getWorldScale();
        return glm::vec3(
            parentScale.x * m_scale.x,
            parentScale.y * m_scale.y,
            parentScale.z * m_scale.z
        );
    }

    const glm::quat & getOrientation() const
    {
        return m_orientation;
    }

    glm::mat4 getNormalMat() const
    {
        return (this -> m_parent != nullptr) ? m_parent -> getNormalMat() * m_rotMat : m_rotMat;
    }

    physics::AABB applyToAABB(const physics::AABB& aabb) const
    {
        std::vector<glm::vec4> box = std::vector<glm::vec4>{
            glm::vec4(aabb.min.x, aabb.min.y, aabb.min.z, 1.0),
            glm::vec4(aabb.min.x, aabb.min.y, aabb.max.z, 1.0),
            glm::vec4(aabb.min.x, aabb.max.y, aabb.min.z, 1.0),
            glm::vec4(aabb.min.x, aabb.max.y, aabb.max.z, 1.0),
            glm::vec4(aabb.max.x, aabb.min.y, aabb.min.z, 1.0),
            glm::vec4(aabb.max.x, aabb.min.y, aabb.max.z, 1.0),
            glm::vec4(aabb.max.x, aabb.max.y, aabb.min.z, 1.0),
            glm::vec4(aabb.max.x, aabb.max.y, aabb.max.z, 1.0)
        };
        physics::AABB res = physics::AABB();
        res.min = glm::vec3(MAXFLOAT, MAXFLOAT, MAXFLOAT);
        res.max = glm::vec3(-MAXFLOAT, -MAXFLOAT, -MAXFLOAT);

        glm::mat4 MW = getModelWorld();
        for (size_t i = 0; i < 8; i++)
        {
            box[i] = MW * box[i];
            for (size_t j = 0; j < 3; j++)
            {
                if (box[i][j] > res.max[j])
                    res.max[j] = box[i][j];
                if (box[i][j] < res.min[j])
                    res.min[j] = box[i][j];
            }
        }
        return res;
    }
};