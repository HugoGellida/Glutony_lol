#pragma once

#include <functional>

#include "EditorUiModeController.hpp"

class SceneEditorController : public EditorUiModeController
{
public:
    bool initialize(Rml::Context* context) override;
    void shutdown() override;
    void activate() override;
    void deactivate() override;
    void setModeChangeCallback(const std::function<void(editor_ui::EditorMode)>& callback) override;
    void syncToWindow(int width, int height) override;
    void setShowStylePanel(bool showStylePanel) override;
    void update() override;
    void render() override;
    UiRect getViewportRect() const override;
    bool isViewportHovered(double mouseX, double mouseY) const override;
    bool isDragging() const override;
    void ProcessEvent(Rml::Event& event) override;

private:
    enum class DragTarget
    {
        None,
        LeftSplitter,
        RightSplitter,
        HorizontalSplitter,
    };

    void attachListeners();
    void detachListeners();
    void applyLayout();
    void refreshPresentation();
    void refreshCachedRects();

    Rml::Context* m_context = nullptr;
    Rml::ElementDocument* m_document = nullptr;
    Rml::Element* m_root = nullptr;
    Rml::Element* m_builderHeader = nullptr;
    Rml::Element* m_leftPanel = nullptr;
    Rml::Element* m_leftSplitter = nullptr;
    Rml::Element* m_centerPanel = nullptr;
    Rml::Element* m_viewportPanel = nullptr;
    Rml::Element* m_horizontalSplitter = nullptr;
    Rml::Element* m_bottomPanel = nullptr;
    Rml::Element* m_rightSplitter = nullptr;
    Rml::Element* m_rightPanel = nullptr;
    std::function<void(editor_ui::EditorMode)> m_modeChangeCallback;
    int m_windowWidth = 1;
    int m_windowHeight = 1;
    float m_leftRatio = 0.22f;
    float m_rightRatio = 0.22f;
    float m_viewportRatio = 0.78f;
    bool m_isWindowMenuOpen = false;
    DragTarget m_dragTarget = DragTarget::None;
    UiRect m_viewportRect;
    UiRect m_centerRect;
};