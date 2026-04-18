#pragma once

#include <RmlUi/Core.h>

#include <memory>

#include "EditorUiCommon.hpp"

class EditorUiDispatcher;
class GameObject;
class Scene;

namespace editor_gizmo
{
    struct ActiveTarget;
}

class EditorUiController : public Rml::EventListener
{
public:
    EditorUiController();
    ~EditorUiController() override;

    EditorUiController(const EditorUiController&) = delete;
    EditorUiController& operator=(const EditorUiController&) = delete;

    bool initialize(Rml::Context* context);
    void shutdown();

    void sync(Scene& scene);
    void syncToWindow(int width, int height);
    void setUiBuilderEnabled(bool enabled);
    bool isUiBuilderEnabled() const;
    void setUiBuilderShowStylePanel(bool showStylePanel);
    void update();
    void render();

    UiRect getViewportRect() const;
    bool isViewportHovered(double mouseX, double mouseY) const;
    bool isDragging() const;
    bool isExternalPreviewActive() const;
    editor_gizmo::ActiveTarget activeViewportGizmoTarget() const;
    void applyViewportSelection(GameObject* gameObject);
    void notifyViewportGameObjectEdited(int gameObjectId);

    void ProcessEvent(Rml::Event& event) override;

private:
    std::unique_ptr<EditorUiDispatcher> m_dispatcher;
};
