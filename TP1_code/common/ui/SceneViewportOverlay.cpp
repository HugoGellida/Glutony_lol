#include "SceneViewportOverlay.hpp"

#include <common/Scene.hpp>
#include <common/gameobject/component/Mesh.hpp>
#include <common/physics/RigidBody.hpp>
#include <common/utils/Raycast.hpp>

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
    for (const std::unique_ptr<UIRenderer>& renderer : m_uiRenderers)
        renderer->shutdown();
    m_uiRenderers.clear();
}

void SceneViewportOverlay::update(double deltaTime)
{
    for (const std::unique_ptr<UIRenderer>& renderer : m_uiRenderers)
        renderer->update(deltaTime);
}

void SceneViewportOverlay::setViewportRect(const UiRect& viewportRect)
{
    m_viewportRect = viewportRect;
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
        if (gameObject->getComponent<component::Mesh>() != nullptr)
        {
            Raycast::raycastTransformedAABB(
                gameObject->transform,
                gameObject->getComponent<component::Mesh>()->getAABB(),
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