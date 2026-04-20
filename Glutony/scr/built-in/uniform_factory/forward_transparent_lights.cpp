#include <common/Scene.hpp>
#include <common/render/RenderUniformFactoryPlugin.hpp>

#include <algorithm>
#include <array>

namespace transparent_light_factory
{
constexpr int kMaxLights = 8;

const std::array<const char*, kMaxLights> kLightTypeNames = {
    "_lightType0", "_lightType1", "_lightType2", "_lightType3",
    "_lightType4", "_lightType5", "_lightType6", "_lightType7"
};

const std::array<const char*, kMaxLights> kLightPosNames = {
    "_lightPos0", "_lightPos1", "_lightPos2", "_lightPos3",
    "_lightPos4", "_lightPos5", "_lightPos6", "_lightPos7"
};

const std::array<const char*, kMaxLights> kLightDirNames = {
    "_lightDir0", "_lightDir1", "_lightDir2", "_lightDir3",
    "_lightDir4", "_lightDir5", "_lightDir6", "_lightDir7"
};

const std::array<const char*, kMaxLights> kLightColorNames = {
    "_lightColor0", "_lightColor1", "_lightColor2", "_lightColor3",
    "_lightColor4", "_lightColor5", "_lightColor6", "_lightColor7"
};

const std::array<const char*, kMaxLights> kLightIntensityNames = {
    "_lightIntensity0", "_lightIntensity1", "_lightIntensity2", "_lightIntensity3",
    "_lightIntensity4", "_lightIntensity5", "_lightIntensity6", "_lightIntensity7"
};

glm::vec3 normalizeOr(const glm::vec3& value, const glm::vec3& fallback)
{
    const float length = glm::length(value);
    if (length <= 0.0001f)
        return fallback;
    return value / length;
}

void writeLightUniforms(const render::UniformFactoryWriter& writer, int lightIndex, const render::LightInput& light)
{
    if (lightIndex < 0 || lightIndex >= kMaxLights)
        return;

    writer.addIntUniform(kLightTypeNames[static_cast<size_t>(lightIndex)], light.type == render::LightType::Directional ? 0 : 1);
    writer.addVec3Uniform(kLightPosNames[static_cast<size_t>(lightIndex)], light.position);
    writer.addVec3Uniform(kLightDirNames[static_cast<size_t>(lightIndex)], normalizeOr(light.direction, glm::vec3(1.0f, 0.75f, -0.5f)));
    writer.addVec3Uniform(kLightColorNames[static_cast<size_t>(lightIndex)], light.color);
    writer.addFloatUniform(kLightIntensityNames[static_cast<size_t>(lightIndex)], light.intensity);
}

void writeDisabledLightUniforms(const render::UniformFactoryWriter& writer, int lightIndex)
{
    render::LightInput disabledLight;
    disabledLight.type = render::LightType::Directional;
    disabledLight.position = glm::vec3(0.0f);
    disabledLight.direction = glm::vec3(1.0f, 0.75f, -0.5f);
    disabledLight.color = glm::vec3(0.0f);
    disabledLight.intensity = 0.0f;
    writeLightUniforms(writer, lightIndex, disabledLight);
}

bool buildTransparentLightList(const render::UniformFactoryExecutionContext& context, const render::UniformFactoryWriter& writer)
{
    if (context.camera == nullptr)
        return false;

    writer.addVec3Uniform("_CAMPOS", context.camera->m_position);

    const std::vector<render::LightInput> lights = context.scene != nullptr
        ? context.scene->collectLightInputs()
        : std::vector<render::LightInput>();
    const int lightCount = std::min(static_cast<int>(lights.size()), kMaxLights);
    writer.addIntUniform("_lightCount", lightCount);

    for (int lightIndex = 0; lightIndex < lightCount; ++lightIndex)
        writeLightUniforms(writer, lightIndex, lights[static_cast<size_t>(lightIndex)]);

    for (int lightIndex = lightCount; lightIndex < kMaxLights; ++lightIndex)
        writeDisabledLightUniforms(writer, lightIndex);

    return true;
}

int iterationCount(const render::UniformFactoryExecutionContext&)
{
    return 1;
}
}
