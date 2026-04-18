#include "SceneViewportOverlay.hpp"

#include <common/Scene.hpp>
#include <common/gameobject/GameObject.hpp>
#include <common/gameobject/component/Mesh.hpp>
#include <common/physics/RigidBody.hpp>
#include <common/utils/Raycast.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cfloat>

void SceneViewportOverlay::setUiContext(Rml::Context* context)
{
    m_uiContext = context;
    for (const std::unique_ptr<UIRenderer>& renderer : m_uiRenderers)
        renderer->initialize(m_uiContext);
}

UIRenderer* SceneViewportOverlay::pushUiRenderer(std::unique_ptr<UIRenderer> renderer)
{
    if (!renderer)
        return nullptr;

    renderer->initialize(m_uiContext);
    m_uiRenderers.push_back(std::move(renderer));
    return m_uiRenderers.back().get();
}

void SceneViewportOverlay::clearUiRenderers()
{
    cancelInteraction();
    m_collector.clear();
    for (const std::unique_ptr<UIRenderer>& renderer : m_uiRenderers)
        renderer->shutdown();
    m_uiRenderers.clear();
}

void SceneViewportOverlay::setScene(Scene* scene)
{
    if (m_scene == scene)
        return;

    cancelInteraction();
    m_scene = scene;
    m_gizmosDirty = true;
}

void SceneViewportOverlay::setActiveTarget(const editor_gizmo::ActiveTarget& activeTarget)
{
    if (m_activeTarget == activeTarget)
        return;

    cancelInteraction();
    m_activeTarget = activeTarget;
    m_gizmosDirty = true;
}

void SceneViewportOverlay::setInteractionEnabled(bool enabled)
{
    if (m_interactionEnabled == enabled)
        return;

    m_interactionEnabled = enabled;
    if (!m_interactionEnabled)
        cancelInteraction();
}

void SceneViewportOverlay::update(double deltaTime)
{
    m_lastDeltaTime = static_cast<float>(deltaTime);

    for (const std::unique_ptr<UIRenderer>& renderer : m_uiRenderers)
        renderer->update(deltaTime);

    if (m_scene == nullptr || m_activeTarget.isNone())
    {
        if (m_gizmosDirty)
            rebuildActiveGizmos();
        return;
    }

    if (m_gizmosDirty || m_activeTarget.kind == editor_gizmo::TargetKind::Component)
        rebuildActiveGizmos();
}

void SceneViewportOverlay::setViewportRect(const UiRect& viewportRect)
{
    m_viewportRect = viewportRect;
}

bool SceneViewportOverlay::beginPointerInteraction(GLFWwindow* window, double clickX, double clickY, bool useViewportRect)
{
    if (!m_interactionEnabled || m_scene == nullptr)
        return false;

    rebuildActiveGizmos();
    if (m_collector.empty())
        return false;

    const editor_gizmo::FrameContext context = buildFrameContext(window, clickX, clickY, useViewportRect, m_lastDeltaTime);
    if (context.scene == nullptr || context.camera == nullptr)
        return false;

    editor_gizmo::HitResult bestHit;
    size_t bestGizmoIndex = 0;
    bool foundHit = false;

    const std::vector<std::unique_ptr<editor_gizmo::Gizmo>>& gizmos = m_collector.gizmos();
    for (size_t gizmoIndex = 0; gizmoIndex < gizmos.size(); ++gizmoIndex)
    {
        const editor_gizmo::HitResult hit = gizmos[gizmoIndex]->hitTest(context);
        if (!hit.hit)
            continue;

        if (!foundHit || hit.distance < bestHit.distance)
        {
            bestHit = hit;
            bestGizmoIndex = gizmoIndex;
            foundHit = true;
        }
    }

    if (!foundHit)
        return false;

    GameObject* gameObject = m_scene->getGameObjectById(m_activeTarget.gameObjectId);
    if (gameObject == nullptr)
        return false;

    cancelInteraction();
    if (!gizmos[bestGizmoIndex]->beginInteraction(context, bestHit, m_interactionState, *gameObject))
        return false;

    if (!m_interactionState.dragging)
        return false;

    m_interactionState.gizmoIndex = bestGizmoIndex;
    return true;
}

bool SceneViewportOverlay::updatePointerInteraction(GLFWwindow* window, double pointerX, double pointerY, bool useViewportRect)
{
    if (!m_interactionState.dragging || m_scene == nullptr)
        return false;

    rebuildActiveGizmos();

    if (m_interactionState.gizmoIndex >= m_collector.gizmos().size())
    {
        cancelInteraction();
        return false;
    }

    GameObject* gameObject = m_scene->getGameObjectById(m_activeTarget.gameObjectId);
    if (gameObject == nullptr)
    {
        cancelInteraction();
        return false;
    }

    const editor_gizmo::FrameContext context = buildFrameContext(window, pointerX, pointerY, useViewportRect, m_lastDeltaTime);
    return m_collector.gizmos()[m_interactionState.gizmoIndex]->updateInteraction(context, m_interactionState, *gameObject);
}

void SceneViewportOverlay::endPointerInteraction()
{
    if (!m_interactionState.dragging)
    {
        m_interactionState = {};
        return;
    }

    if (m_interactionState.gizmoIndex < m_collector.gizmos().size())
        m_collector.gizmos()[m_interactionState.gizmoIndex]->endInteraction(m_interactionState);
    else
        m_interactionState = {};
}

bool SceneViewportOverlay::isDraggingGizmo() const
{
    return m_interactionState.dragging;
}

void SceneViewportOverlay::render()
{
    if (!m_viewportRect.isValid())
    {
        for (const std::unique_ptr<UIRenderer>& renderer : m_uiRenderers)
            renderer->setVisible(false);
        return;
    }

    for (size_t index = 0; index < m_uiRenderers.size(); ++index)
    {
        UIRenderer& renderer = *m_uiRenderers[index];
        renderer.layoutFullscreen(m_viewportRect.x, m_viewportRect.y, m_viewportRect.width, m_viewportRect.height, static_cast<int>(index));
        renderer.pullToFront();
    }

    if (m_gizmosDirty)
        rebuildActiveGizmos();

    if (m_collector.empty())
        return;

    const GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    const GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    editor_gizmo::FrameContext context;
    context.scene = m_scene;
    context.camera = m_scene != nullptr ? &m_scene->getCamera() : nullptr;
    context.viewportRect = m_viewportRect;
    context.deltaTime = m_lastDeltaTime;
    context.interactionEnabled = m_interactionEnabled;

    for (const std::unique_ptr<editor_gizmo::Gizmo>& gizmo : m_collector.gizmos())
        gizmo->render(context, m_interactionState.dragging ? &m_interactionState : nullptr);

    if (depthTestEnabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    if (cullFaceEnabled)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);

    if (blendEnabled)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
}

bool SceneViewportOverlay::hasUiRenderers() const
{
    return !m_uiRenderers.empty();
}

GameObject* SceneViewportOverlay::pickGameObject(Scene& scene, GLFWwindow* window, double clickX, double clickY, bool useViewportRect) const
{
    UiRect pickRect = m_viewportRect;
    if (!useViewportRect || !pickRect.isValid())
    {
        int width = 0;
        int height = 0;
        glfwGetWindowSize(window, &width, &height);
        pickRect = {0, 0, width, height};
    }

    Raycast::Ray ray = Raycast::getRayFromClick(
        pickRect.x,
        pickRect.y,
        pickRect.width,
        pickRect.height,
        clickX,
        clickY,
        scene.getCamera());

    float nearestDistance = MAXFLOAT;
    GameObject* selectedGameObject = nullptr;

    for (size_t index = 0; index < scene.getGameObjectCount(); ++index)
    {
        GameObject* gameObject = scene.getGameObject(index);
        if (gameObject == nullptr)
            continue;

        float hitDistance = MAXFLOAT;
        component::Mesh* pickMesh = gameObject->getComponent<component::Mesh>();
        if (pickMesh == nullptr)
        {
            if (component::MeshRenderer* meshRenderer = gameObject->getComponent<component::MeshRenderer>())
                pickMesh = meshRenderer->getMesh();
        }

        if (pickMesh != nullptr)
        {
            Raycast::raycastTransformedAABB(
                gameObject->transform,
                pickMesh->getAABB(),
                ray,
                &hitDistance);
        }
        else if (gameObject->getComponent<physics::RigidBody>() != nullptr)
        {
            Raycast::raycastAABB(gameObject->getComponent<physics::RigidBody>()->getAABB(), ray, &hitDistance);
        }

        if (hitDistance < nearestDistance && hitDistance > 0.0f)
        {
            selectedGameObject = gameObject;
            nearestDistance = hitDistance;
        }
    }

    return selectedGameObject;
}

editor_gizmo::FrameContext SceneViewportOverlay::buildFrameContext(GLFWwindow* window, double pointerX, double pointerY, bool useViewportRect, float deltaTime) const
{
    editor_gizmo::FrameContext context;
    context.scene = m_scene;
    context.viewportRect = m_viewportRect;
    context.deltaTime = deltaTime;
    context.interactionEnabled = m_interactionEnabled;

    if (m_scene == nullptr)
        return context;

    context.camera = &m_scene->getCamera();

    UiRect pickRect = m_viewportRect;
    if (!useViewportRect || !pickRect.isValid())
    {
        int width = 0;
        int height = 0;
        glfwGetWindowSize(window, &width, &height);
        pickRect = {0, 0, width, height};
        context.viewportRect = pickRect;
    }

    const Raycast::Ray ray = Raycast::getRayFromClick(
        pickRect.x,
        pickRect.y,
        pickRect.width,
        pickRect.height,
        pointerX,
        pointerY,
        m_scene->getCamera());

    context.rayOrigin = ray.o;
    context.rayDirection = ray.d;
    return context;
}

void SceneViewportOverlay::rebuildActiveGizmos()
{
    m_collector.clear();
    m_gizmosDirty = false;

    if (m_scene == nullptr || m_activeTarget.isNone())
        return;

    GameObject* gameObject = m_scene->getGameObjectById(m_activeTarget.gameObjectId);
    if (gameObject == nullptr)
        return;

    if (m_activeTarget.kind == editor_gizmo::TargetKind::Transform)
    {
        if (m_activeTarget.transformMode == editor_gizmo::TransformGizmoMode::Move)
            m_collector.emplace<editor_gizmo::TransformMoveGizmo>(*gameObject);
        else if (m_activeTarget.transformMode == editor_gizmo::TransformGizmoMode::Rotate)
            m_collector.emplace<editor_gizmo::TransformRotateGizmo>(*gameObject);
        return;
    }

    if (m_activeTarget.kind != editor_gizmo::TargetKind::Component)
        return;

    const component::Component* component = gameObject->getComponentAt(m_activeTarget.componentIndex);
    if (component == nullptr || !component->supportsEditorGizmos())
        return;

    component->collectEditorGizmos(m_collector);
}

void SceneViewportOverlay::cancelInteraction()
{
    endPointerInteraction();
}
