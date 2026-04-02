#pragma once

#include "EditorUiCommon.hpp"
#include "UIRenderer.hpp"

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
    void update(double deltaTime);
    void setViewportRect(const UiRect& viewportRect);
    void render();
    bool hasUiRenderers() const;
    GameObject* pickGameObject(Scene& scene, GLFWwindow* window, double clickX, double clickY, bool useViewportRect) const;

private:
    Rml::Context* m_uiContext = nullptr;
    UiRect m_viewportRect;
    std::vector<std::unique_ptr<UIRenderer>> m_uiRenderers;
};