#pragma once

#include <RmlUi/Core.h>

#include <string>

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
    void setUiBuilderEnabled(bool enabled);
    void setUiBuilderShowStylePanel(bool showStylePanel);
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
        LeftHorizontalSplitter,
        PreviewResizeRight,
        PreviewResizeBottom,
        PreviewResizeCorner,
        RightSplitter,
        HorizontalSplitter,
    };

    void applyLayout();
    void refreshCachedRects();
    void refreshModePresentation();
    void refreshBuilderMenuState();
    void refreshPreviewZoomLabel();
    void fitPreviewZoom();
    DragTarget getPreviewResizeTargetAt(float mouseX, float mouseY) const;
    void updatePreviewCursor(float mouseX, float mouseY);
    void reloadPreviewDocument();
    void unloadPreviewDocument();
    void updatePreviewDocumentPlacement();
    bool loadPreviewDocumentFromFile(const std::string& filePath);
    bool savePreviewDocumentToFile(const std::string& filePath) const;
    void setPreviewZoom(float zoom);
    void attachListeners();
    void detachListeners();

    static Rml::String pixels(int value);
    static int clampInt(int value, int minValue, int maxValue);

    Rml::Context* m_context = nullptr;
    Rml::ElementDocument* m_document = nullptr;
    Rml::ElementDocument* m_previewDocument = nullptr;

    Rml::Element* m_root = nullptr;
    Rml::Element* m_builderHeader = nullptr;
    Rml::Element* m_builderMenuFileButton = nullptr;
    Rml::Element* m_builderMenuFileDropdown = nullptr;
    Rml::Element* m_leftPanel = nullptr;
    Rml::Element* m_leftTopPanel = nullptr;
    Rml::Element* m_leftHorizontalSplitter = nullptr;
    Rml::Element* m_leftBottomPanel = nullptr;
    Rml::Element* m_leftSplitter = nullptr;
    Rml::Element* m_centerPanel = nullptr;
    Rml::Element* m_rightSplitter = nullptr;
    Rml::Element* m_rightPanel = nullptr;
    Rml::Element* m_viewportPanel = nullptr;
    Rml::Element* m_previewCanvas = nullptr;
    Rml::Element* m_previewWindow = nullptr;
    Rml::Element* m_previewHost = nullptr;
    Rml::Element* m_previewZoomLabel = nullptr;
    Rml::Element* m_horizontalSplitter = nullptr;
    Rml::Element* m_bottomPanel = nullptr;

    int m_windowWidth = 1;
    int m_windowHeight = 1;

    bool m_uiBuilderEnabled = false;
    bool m_uiBuilderShowStylePanel = false;
    bool m_isFileMenuOpen = false;

    std::string m_previewDocumentSource;
    std::string m_previewDocumentPath;

    float m_leftRatio = 0.22f;
    float m_leftTopRatio = 0.62f;
    float m_centerRatio = 0.56f;
    float m_viewportRatio = 0.78f;
    float m_previewZoom = 1.0f;
    int m_previewDocumentWidth = 1280;
    int m_previewDocumentHeight = 720;

    DragTarget m_dragTarget = DragTarget::None;

    UiRect m_leftPanelRect;
    UiRect m_viewportRect;
    UiRect m_centerRect;
    UiRect m_previewCanvasRect;
    UiRect m_previewPageRect;
    UiRect m_previewWindowRect;
    UiRect m_previewHostRect;

    static constexpr int SplitterThickness = 8;
    static constexpr int BuilderHeaderHeight = 36;
    static constexpr int MinColumnWidth = 120;
    static constexpr int MinCenterWidth = 180;
    static constexpr int MinLeftSectionHeight = 96;
    static constexpr int MinPreviewDocumentWidth = 220;
    static constexpr int MinPreviewDocumentHeight = 140;
    static constexpr int MinViewportHeight = 120;
    static constexpr int MinBottomHeight = 64;
};