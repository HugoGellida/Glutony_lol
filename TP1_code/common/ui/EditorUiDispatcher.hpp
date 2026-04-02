#pragma once

#include <memory>

#include "EditorUiCommon.hpp"

class EditorUiModeController;
class Scene;
class SceneEditorController;
class UiBuilderController;

class EditorUiDispatcher
{
public:
    EditorUiDispatcher();
    ~EditorUiDispatcher();

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

    void ProcessEvent(Rml::Event& event);

private:
    EditorUiModeController* activeController();
    const EditorUiModeController* activeController() const;
    void setMode(editor_ui::EditorMode mode);

    Rml::Context* m_context = nullptr;
    editor_ui::EditorMode m_mode = editor_ui::EditorMode::SceneEditor;
    std::unique_ptr<SceneEditorController> m_sceneEditor;
    std::unique_ptr<UiBuilderController> m_uiBuilder;
};