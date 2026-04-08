#pragma once

#include <RmlUi/Core.h>

#include <common/ui/EditorUiCommon.hpp>

namespace UI
{
class SceneEditorLayoutManager
{
public:
    struct Elements
    {
        Rml::Element* root = nullptr;
        Rml::Element* builderHeader = nullptr;
        Rml::Element* leftPanel = nullptr;
        Rml::Element* leftSplitter = nullptr;
        Rml::Element* centerPanel = nullptr;
        Rml::Element* viewportPanel = nullptr;
        Rml::Element* horizontalSplitter = nullptr;
        Rml::Element* bottomPanel = nullptr;
        Rml::Element* bottomBrowserFilesPane = nullptr;
        Rml::Element* bottomBrowserSplitter = nullptr;
        Rml::Element* bottomBrowserTreePane = nullptr;
        Rml::Element* rightSplitter = nullptr;
        Rml::Element* rightPanel = nullptr;
    };

    void setWindowSize(int width, int height);
    void setElements(const Elements& elements);
    void setViewportSurface(Rml::Element* viewportSurface);
    void clear();

    void applyLayout();
    void refreshCachedRects();
    void dragLeftSplitter(float mouseX);
    void dragRightSplitter(float mouseX);
    void dragHorizontalSplitter(float mouseY);
    void dragBottomBrowserSplitter(float mouseX);

    const UiRect& viewportRect() const;
    const UiRect& centerRect() const;

private:
    int m_windowWidth = 1;
    int m_windowHeight = 1;
    float m_leftRatio = 0.22f;
    float m_rightRatio = 0.22f;
    float m_viewportRatio = 0.78f;
    float m_bottomBrowserTreeRatio = 0.34f;
    Elements m_elements;
    Rml::Element* m_viewportSurface = nullptr;
    UiRect m_viewportRect;
    UiRect m_centerRect;
};
}