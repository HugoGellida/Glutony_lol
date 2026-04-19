#pragma once

#include <RmlUi/Core.h>

#include <common/ui/EditorUiCommon.hpp>

namespace UI
{
class UiBuilderLayoutManager
{
public:
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

    struct Elements
    {
        Rml::Element* root = nullptr;
        Rml::Element* builderHeader = nullptr;
        Rml::Element* leftPanel = nullptr;
        Rml::Element* leftSplitter = nullptr;
        Rml::Element* centerPanel = nullptr;
        Rml::Element* rightSplitter = nullptr;
        Rml::Element* rightPanel = nullptr;
        Rml::Element* viewportPanel = nullptr;
        Rml::Element* horizontalSplitter = nullptr;
        Rml::Element* bottomPanel = nullptr;
        Rml::Element* leftTopPanel = nullptr;
        Rml::Element* leftHorizontalSplitter = nullptr;
        Rml::Element* leftBottomPanel = nullptr;
        Rml::Element* previewCanvas = nullptr;
        Rml::Element* previewWindow = nullptr;
        Rml::Element* previewHost = nullptr;
    };

    void setWindowSize(int width, int height);
    void setElements(const Elements& elements);
    void setPreviewDocument(Rml::ElementDocument* previewDocument);
    void setPreviewZoom(float zoom);
    float previewZoom() const;
    void setPreviewDocumentSize(int width, int height);
    int previewDocumentWidth() const;
    int previewDocumentHeight() const;
    void fitPreviewZoom();
    void applyLayout();
    void refreshCachedRects();
    void updatePreviewDocumentPlacement(Rml::ElementDocument* owningDocument);
    DragTarget getPreviewResizeTargetAt(float mouseX, float mouseY) const;
    void updatePreviewCursor(float mouseX, float mouseY);
    bool dragColumnSplitter(DragTarget dragTarget, float mouseX);
    bool dragLeftColumnSplitter(float mouseY);
    bool dragPreviewResize(DragTarget dragTarget, float mouseX, float mouseY);

    const UiRect& leftPanelRect() const;
    const UiRect& viewportRect() const;
    const UiRect& centerRect() const;
    const UiRect& previewCanvasRect() const;
    const UiRect& previewPageRect() const;
    const UiRect& previewWindowRect() const;
    const UiRect& previewHostRect() const;

private:
    int m_windowWidth = 1;
    int m_windowHeight = 1;
    float m_leftTopRatio = 0.62f;
    float m_leftRatio = 0.22f;
    float m_rightRatio = 0.22f;
    float m_previewZoom = 1.0f;
    int m_previewDocumentWidth = 1280;
    int m_previewDocumentHeight = 720;
    Elements m_elements;
    Rml::ElementDocument* m_previewDocument = nullptr;
    UiRect m_leftPanelRect;
    UiRect m_viewportRect;
    UiRect m_centerRect;
    UiRect m_previewCanvasRect;
    UiRect m_previewPageRect;
    UiRect m_previewWindowRect;
    UiRect m_previewHostRect;
};
}