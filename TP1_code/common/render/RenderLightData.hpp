#pragma once

#include <glm/vec3.hpp>

namespace render
{
enum class LightType
{
    Directional,
    Point,
};

struct LightInput
{
    LightType type = LightType::Directional;
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 direction{1.0f, 0.75f, -0.5f};
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
};

struct DirectionalLightSettings
{
    bool enabled = true;
    glm::vec3 direction{1.0f, 0.75f, -0.5f};
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
};
}