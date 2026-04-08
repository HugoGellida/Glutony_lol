#pragma once

#include "Component.hpp"
#include "ComponentSerialization.hpp"

#include <glm/vec3.hpp>

namespace component
{
class PointLight : public Component
{
private:
    bool m_enabled = true;
    glm::vec3 m_color{1.0f, 1.0f, 1.0f};
    float m_intensity = 1.0f;

public:
    void run() override
    {
    }

    bool isEnabled() const
    {
        return m_enabled;
    }

    void setEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

    const glm::vec3& getColor() const
    {
        return m_color;
    }

    void setColor(const glm::vec3& color)
    {
        m_color = color;
    }

    float getIntensity() const
    {
        return m_intensity;
    }

    void setIntensity(float intensity)
    {
        m_intensity = intensity;
    }

    static const component_meta::ComponentDescriptor& componentDescriptor();

    const component_meta::ComponentDescriptor* getComponentDescriptor() const override
    {
        return &componentDescriptor();
    }
};
}