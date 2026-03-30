#pragma once

#include <functional>

#include "EditorUiCommon.hpp"

class EditorUiModeController : public Rml::EventListener
{
public:
    virtual ~EditorUiModeController() = default;

    virtual bool initialize(Rml::Context* context) = 0;
    virtual void shutdown() = 0;
    virtual void activate() = 0;
    virtual void deactivate() = 0;
    virtual void setModeChangeCallback(const std::function<void(editor_ui::EditorMode)>& callback) = 0;
    virtual void syncToWindow(int width, int height) = 0;
    virtual void setShowStylePanel(bool showStylePanel) = 0;
    virtual void update() = 0;
    virtual void render() = 0;
    virtual UiRect getViewportRect() const = 0;
    virtual bool isViewportHovered(double mouseX, double mouseY) const = 0;
    virtual bool isDragging() const = 0;
};