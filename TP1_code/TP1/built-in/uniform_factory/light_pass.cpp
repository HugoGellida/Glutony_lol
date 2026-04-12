#include <common/Scene.hpp>
#include <common/render/RenderUniformFactoryPlugin.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>

#include <cmath>
#include <vector>

namespace
{
constexpr float kDirectionalShadowHalfExtent = 20.0f;
constexpr float kDirectionalShadowDepth = 60.0f;
constexpr float kDirectionalShadowDistance = 18.0f;
constexpr float kPointShadowNearPlane = 0.1f;
constexpr float kPointShadowFarPlane = 30.0f;

struct IterationSelection
{
    render::LightInput light;
    int faceIndex = 0;
};

glm::vec3 normalizeOr(const glm::vec3& value, const glm::vec3& fallback)
{
    const float length = glm::length(value);
    if (length <= 0.0001f)
        return fallback;
    return value / length;
}

glm::vec3 cameraForward(const Camera& camera)
{
    const glm::mat4 rotation = Transform::rotationMatrix(camera.m_orientation);
    return normalizeOr(glm::vec3(rotation * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)), glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec3 pointLightFaceDirection(int faceIndex)
{
    switch (faceIndex)
    {
    case 0:
        return glm::vec3(1.0f, 0.0f, 0.0f);
    case 1:
        return glm::vec3(-1.0f, 0.0f, 0.0f);
    case 2:
        return glm::vec3(0.0f, 1.0f, 0.0f);
    case 3:
        return glm::vec3(0.0f, -1.0f, 0.0f);
    case 4:
        return glm::vec3(0.0f, 0.0f, 1.0f);
    default:
        return glm::vec3(0.0f, 0.0f, -1.0f);
    }
}

glm::vec3 pointLightFaceUp(int faceIndex)
{
    switch (faceIndex)
    {
    case 0:
    case 1:
    case 4:
    case 5:
        return glm::vec3(0.0f, -1.0f, 0.0f);
    case 2:
        return glm::vec3(0.0f, 0.0f, 1.0f);
    default:
        return glm::vec3(0.0f, 0.0f, -1.0f);
    }
}

int totalIterationCount(const std::vector<render::LightInput>& lights)
{
    int total = 0;
    for (const render::LightInput& light : lights)
        total += light.type == render::LightType::Point ? 6 : 1;
    return total;
}

bool selectIteration(const render::UniformFactoryExecutionContext& context, IterationSelection& selection)
{
    if (context.scene == nullptr)
        return false;

    const std::vector<render::LightInput> lights = context.scene->collectLightInputs();
    const int expectedIterationCount = totalIterationCount(lights);
    if (context.currentIteration < 0 || context.currentIteration >= expectedIterationCount)
        return false;
    if (context.iterationCount != expectedIterationCount)
        return false;

    int iterationOffset = 0;
    for (const render::LightInput& light : lights)
    {
        const int lightIterationCount = light.type == render::LightType::Point ? 6 : 1;
        if (context.currentIteration < iterationOffset + lightIterationCount)
        {
            selection.light = light;
            selection.faceIndex = light.type == render::LightType::Point ? context.currentIteration - iterationOffset : 0;
            return true;
        }

        iterationOffset += lightIterationCount;
    }

    return false;
}

glm::mat4 buildLightProjectionView(const render::UniformFactoryExecutionContext& context, const IterationSelection& selection)
{
    if (selection.light.type == render::LightType::Point)
    {
        const glm::vec3 direction = pointLightFaceDirection(selection.faceIndex);
        const glm::vec3 up = pointLightFaceUp(selection.faceIndex);
        const glm::mat4 view = glm::lookAt(selection.light.position, selection.light.position + direction, up);
        const glm::mat4 projection = glm::perspective(90.0f, 1.0f, kPointShadowNearPlane, kPointShadowFarPlane);
        return projection * view;
    }

    if (context.camera == nullptr)
        return glm::mat4(1.0f);

    const glm::vec3 lightDirection = normalizeOr(selection.light.direction, glm::vec3(1.0f, -1.0f, 0.0f));
    const glm::vec3 forward = cameraForward(*context.camera);
    const glm::vec3 center = context.camera->m_position + forward * kDirectionalShadowDistance;
    const glm::vec3 up = std::abs(glm::dot(lightDirection, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.95f
        ? glm::vec3(0.0f, 0.0f, 1.0f)
        : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::mat4 view = glm::lookAt(center - lightDirection * (kDirectionalShadowDepth * 0.5f), center, up);
    const glm::mat4 projection = glm::ortho(
        -kDirectionalShadowHalfExtent,
        kDirectionalShadowHalfExtent,
        -kDirectionalShadowHalfExtent,
        kDirectionalShadowHalfExtent,
        0.1f,
        kDirectionalShadowDepth);
    return projection * view;
}

void writeCameraUniforms(const render::UniformFactoryExecutionContext& context, const render::UniformFactoryWriter& writer, const glm::mat4& model)
{
    const glm::mat4 view = context.camera->inverseTransform();
    const glm::mat4 projection = context.camera->projectionMatrix();

    writer.addMat4Uniform("MVP", projection * view * model);
    writer.addMat4Uniform("MVP_ORTHO", glm::transpose(glm::inverse(model)));
    writer.addMat4Uniform("MODEL", model);
}

void writeLightUniforms(const render::UniformFactoryWriter& writer, const render::LightInput& light)
{
    writer.addIntUniform("_lightType", light.type == render::LightType::Directional ? 0 : 1);
    writer.addVec3Uniform("_lightPos", light.position);
    writer.addVec3Uniform("_lightDir", light.direction);
    writer.addVec3Uniform("_lightColor", light.color);
    writer.addFloatUniform("_lightIntensity", light.intensity);
}
}

int iterationCount(const render::UniformFactoryExecutionContext& context)
{
    if (context.scene == nullptr)
        return 0;

    return totalIterationCount(context.scene->collectLightInputs());
}

bool buildUniforms(const render::UniformFactoryExecutionContext& context, const render::UniformFactoryWriter& writer)
{
    if (context.camera == nullptr || context.transform == nullptr)
        return false;

    IterationSelection selection;
    if (!selectIteration(context, selection))
        return false;

    const glm::mat4 model = context.transform->getModelWorld();
    writeCameraUniforms(context, writer, model);
    writeLightUniforms(writer, selection.light);
    return true;
}

bool buildShadowMapUniforms(const render::UniformFactoryExecutionContext& context, const render::UniformFactoryWriter& writer)
{
    if (context.camera == nullptr || context.transform == nullptr)
        return false;

    IterationSelection selection;
    if (!selectIteration(context, selection))
        return false;

    const glm::mat4 model = context.transform->getModelWorld();
    writer.addMat4Uniform("LIGHT_MVP", buildLightProjectionView(context, selection) * model);
    return true;
}

bool buildShadowOcclusionUniforms(const render::UniformFactoryExecutionContext& context, const render::UniformFactoryWriter& writer)
{
    if (context.camera == nullptr || context.transform == nullptr)
        return false;

    IterationSelection selection;
    if (!selectIteration(context, selection))
        return false;

    const glm::mat4 model = context.transform->getModelWorld();
    writeCameraUniforms(context, writer, model);
    writer.addMat4Uniform("LIGHT_MVP", buildLightProjectionView(context, selection));
    writer.addIntUniform("_lightType", selection.light.type == render::LightType::Directional ? 0 : 1);
    writer.addVec3Uniform("_lightPos", selection.light.position);
    writer.addVec3Uniform("_lightDir", selection.light.direction);
    writer.addIntUniform("_shadowFaceIndex", selection.faceIndex);
    return true;
}