#include <common/UI/SceneEditorLayoutManager.hpp>

#include <algorithm>
#include <cmath>

namespace
{
constexpr int MinAssetBrowserPaneWidth = 160;
constexpr int PanelHeaderHeight = 34;
}

namespace UI
{
void SceneEditorLayoutManager::setWindowSize(int width, int height)
{
    m_windowWidth = std::max(width, 1);
    m_windowHeight = std::max(height, 1);
}

void SceneEditorLayoutManager::setElements(const Elements& elements)
{
    m_elements = elements;
}

void SceneEditorLayoutManager::setViewportSurface(Rml::Element* viewportSurface)
{
    m_viewportSurface = viewportSurface;
}

void SceneEditorLayoutManager::clear()
{
    m_elements = {};
    m_viewportSurface = nullptr;
    m_viewportRect = {};
    m_centerRect = {};
}

void SceneEditorLayoutManager::applyLayout()
{
    if (m_elements.root == nullptr)
        return;

    const int totalWidth = std::max(m_windowWidth, 1);
    const int totalHeight = std::max(m_windowHeight, 1);
    const int contentTop = editor_ui::BuilderHeaderHeight;
    const int contentHeight = std::max(1, totalHeight - contentTop);

    int leftWidth = static_cast<int>(std::lround(totalWidth * m_leftRatio));
    int rightWidth = static_cast<int>(std::lround(totalWidth * m_rightRatio));

    leftWidth = editor_ui::clampInt(leftWidth, editor_ui::MinColumnWidth, totalWidth - editor_ui::MinCenterWidth - editor_ui::MinColumnWidth - (2 * editor_ui::SplitterThickness));
    rightWidth = editor_ui::clampInt(rightWidth, editor_ui::MinColumnWidth, totalWidth - leftWidth - editor_ui::MinCenterWidth - (2 * editor_ui::SplitterThickness));

    int centerWidth = totalWidth - leftWidth - rightWidth - (2 * editor_ui::SplitterThickness);
    if (centerWidth < editor_ui::MinCenterWidth)
    {
        const int deficit = editor_ui::MinCenterWidth - centerWidth;
        if (rightWidth - deficit >= editor_ui::MinColumnWidth)
            rightWidth -= deficit;
        else
            leftWidth = std::max(editor_ui::MinColumnWidth, leftWidth - (deficit - (rightWidth - editor_ui::MinColumnWidth)));

        rightWidth = editor_ui::clampInt(rightWidth, editor_ui::MinColumnWidth, totalWidth - leftWidth - editor_ui::MinCenterWidth - (2 * editor_ui::SplitterThickness));
        centerWidth = totalWidth - leftWidth - rightWidth - (2 * editor_ui::SplitterThickness);
    }

    m_leftRatio = static_cast<float>(leftWidth) / static_cast<float>(totalWidth);
    m_rightRatio = static_cast<float>(rightWidth) / static_cast<float>(totalWidth);

    const int centerX = leftWidth + editor_ui::SplitterThickness;
    const int rightSplitterX = centerX + centerWidth;
    const int rightX = rightSplitterX + editor_ui::SplitterThickness;

    m_centerRect = {centerX, contentTop, centerWidth, contentHeight};

    m_elements.root->SetProperty("width", editor_ui::pixels(totalWidth));
    m_elements.root->SetProperty("height", editor_ui::pixels(totalHeight));
    if (m_elements.builderHeader != nullptr)
    {
        m_elements.builderHeader->SetProperty("display", "block");
        m_elements.builderHeader->SetProperty("left", editor_ui::pixels(0));
        m_elements.builderHeader->SetProperty("top", editor_ui::pixels(0));
        m_elements.builderHeader->SetProperty("width", editor_ui::pixels(totalWidth));
        m_elements.builderHeader->SetProperty("height", editor_ui::pixels(editor_ui::BuilderHeaderHeight));
    }
    if (m_elements.leftPanel != nullptr)
    {
        m_elements.leftPanel->SetProperty("left", editor_ui::pixels(0));
        m_elements.leftPanel->SetProperty("top", editor_ui::pixels(contentTop));
        m_elements.leftPanel->SetProperty("width", editor_ui::pixels(leftWidth));
        m_elements.leftPanel->SetProperty("height", editor_ui::pixels(contentHeight));
    }
    if (m_elements.leftSplitter != nullptr)
    {
        m_elements.leftSplitter->SetProperty("left", editor_ui::pixels(leftWidth));
        m_elements.leftSplitter->SetProperty("top", editor_ui::pixels(contentTop));
        m_elements.leftSplitter->SetProperty("width", editor_ui::pixels(editor_ui::SplitterThickness));
        m_elements.leftSplitter->SetProperty("height", editor_ui::pixels(contentHeight));
    }
    if (m_elements.centerPanel != nullptr)
    {
        m_elements.centerPanel->SetProperty("left", editor_ui::pixels(centerX));
        m_elements.centerPanel->SetProperty("top", editor_ui::pixels(contentTop));
        m_elements.centerPanel->SetProperty("width", editor_ui::pixels(centerWidth));
        m_elements.centerPanel->SetProperty("height", editor_ui::pixels(contentHeight));
    }
    if (m_elements.rightSplitter != nullptr)
    {
        m_elements.rightSplitter->SetProperty("left", editor_ui::pixels(rightSplitterX));
        m_elements.rightSplitter->SetProperty("top", editor_ui::pixels(contentTop));
        m_elements.rightSplitter->SetProperty("width", editor_ui::pixels(editor_ui::SplitterThickness));
        m_elements.rightSplitter->SetProperty("height", editor_ui::pixels(contentHeight));
    }
    if (m_elements.rightPanel != nullptr)
    {
        m_elements.rightPanel->SetProperty("left", editor_ui::pixels(rightX));
        m_elements.rightPanel->SetProperty("top", editor_ui::pixels(contentTop));
        m_elements.rightPanel->SetProperty("width", editor_ui::pixels(rightWidth));
        m_elements.rightPanel->SetProperty("height", editor_ui::pixels(contentHeight));
    }

    const int viewportHeight = editor_ui::clampInt(
        static_cast<int>(std::lround(static_cast<float>(contentHeight) * m_viewportRatio)),
        editor_ui::MinViewportHeight,
        contentHeight - editor_ui::MinBottomHeight - editor_ui::SplitterThickness);
    const int bottomHeight = contentHeight - viewportHeight - editor_ui::SplitterThickness;

    if (m_elements.viewportPanel != nullptr)
    {
        m_elements.viewportPanel->SetProperty("display", "block");
        m_elements.viewportPanel->SetProperty("left", editor_ui::pixels(0));
        m_elements.viewportPanel->SetProperty("top", editor_ui::pixels(0));
        m_elements.viewportPanel->SetProperty("width", editor_ui::pixels(centerWidth));
        m_elements.viewportPanel->SetProperty("height", editor_ui::pixels(viewportHeight));
    }
    if (m_elements.horizontalSplitter != nullptr)
    {
        m_elements.horizontalSplitter->SetProperty("display", "block");
        m_elements.horizontalSplitter->SetProperty("left", editor_ui::pixels(0));
        m_elements.horizontalSplitter->SetProperty("top", editor_ui::pixels(viewportHeight));
        m_elements.horizontalSplitter->SetProperty("width", editor_ui::pixels(centerWidth));
        m_elements.horizontalSplitter->SetProperty("height", editor_ui::pixels(editor_ui::SplitterThickness));
    }
    if (m_elements.bottomPanel != nullptr)
    {
        m_elements.bottomPanel->SetProperty("display", "block");
        m_elements.bottomPanel->SetProperty("left", editor_ui::pixels(0));
        m_elements.bottomPanel->SetProperty("top", editor_ui::pixels(viewportHeight + editor_ui::SplitterThickness));
        m_elements.bottomPanel->SetProperty("width", editor_ui::pixels(centerWidth));
        m_elements.bottomPanel->SetProperty("height", editor_ui::pixels(bottomHeight));
    }

    if (m_elements.bottomBrowserFilesPane != nullptr && m_elements.bottomBrowserTreePane != nullptr && m_elements.bottomBrowserSplitter != nullptr)
    {
        const int totalBrowserWidth = std::max(centerWidth, 1);
        const int browserContentHeight = std::max(bottomHeight - PanelHeaderHeight, 1);
        const int treeWidth = editor_ui::clampInt(
            static_cast<int>(std::lround(static_cast<float>(totalBrowserWidth) * m_bottomBrowserTreeRatio)),
            MinAssetBrowserPaneWidth,
            totalBrowserWidth - MinAssetBrowserPaneWidth - editor_ui::SplitterThickness);
        const int filesWidth = totalBrowserWidth - treeWidth - editor_ui::SplitterThickness;

        m_elements.bottomBrowserTreePane->SetProperty("left", editor_ui::pixels(0));
        m_elements.bottomBrowserTreePane->SetProperty("top", editor_ui::pixels(0));
        m_elements.bottomBrowserTreePane->SetProperty("width", editor_ui::pixels(treeWidth));
        m_elements.bottomBrowserTreePane->SetProperty("height", editor_ui::pixels(browserContentHeight));

        m_elements.bottomBrowserSplitter->SetProperty("left", editor_ui::pixels(treeWidth));
        m_elements.bottomBrowserSplitter->SetProperty("top", editor_ui::pixels(0));
        m_elements.bottomBrowserSplitter->SetProperty("width", editor_ui::pixels(editor_ui::SplitterThickness));
        m_elements.bottomBrowserSplitter->SetProperty("height", editor_ui::pixels(browserContentHeight));

        m_elements.bottomBrowserFilesPane->SetProperty("left", editor_ui::pixels(treeWidth + editor_ui::SplitterThickness));
        m_elements.bottomBrowserFilesPane->SetProperty("top", editor_ui::pixels(0));
        m_elements.bottomBrowserFilesPane->SetProperty("width", editor_ui::pixels(filesWidth));
        m_elements.bottomBrowserFilesPane->SetProperty("height", editor_ui::pixels(browserContentHeight));
    }
}

void SceneEditorLayoutManager::refreshCachedRects()
{
    if (m_elements.viewportPanel == nullptr || m_elements.centerPanel == nullptr)
        return;

    Rml::Element* viewportTarget = m_viewportSurface != nullptr ? m_viewportSurface : m_elements.viewportPanel;
    m_viewportRect.x = static_cast<int>(std::lround(viewportTarget->GetAbsoluteLeft() + viewportTarget->GetClientLeft()));
    m_viewportRect.y = static_cast<int>(std::lround(viewportTarget->GetAbsoluteTop() + viewportTarget->GetClientTop()));
    m_viewportRect.width = static_cast<int>(std::lround(viewportTarget->GetClientWidth()));
    m_viewportRect.height = static_cast<int>(std::lround(viewportTarget->GetClientHeight()));

    m_centerRect.x = static_cast<int>(std::lround(m_elements.centerPanel->GetAbsoluteLeft() + m_elements.centerPanel->GetClientLeft()));
    m_centerRect.y = static_cast<int>(std::lround(m_elements.centerPanel->GetAbsoluteTop() + m_elements.centerPanel->GetClientTop()));
    m_centerRect.width = static_cast<int>(std::lround(m_elements.centerPanel->GetClientWidth()));
    m_centerRect.height = static_cast<int>(std::lround(m_elements.centerPanel->GetClientHeight()));
}

void SceneEditorLayoutManager::dragLeftSplitter(float mouseX)
{
    const int totalWidth = std::max(m_windowWidth, 1);
    const int minLeftWidth = editor_ui::MinColumnWidth;
    const int minCenterWidth = editor_ui::MinCenterWidth;
    const int minRightWidth = editor_ui::MinColumnWidth;
    int leftWidth = static_cast<int>(std::lround(totalWidth * m_leftRatio));
    int rightWidth = static_cast<int>(std::lround(totalWidth * m_rightRatio));
    rightWidth = editor_ui::clampInt(rightWidth, minRightWidth, totalWidth - leftWidth - minCenterWidth - (2 * editor_ui::SplitterThickness));
    const int maxLeftWidth = totalWidth - minCenterWidth - rightWidth - (2 * editor_ui::SplitterThickness);
    leftWidth = editor_ui::clampInt(static_cast<int>(std::lround(mouseX)), minLeftWidth, maxLeftWidth);
    m_leftRatio = static_cast<float>(leftWidth) / static_cast<float>(totalWidth);
}

void SceneEditorLayoutManager::dragRightSplitter(float mouseX)
{
    const int totalWidth = std::max(m_windowWidth, 1);
    const int minLeftWidth = editor_ui::MinColumnWidth;
    const int minCenterWidth = editor_ui::MinCenterWidth;
    const int minRightWidth = editor_ui::MinColumnWidth;
    int leftWidth = static_cast<int>(std::lround(totalWidth * m_leftRatio));
    int rightWidth = static_cast<int>(std::lround(totalWidth * m_rightRatio));
    leftWidth = editor_ui::clampInt(leftWidth, minLeftWidth, totalWidth - minCenterWidth - minRightWidth - (2 * editor_ui::SplitterThickness));
    rightWidth = editor_ui::clampInt(
        totalWidth - static_cast<int>(std::lround(mouseX)) - editor_ui::SplitterThickness,
        minRightWidth,
        totalWidth - leftWidth - minCenterWidth - (2 * editor_ui::SplitterThickness));
    m_rightRatio = static_cast<float>(rightWidth) / static_cast<float>(totalWidth);
}

void SceneEditorLayoutManager::dragHorizontalSplitter(float mouseY)
{
    if (!m_centerRect.isValid())
        return;

    const int localY = static_cast<int>(std::lround(mouseY)) - m_centerRect.y;
    const int maxViewportHeight = m_centerRect.height - editor_ui::MinBottomHeight - editor_ui::SplitterThickness;
    const int viewportHeight = editor_ui::clampInt(localY, editor_ui::MinViewportHeight, maxViewportHeight);
    m_viewportRatio = static_cast<float>(viewportHeight) / static_cast<float>(std::max(m_centerRect.height, 1));
}

void SceneEditorLayoutManager::dragBottomBrowserSplitter(float mouseX)
{
    if (m_elements.bottomPanel == nullptr)
        return;

    const int totalWidth = static_cast<int>(std::lround(m_elements.bottomPanel->GetClientWidth()));
    const int localX = static_cast<int>(std::lround(mouseX)) - static_cast<int>(std::lround(m_elements.bottomPanel->GetAbsoluteLeft()));
    const int maxTreeWidth = totalWidth - MinAssetBrowserPaneWidth - editor_ui::SplitterThickness;
    const int treeWidth = editor_ui::clampInt(localX, MinAssetBrowserPaneWidth, maxTreeWidth);
    m_bottomBrowserTreeRatio = static_cast<float>(treeWidth) / static_cast<float>(totalWidth);
}

const UiRect& SceneEditorLayoutManager::viewportRect() const
{
    return m_viewportRect;
}

const UiRect& SceneEditorLayoutManager::centerRect() const
{
    return m_centerRect;
}
}