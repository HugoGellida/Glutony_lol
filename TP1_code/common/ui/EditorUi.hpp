#pragma once

#include <RmlUi/Core.h>

struct UiRect
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    bool isValid() const
    {
        return width > 0 && height > 0;
    }

    bool contains(double px, double py) const
    {
        return px >= x && py >= y && px < (x + width) && py < (y + height);
    }

    int centerX() const
    {
        return x + width / 2;
    }

    int centerY() const
    {
        return y + height / 2;
    }
};

class EditorUiController : public Rml::EventListener
{
public:
    EditorUiController() = default;
    ~EditorUiController() override = default;

    bool initialize(Rml::Context* context);
    void shutdown();

    void syncToWindow(int width, int height);
    void update();
    void render();

    UiRect getViewportRect() const;
    bool isViewportHovered(double mouseX, double mouseY) const;
    bool isDragging() const;

    void ProcessEvent(Rml::Event& event) override;

private:
    enum class DragTarget
    {
        None,
        LeftSplitter,
        RightSplitter,
        HorizontalSplitter,
    };

    void applyLayout();
    void refreshCachedRects();
    void attachListeners();
    void detachListeners();

    static Rml::String pixels(int value);
    static int clampInt(int value, int minValue, int maxValue);

    Rml::Context* m_context = nullptr;
    Rml::ElementDocument* m_document = nullptr;

    Rml::Element* m_root = nullptr;
    Rml::Element* m_leftPanel = nullptr;
    Rml::Element* m_leftSplitter = nullptr;
    Rml::Element* m_centerPanel = nullptr;
    Rml::Element* m_rightSplitter = nullptr;
    Rml::Element* m_rightPanel = nullptr;
    Rml::Element* m_viewportPanel = nullptr;
    Rml::Element* m_horizontalSplitter = nullptr;
    Rml::Element* m_bottomPanel = nullptr;

    int m_windowWidth = 1;
    int m_windowHeight = 1;

    float m_leftRatio = 0.22f;
    float m_centerRatio = 0.56f;
    float m_viewportRatio = 0.78f;

    DragTarget m_dragTarget = DragTarget::None;

    UiRect m_viewportRect;
    UiRect m_centerRect;

    static constexpr int SplitterThickness = 8;
    static constexpr int MinColumnWidth = 120;
    static constexpr int MinCenterWidth = 180;
    static constexpr int MinViewportHeight = 120;
    static constexpr int MinBottomHeight = 64;
};