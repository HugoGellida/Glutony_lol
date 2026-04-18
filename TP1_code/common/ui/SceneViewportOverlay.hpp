#pragma once

#include "EditorUiCommon.hpp"
#include "UIRenderer.hpp"

#include <common/editor_gizmo/Gizmo.hpp>

#include <RmlUi/Core.h>

#include <memory>
#include <vector>

class GameObject;
class Scene;
struct GLFWwindow;

class SceneViewportOverlay
{
public:
    void setUiContext(Rml::Context* context);
    UIRenderer* pushUiRenderer(std::unique_ptr<UIRenderer> renderer);
    void clearUiRenderers();
    void setScene(Scene* scene);
    void setActiveTarget(const editor_gizmo::ActiveTarget& activeTarget);
    void setInteractionEnabled(bool enabled);
    void update(double deltaTime);
    void setViewportRect(const UiRect& viewportRect);
    bool beginPointerInteraction(GLFWwindow* window, double clickX, double clickY, bool useViewportRect);
    bool updatePointerInteraction(GLFWwindow* window, double pointerX, double pointerY, bool useViewportRect);
    void endPointerInteraction();
    bool isDraggingGizmo() const;
    void render();
    bool hasUiRenderers() const;
    GameObject* pickGameObject(Scene& scene, GLFWwindow* window, double clickX, double clickY, bool useViewportRect) const;

private:
    editor_gizmo::FrameContext buildFrameContext(GLFWwindow* window, double pointerX, double pointerY, bool useViewportRect, float deltaTime) const;
    void rebuildActiveGizmos();
    void cancelInteraction();

    Rml::Context* m_uiContext = nullptr;
    Scene* m_scene = nullptr;
    UiRect m_viewportRect;
    editor_gizmo::ActiveTarget m_activeTarget;
    editor_gizmo::Collector m_collector;
    editor_gizmo::InteractionState m_interactionState;
    bool m_interactionEnabled = false;
    float m_lastDeltaTime = 0.0f;
    bool m_gizmosDirty = true;
    std::vector<std::unique_ptr<UIRenderer>> m_uiRenderers;
};
