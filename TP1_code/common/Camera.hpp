#pragma once
#include <glm/glm.hpp>
#include "common/gameobject/Transform.hpp"

class Camera
{
public:
    glm::vec3 m_position = glm::vec3(0, 0, 0);
    glm::vec3  m_orientation = glm::vec3(0, 0, 0);
    /// in rad
    float m_fov = 60.0f;
    float m_aspectRatio = 16.0f / 9.0f;
    float m_nearPlane = 0.1f;
    float m_farPlane = 100.0f;

    

    glm::mat4 projectionMatrix() const
    {
        const float DEG2RAD = acos(-1.0f) / 180.0F;
        float tangent = tan((m_fov / 2.0f) * DEG2RAD);
        float top = m_nearPlane * tangent;
        float right = top * m_aspectRatio;

        const float a = m_nearPlane / right;
        const float b = m_nearPlane / top;
        const float c = -(m_farPlane + m_nearPlane) / (m_farPlane - m_nearPlane);
        const float d = -(2.0f * m_farPlane * m_nearPlane) / (m_farPlane - m_nearPlane);

        return glm::mat4(
            a,   0.0f, 0.0f, 0.0f,
            0.0f, b,   0.0f, 0.0f,
            0.0f, 0.0f, c,   -1.0f,
            0.0f, 0.0f, d,   0.0f
        );
    }

    glm::mat4 inverseTransform() const
    {
        return glm::transpose(Transform::rotationMatrix(m_orientation)) * Transform::translationMatrix(-m_position);
    }
};