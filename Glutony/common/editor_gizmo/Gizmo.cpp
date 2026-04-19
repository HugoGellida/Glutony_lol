#include "Gizmo.hpp"

#include <common/Camera.hpp>
#include <common/gameobject/GameObject.hpp>
#include <common/physics/BoxCollider.hpp>
#include <common/physics/PlaneCollider.hpp>
#include <common/physics/SphereCollider.hpp>
#include <common/utils/Raycast.hpp>

#include <GL/glew.h>

#include <array>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace
{
class LineRenderer
{
public:
    bool initialize()
    {
        if (m_program != 0)
            return true;

        static const char* kVertexShaderSource = R"GLSL(
            #version 330 core
            layout(location = 0) in vec3 inPosition;
            uniform mat4 uViewProjection;
            void main()
            {
                gl_Position = uViewProjection * vec4(inPosition, 1.0);
            }
        )GLSL";

        static const char* kFragmentShaderSource = R"GLSL(
            #version 330 core
            uniform vec4 uColor;
            out vec4 outColor;
            void main()
            {
                outColor = uColor;
            }
        )GLSL";

        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, kVertexShaderSource);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, kFragmentShaderSource);
        if (vertexShader == 0 || fragmentShader == 0)
        {
            if (vertexShader != 0)
                glDeleteShader(vertexShader);
            if (fragmentShader != 0)
                glDeleteShader(fragmentShader);
            return false;
        }

        m_program = glCreateProgram();
        glAttachShader(m_program, vertexShader);
        glAttachShader(m_program, fragmentShader);
        glLinkProgram(m_program);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        GLint linkStatus = GL_FALSE;
        glGetProgramiv(m_program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE)
        {
            glDeleteProgram(m_program);
            m_program = 0;
            return false;
        }

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        if (m_vao == 0 || m_vbo == 0)
        {
            shutdown();
            return false;
        }

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 2, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        m_viewProjectionLocation = glGetUniformLocation(m_program, "uViewProjection");
        m_colorLocation = glGetUniformLocation(m_program, "uColor");
        return true;
    }

    void shutdown()
    {
        if (m_vbo != 0)
        {
            glDeleteBuffers(1, &m_vbo);
            m_vbo = 0;
        }

        if (m_vao != 0)
        {
            glDeleteVertexArrays(1, &m_vao);
            m_vao = 0;
        }

        if (m_program != 0)
        {
            glDeleteProgram(m_program);
            m_program = 0;
        }

        m_viewProjectionLocation = -1;
        m_colorLocation = -1;
    }

    void drawSegment(const glm::mat4& viewProjection, const glm::vec3& start, const glm::vec3& end, const glm::vec4& color)
    {
        if (!initialize())
            return;

        const std::array<glm::vec3, 2> vertices = {start, end};
        glUseProgram(m_program);
        glUniformMatrix4fv(m_viewProjectionLocation, 1, GL_FALSE, &(viewProjection[0][0]));
        glUniform4f(m_colorLocation, color.r, color.g, color.b, color.a);
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINES, 0, 2);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glUseProgram(0);
    }

private:
    static GLuint compileShader(GLenum shaderType, const char* source)
    {
        GLuint shader = glCreateShader(shaderType);
        if (shader == 0)
            return 0;

        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint compileStatus = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
        if (compileStatus == GL_TRUE)
            return shader;

        glDeleteShader(shader);
        return 0;
    }

    GLuint m_program = 0;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLint m_viewProjectionLocation = -1;
    GLint m_colorLocation = -1;
};

LineRenderer& lineRenderer()
{
    static LineRenderer renderer;
    return renderer;
}

glm::vec3 safeNormalize(const glm::vec3& value, const glm::vec3& fallback)
{
    const float length = glm::length(value);
    if (length <= 1e-5f)
        return fallback;
    return value / length;
}

float gizmoScaleForCamera(const Camera& camera, const glm::vec3& pivotWorld)
{
    const float distance = glm::distance(camera.m_position, pivotWorld);
    return std::max(0.35f, distance * 0.18f);
}

glm::vec3 axisVectorForHandle(int handleId)
{
    switch (handleId)
    {
    case 0:
        return glm::vec3(1.0f, 0.0f, 0.0f);
    case 1:
        return glm::vec3(0.0f, 1.0f, 0.0f);
    case 2:
        return glm::vec3(0.0f, 0.0f, 1.0f);
    default:
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }
}

glm::vec4 colorForHandle(int handleId, bool active)
{
    glm::vec4 color(1.0f);
    switch (handleId)
    {
    case 0:
        color = glm::vec4(0.92f, 0.22f, 0.22f, 0.95f);
        break;
    case 1:
        color = glm::vec4(0.28f, 0.88f, 0.34f, 0.95f);
        break;
    case 2:
        color = glm::vec4(0.26f, 0.52f, 0.96f, 0.95f);
        break;
    default:
        break;
    }

    if (active)
        color = glm::min(color + glm::vec4(0.18f, 0.18f, 0.18f, 0.0f), glm::vec4(1.0f));
    return color;
}

glm::vec4 sphereColliderColor()
{
    return glm::vec4(0.34f, 0.86f, 0.98f, 0.95f);
}

glm::vec4 boxColliderColor()
{
    return glm::vec4(0.98f, 0.74f, 0.26f, 0.95f);
}

glm::vec4 planeColliderColor()
{
    return glm::vec4(0.56f, 0.94f, 0.50f, 0.95f);
}

void ringBasisForHandle(int handleId, glm::vec3& tangent, glm::vec3& bitangent)
{
    switch (handleId)
    {
    case 0:
        tangent = glm::vec3(0.0f, 1.0f, 0.0f);
        bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
        break;
    case 1:
        tangent = glm::vec3(0.0f, 0.0f, 1.0f);
        bitangent = glm::vec3(1.0f, 0.0f, 0.0f);
        break;
    case 2:
    default:
        tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        bitangent = glm::vec3(0.0f, 1.0f, 0.0f);
        break;
    }
}

void buildBasisFromNormal(const glm::vec3& normal, glm::vec3& tangent, glm::vec3& bitangent)
{
    const glm::vec3 safeNormal = safeNormalize(normal, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec3 reference = std::abs(safeNormal.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    tangent = safeNormalize(glm::cross(reference, safeNormal), glm::vec3(1.0f, 0.0f, 0.0f));
    bitangent = safeNormalize(glm::cross(safeNormal, tangent), glm::vec3(0.0f, 0.0f, 1.0f));
}

glm::vec3 projectOnPlane(const glm::vec3& vector, const glm::vec3& planeNormal)
{
    return vector - planeNormal * glm::dot(vector, planeNormal);
}

glm::vec3 axisInteractionPlaneNormal(const Camera& camera, const glm::vec3& pivotWorld, const glm::vec3& axis)
{
    const glm::vec3 viewVector = pivotWorld - camera.m_position;
    glm::vec3 planeNormal = glm::cross(axis, glm::cross(viewVector, axis));
    if (glm::length(planeNormal) <= 1e-5f)
    {
        const glm::vec3 fallbackAxis = std::abs(axis.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        planeNormal = glm::cross(axis, glm::cross(fallbackAxis, axis));
    }

    return safeNormalize(planeNormal, glm::vec3(0.0f, 0.0f, 1.0f));
}

bool intersectRingPlane(
    const editor_gizmo::FrameContext& context,
    const glm::vec3& pivotWorld,
    const glm::vec3& axisWorld,
    float ringRadius,
    float ringThickness,
    glm::vec3& hitPointWorld,
    float& distance)
{
    Raycast::Ray ray;
    ray.o = context.rayOrigin;
    ray.d = safeNormalize(context.rayDirection, glm::vec3(0.0f, 0.0f, -1.0f));
    if (!Raycast::raycastPlane(ray, pivotWorld, axisWorld, &hitPointWorld, &distance))
        return false;

    const glm::vec3 planarOffset = projectOnPlane(hitPointWorld - pivotWorld, axisWorld);
    const float radius = glm::length(planarOffset);
    return std::abs(radius - ringRadius) <= ringThickness;
}

void drawRing(
    const glm::mat4& viewProjection,
    const glm::vec3& pivotWorld,
    const glm::vec3& tangent,
    const glm::vec3& bitangent,
    float radius,
    const glm::vec4& color)
{
    constexpr int SegmentCount = 64;
    for (int segmentIndex = 0; segmentIndex < SegmentCount; ++segmentIndex)
    {
        const float t0 = (static_cast<float>(segmentIndex) / static_cast<float>(SegmentCount)) * glm::two_pi<float>();
        const float t1 = (static_cast<float>(segmentIndex + 1) / static_cast<float>(SegmentCount)) * glm::two_pi<float>();
        const glm::vec3 start = pivotWorld + tangent * std::cos(t0) * radius + bitangent * std::sin(t0) * radius;
        const glm::vec3 end = pivotWorld + tangent * std::cos(t1) * radius + bitangent * std::sin(t1) * radius;
        lineRenderer().drawSegment(viewProjection, start, end, color);
    }
}

void drawWireCube(const glm::mat4& viewProjection, const glm::vec3& center, float halfSize, const glm::vec4& color)
{
    const std::array<glm::vec3, 8> vertices = {
        center + glm::vec3(-halfSize, -halfSize, -halfSize),
        center + glm::vec3(halfSize, -halfSize, -halfSize),
        center + glm::vec3(halfSize, halfSize, -halfSize),
        center + glm::vec3(-halfSize, halfSize, -halfSize),
        center + glm::vec3(-halfSize, -halfSize, halfSize),
        center + glm::vec3(halfSize, -halfSize, halfSize),
        center + glm::vec3(halfSize, halfSize, halfSize),
        center + glm::vec3(-halfSize, halfSize, halfSize),
    };

    static const int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7},
    };

    for (const auto& edge : edges)
        lineRenderer().drawSegment(viewProjection, vertices[edge[0]], vertices[edge[1]], color);
}

void drawWireBox(const glm::mat4& viewProjection, const glm::vec3 vertices[8], const glm::vec4& color)
{
    static const int edges[12][2] = {
        {0, 1}, {1, 3}, {3, 2}, {2, 0},
        {4, 5}, {5, 7}, {7, 6}, {6, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7},
    };

    for (const auto& edge : edges)
        lineRenderer().drawSegment(viewProjection, vertices[edge[0]], vertices[edge[1]], color);
}

bool intersectDragPlane(const editor_gizmo::FrameContext& context, const glm::vec3& planePoint, const glm::vec3& planeNormal, glm::vec3& worldPoint)
{
    Raycast::Ray ray;
    ray.o = context.rayOrigin;
    ray.d = safeNormalize(context.rayDirection, glm::vec3(0.0f, 0.0f, -1.0f));
    return Raycast::raycastPlane(ray, planePoint, planeNormal, &worldPoint, nullptr);
}

glm::mat4 viewProjectionMatrix(const Camera& camera)
{
    return camera.projectionMatrix() * camera.inverseTransform();
}
}

namespace editor_gizmo
{
ActiveTarget ActiveTarget::none()
{
    return {};
}

ActiveTarget ActiveTarget::transform(int gameObjectId, TransformGizmoMode mode)
{
    ActiveTarget target;
    target.kind = TargetKind::Transform;
    target.gameObjectId = gameObjectId;
    target.transformMode = mode;
    return target;
}

ActiveTarget ActiveTarget::component(int gameObjectId, size_t componentIndex)
{
    ActiveTarget target;
    target.kind = TargetKind::Component;
    target.gameObjectId = gameObjectId;
    target.componentIndex = componentIndex;
    return target;
}

bool ActiveTarget::isNone() const
{
    return kind == TargetKind::None || gameObjectId <= 0;
}

void Collector::clear()
{
    m_gizmos.clear();
}

bool Collector::empty() const
{
    return m_gizmos.empty();
}

const std::vector<std::unique_ptr<Gizmo>>& Collector::gizmos() const
{
    return m_gizmos;
}

void collectBuiltInComponentGizmos(const component::Component& component, Collector& collector)
{
    const component_meta::ComponentDescriptor* descriptor = component.getComponentDescriptor();
    if (descriptor == nullptr)
        return;

    if (descriptor == &physics::SphereCollider::componentDescriptor())
    {
        collector.emplace<SphereColliderGizmo>(static_cast<const physics::SphereCollider&>(component));
        return;
    }

    if (descriptor == &physics::BoxCollider::componentDescriptor())
    {
        collector.emplace<BoxColliderGizmo>(static_cast<const physics::BoxCollider&>(component));
        return;
    }

    if (descriptor == &physics::PlaneCollider::componentDescriptor())
    {
        collector.emplace<PlaneColliderGizmo>(static_cast<const physics::PlaneCollider&>(component));
        return;
    }
}

TransformMoveGizmo::TransformMoveGizmo(GameObject& target)
    : m_target(&target)
{
}

HitResult TransformMoveGizmo::hitTest(const FrameContext& context) const
{
    HitResult bestHit;
    if (m_target == nullptr || context.camera == nullptr || context.scene == nullptr)
        return bestHit;

    const glm::vec3 pivotWorld = m_target->transform.getWorldPos(glm::vec3(0.0f, 0.0f, 0.0f));
    const float scale = gizmoScaleForCamera(*context.camera, pivotWorld);
    const float halfLength = scale;
    const float halfThickness = std::max(0.06f * scale, 0.04f);

    for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
    {
        const glm::vec3 axis = axisVectorForHandle(axisIndex);
        const glm::vec3 center = pivotWorld + axis * (halfLength * 0.5f);

        glm::vec3 halfExtents(halfThickness, halfThickness, halfThickness);
        halfExtents[axisIndex] = halfLength * 0.5f;

        const glm::mat3 orientation(1.0f);
        float distance = std::numeric_limits<float>::max();
        Raycast::Ray ray;
        ray.o = context.rayOrigin;
        ray.d = safeNormalize(context.rayDirection, glm::vec3(0.0f, 0.0f, -1.0f));
        if (!Raycast::raycastOBB(center, orientation, halfExtents, ray, &distance))
            continue;

        if (distance > 0.0f && distance < bestHit.distance)
        {
            bestHit.hit = true;
            bestHit.distance = distance;
            bestHit.handleId = axisIndex;
        }
    }

    return bestHit;
}

bool TransformMoveGizmo::beginInteraction(const FrameContext& context, const HitResult& hitResult, InteractionState& state, GameObject& target) const
{
    if (!context.interactionEnabled || !hitResult.hit || &target != m_target || context.camera == nullptr)
        return false;

    const glm::vec3 axis = axisVectorForHandle(hitResult.handleId);
    if (glm::length(axis) <= 0.0f)
        return false;

    const glm::vec3 pivotWorld = target.transform.getWorldPos(glm::vec3(0.0f, 0.0f, 0.0f));
    const glm::vec3 planeNormal = axisInteractionPlaneNormal(*context.camera, pivotWorld, axis);

    glm::vec3 hitPointWorld(0.0f);
    if (!intersectDragPlane(context, pivotWorld, planeNormal, hitPointWorld))
        return false;

    state.dragging = true;
    state.handleId = hitResult.handleId;
    state.dragAxisWorld = axis;
    state.dragPlanePointWorld = pivotWorld;
    state.dragPlaneNormalWorld = planeNormal;
    state.dragStartObjectWorldPosition = pivotWorld;
    state.dragStartHitPointWorld = hitPointWorld;
    return true;
}

bool TransformMoveGizmo::updateInteraction(const FrameContext& context, InteractionState& state, GameObject& target) const
{
    if (!state.dragging || &target != m_target)
        return false;

    glm::vec3 currentPointWorld(0.0f);
    if (!intersectDragPlane(context, state.dragPlanePointWorld, state.dragPlaneNormalWorld, currentPointWorld))
        return false;

    const float deltaOnAxis = glm::dot(currentPointWorld - state.dragStartHitPointWorld, state.dragAxisWorld);
    const glm::vec3 nextWorldPosition = state.dragStartObjectWorldPosition + state.dragAxisWorld * deltaOnAxis;

    if (glm::distance(target.transform.getWorldPos(glm::vec3(0.0f, 0.0f, 0.0f)), nextWorldPosition) <= 1e-5f)
        return false;

    target.transform.setWorldPosition(nextWorldPosition);
    return true;
}

void TransformMoveGizmo::endInteraction(InteractionState& state) const
{
    state = {};
}

void TransformMoveGizmo::render(const FrameContext& context, const InteractionState* interactionState) const
{
    if (m_target == nullptr || context.camera == nullptr || !context.viewportRect.isValid())
        return;

    const glm::vec3 pivotWorld = m_target->transform.getWorldPos(glm::vec3(0.0f, 0.0f, 0.0f));
    const float scale = gizmoScaleForCamera(*context.camera, pivotWorld);
    const glm::mat4 viewProjection = viewProjectionMatrix(*context.camera);

    for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
    {
        const glm::vec3 axis = axisVectorForHandle(axisIndex);
        const glm::vec3 start = pivotWorld;
        const glm::vec3 end = pivotWorld + axis * scale;
        const bool active = interactionState != nullptr && interactionState->dragging && interactionState->handleId == axisIndex;
        lineRenderer().drawSegment(viewProjection, start, end, colorForHandle(axisIndex, active));
    }
}

TransformRotateGizmo::TransformRotateGizmo(GameObject& target)
    : m_target(&target)
{
}

HitResult TransformRotateGizmo::hitTest(const FrameContext& context) const
{
    HitResult bestHit;
    if (m_target == nullptr || context.camera == nullptr || context.scene == nullptr)
        return bestHit;

    const glm::vec3 pivotWorld = m_target->transform.getWorldPos(glm::vec3(0.0f, 0.0f, 0.0f));
    const float scale = gizmoScaleForCamera(*context.camera, pivotWorld);
    const float ringRadius = scale * 1.12f;
    const float ringThickness = std::max(0.08f * scale, 0.06f);

    for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
    {
        const glm::vec3 axis = axisVectorForHandle(axisIndex);
        glm::vec3 hitPointWorld(0.0f);
        float distance = std::numeric_limits<float>::max();
        if (!intersectRingPlane(context, pivotWorld, axis, ringRadius, ringThickness, hitPointWorld, distance))
            continue;

        if (distance > 0.0f && distance < bestHit.distance)
        {
            bestHit.hit = true;
            bestHit.distance = distance;
            bestHit.handleId = axisIndex;
        }
    }

    return bestHit;
}

bool TransformRotateGizmo::beginInteraction(const FrameContext& context, const HitResult& hitResult, InteractionState& state, GameObject& target) const
{
    if (!context.interactionEnabled || !hitResult.hit || &target != m_target || context.camera == nullptr)
        return false;

    const glm::vec3 axis = axisVectorForHandle(hitResult.handleId);
    if (glm::length(axis) <= 0.0f)
        return false;

    const glm::vec3 pivotWorld = target.transform.getWorldPos(glm::vec3(0.0f, 0.0f, 0.0f));
    const float scale = gizmoScaleForCamera(*context.camera, pivotWorld);
    const float ringRadius = scale * 1.12f;
    const float ringThickness = std::max(0.08f * scale, 0.06f);
    glm::vec3 hitPointWorld(0.0f);
    float distance = 0.0f;
    if (!intersectRingPlane(context, pivotWorld, axis, ringRadius, ringThickness, hitPointWorld, distance))
        return false;

    state.dragging = true;
    state.handleId = hitResult.handleId;
    state.dragAxisWorld = axis;
    state.dragPlanePointWorld = pivotWorld;
    state.dragPlaneNormalWorld = axis;
    state.dragStartHitPointWorld = hitPointWorld;
    return true;
}

bool TransformRotateGizmo::updateInteraction(const FrameContext& context, InteractionState& state, GameObject& target) const
{
    if (!state.dragging || &target != m_target)
        return false;

    glm::vec3 currentPointWorld(0.0f);
    if (!intersectDragPlane(context, state.dragPlanePointWorld, state.dragPlaneNormalWorld, currentPointWorld))
        return false;

    const glm::vec3 previousVector = safeNormalize(
        projectOnPlane(state.dragStartHitPointWorld - state.dragPlanePointWorld, state.dragPlaneNormalWorld),
        glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::vec3 currentVector = safeNormalize(
        projectOnPlane(currentPointWorld - state.dragPlanePointWorld, state.dragPlaneNormalWorld),
        previousVector);

    const float dotValue = glm::clamp(glm::dot(previousVector, currentVector), -1.0f, 1.0f);
    const float signedAngle = std::atan2(
        glm::dot(glm::cross(previousVector, currentVector), state.dragPlaneNormalWorld),
        dotValue);

    if (std::abs(signedAngle) <= 1e-5f)
        return false;

    const glm::quat currentWorldOrientation = target.transform.getWorldOrientation();
    const glm::quat nextWorldOrientation = glm::normalize(
        glm::angleAxis(signedAngle, glm::normalize(state.dragPlaneNormalWorld)) * currentWorldOrientation);

    target.transform.setWorldOrientation(nextWorldOrientation);
    state.dragStartHitPointWorld = currentPointWorld;
    return true;
}

void TransformRotateGizmo::endInteraction(InteractionState& state) const
{
    state = {};
}

void TransformRotateGizmo::render(const FrameContext& context, const InteractionState* interactionState) const
{
    if (m_target == nullptr || context.camera == nullptr || !context.viewportRect.isValid())
        return;

    const glm::vec3 pivotWorld = m_target->transform.getWorldPos(glm::vec3(0.0f, 0.0f, 0.0f));
    const float scale = gizmoScaleForCamera(*context.camera, pivotWorld);
    const float ringRadius = scale * 1.12f;
    const glm::mat4 viewProjection = viewProjectionMatrix(*context.camera);

    for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
    {
        glm::vec3 tangent(1.0f, 0.0f, 0.0f);
        glm::vec3 bitangent(0.0f, 1.0f, 0.0f);
        ringBasisForHandle(axisIndex, tangent, bitangent);
        const bool active = interactionState != nullptr && interactionState->dragging && interactionState->handleId == axisIndex;
        drawRing(viewProjection, pivotWorld, tangent, bitangent, ringRadius, colorForHandle(axisIndex, active));
    }
}

TransformScaleGizmo::TransformScaleGizmo(GameObject& target)
    : m_target(&target)
{
}

HitResult TransformScaleGizmo::hitTest(const FrameContext& context) const
{
    HitResult bestHit;
    if (m_target == nullptr || context.camera == nullptr || context.scene == nullptr)
        return bestHit;

    const glm::vec3 pivotWorld = m_target->transform.getWorldPos(glm::vec3(0.0f, 0.0f, 0.0f));
    const float scale = gizmoScaleForCamera(*context.camera, pivotWorld);
    const float cubeHalfSize = std::max(0.12f * scale, 0.06f);
    const float stemLength = std::max(scale - cubeHalfSize, cubeHalfSize);
    const float stemHalfThickness = std::max(0.05f * scale, 0.035f);

    for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
    {
        const glm::vec3 axis = axisVectorForHandle(axisIndex);
        const glm::vec3 cubeCenter = pivotWorld + axis * scale;
        const glm::vec3 stemCenter = pivotWorld + axis * (stemLength * 0.5f);

        Raycast::Ray ray;
        ray.o = context.rayOrigin;
        ray.d = safeNormalize(context.rayDirection, glm::vec3(0.0f, 0.0f, -1.0f));

        float distance = std::numeric_limits<float>::max();
        glm::vec3 stemHalfExtents(stemHalfThickness, stemHalfThickness, stemHalfThickness);
        stemHalfExtents[axisIndex] = stemLength * 0.5f;

        if (Raycast::raycastOBB(stemCenter, glm::mat3(1.0f), stemHalfExtents, ray, &distance) &&
            distance > 0.0f && distance < bestHit.distance)
        {
            bestHit.hit = true;
            bestHit.distance = distance;
            bestHit.handleId = axisIndex;
        }

        distance = std::numeric_limits<float>::max();
        const glm::vec3 cubeHalfExtents(cubeHalfSize, cubeHalfSize, cubeHalfSize);
        if (Raycast::raycastOBB(cubeCenter, glm::mat3(1.0f), cubeHalfExtents, ray, &distance) &&
            distance > 0.0f && distance < bestHit.distance)
        {
            bestHit.hit = true;
            bestHit.distance = distance;
            bestHit.handleId = axisIndex;
        }
    }

    return bestHit;
}

bool TransformScaleGizmo::beginInteraction(const FrameContext& context, const HitResult& hitResult, InteractionState& state, GameObject& target) const
{
    if (!context.interactionEnabled || !hitResult.hit || &target != m_target || context.camera == nullptr)
        return false;

    const glm::vec3 axis = axisVectorForHandle(hitResult.handleId);
    if (glm::length(axis) <= 0.0f)
        return false;

    const glm::vec3 pivotWorld = target.transform.getWorldPos(glm::vec3(0.0f, 0.0f, 0.0f));
    const glm::vec3 planeNormal = axisInteractionPlaneNormal(*context.camera, pivotWorld, axis);

    glm::vec3 hitPointWorld(0.0f);
    if (!intersectDragPlane(context, pivotWorld, planeNormal, hitPointWorld))
        return false;

    state.dragging = true;
    state.handleId = hitResult.handleId;
    state.dragAxisWorld = axis;
    state.dragPlanePointWorld = pivotWorld;
    state.dragPlaneNormalWorld = planeNormal;
    state.dragStartHitPointWorld = hitPointWorld;
    state.dragStartObjectLocalScale = target.transform.getScale();
    state.dragReferenceScalar = std::max(gizmoScaleForCamera(*context.camera, pivotWorld), 0.5f);
    return true;
}

bool TransformScaleGizmo::updateInteraction(const FrameContext& context, InteractionState& state, GameObject& target) const
{
    if (!state.dragging || &target != m_target)
        return false;

    glm::vec3 currentPointWorld(0.0f);
    if (!intersectDragPlane(context, state.dragPlanePointWorld, state.dragPlaneNormalWorld, currentPointWorld))
        return false;

    const float deltaOnAxis = glm::dot(currentPointWorld - state.dragStartHitPointWorld, state.dragAxisWorld);
    glm::vec3 nextScale = state.dragStartObjectLocalScale;
    nextScale[state.handleId] = std::max(0.05f, state.dragStartObjectLocalScale[state.handleId] + deltaOnAxis / state.dragReferenceScalar);

    if (glm::distance(target.transform.getScale(), nextScale) <= 1e-5f)
        return false;

    target.transform.setScale(nextScale);
    return true;
}

void TransformScaleGizmo::endInteraction(InteractionState& state) const
{
    state = {};
}

void TransformScaleGizmo::render(const FrameContext& context, const InteractionState* interactionState) const
{
    if (m_target == nullptr || context.camera == nullptr || !context.viewportRect.isValid())
        return;

    const glm::vec3 pivotWorld = m_target->transform.getWorldPos(glm::vec3(0.0f, 0.0f, 0.0f));
    const float scale = gizmoScaleForCamera(*context.camera, pivotWorld);
    const float cubeHalfSize = std::max(0.12f * scale, 0.06f);
    const float stemLength = std::max(scale - cubeHalfSize, cubeHalfSize);
    const glm::mat4 viewProjection = viewProjectionMatrix(*context.camera);

    for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
    {
        const glm::vec3 axis = axisVectorForHandle(axisIndex);
        const glm::vec3 stemEnd = pivotWorld + axis * stemLength;
        const glm::vec3 cubeCenter = pivotWorld + axis * scale;
        const bool active = interactionState != nullptr && interactionState->dragging && interactionState->handleId == axisIndex;
        const glm::vec4 color = colorForHandle(axisIndex, active);
        lineRenderer().drawSegment(viewProjection, pivotWorld, stemEnd, color);
        drawWireCube(viewProjection, cubeCenter, cubeHalfSize, color);
    }
}

SphereColliderGizmo::SphereColliderGizmo(const physics::SphereCollider& collider)
    : m_collider(&collider)
{
}

HitResult SphereColliderGizmo::hitTest(const FrameContext& context) const
{
    (void)context;
    return {};
}

bool SphereColliderGizmo::beginInteraction(const FrameContext& context, const HitResult& hitResult, InteractionState& state, GameObject& target) const
{
    (void)context;
    (void)hitResult;
    (void)state;
    (void)target;
    return false;
}

bool SphereColliderGizmo::updateInteraction(const FrameContext& context, InteractionState& state, GameObject& target) const
{
    (void)context;
    (void)state;
    (void)target;
    return false;
}

void SphereColliderGizmo::endInteraction(InteractionState& state) const
{
    state = {};
}

void SphereColliderGizmo::render(const FrameContext& context, const InteractionState* interactionState) const
{
    (void)interactionState;
    if (m_collider == nullptr || context.camera == nullptr || !context.viewportRect.isValid())
        return;

    GameObject* owner = m_collider->getOwner();
    if (owner == nullptr)
        return;

    const glm::vec3 center = m_collider->getWorldCenter(owner->transform);
    const float radius = m_collider->getWorldRadius(owner->transform);
    if (radius <= 1e-5f)
        return;

    const glm::mat4 viewProjection = viewProjectionMatrix(*context.camera);
    const glm::vec4 color = sphereColliderColor();

    for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
    {
        glm::vec3 tangent(0.0f);
        glm::vec3 bitangent(0.0f);
        ringBasisForHandle(axisIndex, tangent, bitangent);
        drawRing(viewProjection, center, tangent, bitangent, radius, color);
    }
}

BoxColliderGizmo::BoxColliderGizmo(const physics::BoxCollider& collider)
    : m_collider(&collider)
{
}

HitResult BoxColliderGizmo::hitTest(const FrameContext& context) const
{
    (void)context;
    return {};
}

bool BoxColliderGizmo::beginInteraction(const FrameContext& context, const HitResult& hitResult, InteractionState& state, GameObject& target) const
{
    (void)context;
    (void)hitResult;
    (void)state;
    (void)target;
    return false;
}

bool BoxColliderGizmo::updateInteraction(const FrameContext& context, InteractionState& state, GameObject& target) const
{
    (void)context;
    (void)state;
    (void)target;
    return false;
}

void BoxColliderGizmo::endInteraction(InteractionState& state) const
{
    state = {};
}

void BoxColliderGizmo::render(const FrameContext& context, const InteractionState* interactionState) const
{
    (void)interactionState;
    if (m_collider == nullptr || context.camera == nullptr || !context.viewportRect.isValid())
        return;

    GameObject* owner = m_collider->getOwner();
    if (owner == nullptr)
        return;

    glm::vec3 vertices[8];
    m_collider->getVertices(owner->transform, vertices);
    drawWireBox(viewProjectionMatrix(*context.camera), vertices, boxColliderColor());
}

PlaneColliderGizmo::PlaneColliderGizmo(const physics::PlaneCollider& collider)
    : m_collider(&collider)
{
}

HitResult PlaneColliderGizmo::hitTest(const FrameContext& context) const
{
    (void)context;
    return {};
}

bool PlaneColliderGizmo::beginInteraction(const FrameContext& context, const HitResult& hitResult, InteractionState& state, GameObject& target) const
{
    (void)context;
    (void)hitResult;
    (void)state;
    (void)target;
    return false;
}

bool PlaneColliderGizmo::updateInteraction(const FrameContext& context, InteractionState& state, GameObject& target) const
{
    (void)context;
    (void)state;
    (void)target;
    return false;
}

void PlaneColliderGizmo::endInteraction(InteractionState& state) const
{
    state = {};
}

void PlaneColliderGizmo::render(const FrameContext& context, const InteractionState* interactionState) const
{
    (void)interactionState;
    if (m_collider == nullptr || context.camera == nullptr || !context.viewportRect.isValid())
        return;

    GameObject* owner = m_collider->getOwner();
    if (owner == nullptr)
        return;

    const glm::vec3 origin = m_collider->getWorldOrigin(owner->transform);
    const glm::vec3 normal = safeNormalize(m_collider->getWorldNormal(owner->transform), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 tangent(0.0f);
    glm::vec3 bitangent(0.0f);
    buildBasisFromNormal(normal, tangent, bitangent);

    const float halfSize = gizmoScaleForCamera(*context.camera, origin);
    const glm::mat4 viewProjection = viewProjectionMatrix(*context.camera);
    const glm::vec4 color = planeColliderColor();

    const glm::vec3 p0 = origin + (-tangent - bitangent) * halfSize;
    const glm::vec3 p1 = origin + (tangent - bitangent) * halfSize;
    const glm::vec3 p2 = origin + (tangent + bitangent) * halfSize;
    const glm::vec3 p3 = origin + (-tangent + bitangent) * halfSize;

    lineRenderer().drawSegment(viewProjection, p0, p1, color);
    lineRenderer().drawSegment(viewProjection, p1, p2, color);
    lineRenderer().drawSegment(viewProjection, p2, p3, color);
    lineRenderer().drawSegment(viewProjection, p3, p0, color);

    lineRenderer().drawSegment(viewProjection, origin - tangent * halfSize, origin + tangent * halfSize, color);
    lineRenderer().drawSegment(viewProjection, origin - bitangent * halfSize, origin + bitangent * halfSize, color);
    lineRenderer().drawSegment(viewProjection, origin, origin + normal * (halfSize * 0.8f), color);
}
}
