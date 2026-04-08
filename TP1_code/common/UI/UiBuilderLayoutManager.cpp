#include <common/UI/UiBuilderLayoutManager.hpp>

#include <algorithm>
#include <cmath>

namespace UI
{
void UiBuilderLayoutManager::setWindowSize(int width, int height)
{
    m_windowWidth = std::max(width, 1);
    m_windowHeight = std::max(height, 1);
}

void UiBuilderLayoutManager::setElements(const Elements& elements)
{
    m_elements = elements;
}

void UiBuilderLayoutManager::setPreviewDocument(Rml::ElementDocument* previewDocument)
{
    m_previewDocument = previewDocument;
}

void UiBuilderLayoutManager::setPreviewZoom(float zoom)
{
    m_previewZoom = std::max(0.25f, std::min(zoom, 3.0f));
}

float UiBuilderLayoutManager::previewZoom() const
{
    return m_previewZoom;
}

void UiBuilderLayoutManager::setPreviewDocumentSize(int width, int height)
{
    m_previewDocumentWidth = std::max(width, editor_ui::MinPreviewDocumentWidth);
    m_previewDocumentHeight = std::max(height, editor_ui::MinPreviewDocumentHeight);
}

int UiBuilderLayoutManager::previewDocumentWidth() const
{
    return m_previewDocumentWidth;
}

int UiBuilderLayoutManager::previewDocumentHeight() const
{
    return m_previewDocumentHeight;
}

void UiBuilderLayoutManager::fitPreviewZoom()
{
    if (!m_previewCanvasRect.isValid())
    {
        setPreviewZoom(1.0f);
        return;
    }

    const float canvasWidth = static_cast<float>(std::max(m_previewCanvasRect.width - 32, 1));
    const float canvasHeight = static_cast<float>(std::max(m_previewCanvasRect.height - 32, 1));
    const float fitZoomX = canvasWidth / static_cast<float>(std::max(m_previewDocumentWidth, 1));
    const float fitZoomY = canvasHeight / static_cast<float>(std::max(m_previewDocumentHeight, 1));
    setPreviewZoom(std::min(fitZoomX, fitZoomY));
}

void UiBuilderLayoutManager::applyLayout()
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

    if (m_elements.leftTopPanel != nullptr && m_elements.leftHorizontalSplitter != nullptr && m_elements.leftBottomPanel != nullptr)
    {
        const int topHeight = editor_ui::clampInt(
            static_cast<int>(std::lround(static_cast<float>(contentHeight) * m_leftTopRatio)),
            editor_ui::MinLeftSectionHeight,
            contentHeight - editor_ui::MinLeftSectionHeight - editor_ui::SplitterThickness);
        const int bottomHeight = contentHeight - topHeight - editor_ui::SplitterThickness;

        m_elements.leftTopPanel->SetProperty("left", editor_ui::pixels(0));
        m_elements.leftTopPanel->SetProperty("top", editor_ui::pixels(0));
        m_elements.leftTopPanel->SetProperty("width", editor_ui::pixels(leftWidth));
        m_elements.leftTopPanel->SetProperty("height", editor_ui::pixels(topHeight));

        m_elements.leftHorizontalSplitter->SetProperty("left", editor_ui::pixels(0));
        m_elements.leftHorizontalSplitter->SetProperty("top", editor_ui::pixels(topHeight));
        m_elements.leftHorizontalSplitter->SetProperty("width", editor_ui::pixels(leftWidth));
        m_elements.leftHorizontalSplitter->SetProperty("height", editor_ui::pixels(editor_ui::SplitterThickness));

        m_elements.leftBottomPanel->SetProperty("left", editor_ui::pixels(0));
        m_elements.leftBottomPanel->SetProperty("top", editor_ui::pixels(topHeight + editor_ui::SplitterThickness));
        m_elements.leftBottomPanel->SetProperty("width", editor_ui::pixels(leftWidth));
        m_elements.leftBottomPanel->SetProperty("height", editor_ui::pixels(bottomHeight));
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

    if (m_elements.viewportPanel != nullptr)
    {
        m_elements.viewportPanel->SetProperty("display", "block");
        m_elements.viewportPanel->SetProperty("left", editor_ui::pixels(0));
        m_elements.viewportPanel->SetProperty("top", editor_ui::pixels(0));
        m_elements.viewportPanel->SetProperty("width", editor_ui::pixels(centerWidth));
        m_elements.viewportPanel->SetProperty("height", editor_ui::pixels(contentHeight));
    }

    if (m_elements.horizontalSplitter != nullptr)
        m_elements.horizontalSplitter->SetProperty("display", "none");
    if (m_elements.bottomPanel != nullptr)
        m_elements.bottomPanel->SetProperty("display", "none");
}

void UiBuilderLayoutManager::refreshCachedRects()
{
    if (m_elements.leftPanel != nullptr)
    {
        m_leftPanelRect.x = static_cast<int>(std::lround(m_elements.leftPanel->GetAbsoluteLeft() + m_elements.leftPanel->GetClientLeft()));
        m_leftPanelRect.y = static_cast<int>(std::lround(m_elements.leftPanel->GetAbsoluteTop() + m_elements.leftPanel->GetClientTop()));
        m_leftPanelRect.width = static_cast<int>(std::lround(m_elements.leftPanel->GetClientWidth()));
        m_leftPanelRect.height = static_cast<int>(std::lround(m_elements.leftPanel->GetClientHeight()));
    }
    else
    {
        m_leftPanelRect = {};
    }

    m_viewportRect = {};

    if (m_elements.centerPanel != nullptr)
    {
        m_centerRect.x = static_cast<int>(std::lround(m_elements.centerPanel->GetAbsoluteLeft() + m_elements.centerPanel->GetClientLeft()));
        m_centerRect.y = static_cast<int>(std::lround(m_elements.centerPanel->GetAbsoluteTop() + m_elements.centerPanel->GetClientTop()));
        m_centerRect.width = static_cast<int>(std::lround(m_elements.centerPanel->GetClientWidth()));
        m_centerRect.height = static_cast<int>(std::lround(m_elements.centerPanel->GetClientHeight()));
    }
    else
    {
        m_centerRect = {};
    }

    if (m_elements.previewCanvas != nullptr)
    {
        m_previewCanvasRect.x = static_cast<int>(std::lround(m_elements.previewCanvas->GetAbsoluteLeft() + m_elements.previewCanvas->GetClientLeft()));
        m_previewCanvasRect.y = static_cast<int>(std::lround(m_elements.previewCanvas->GetAbsoluteTop() + m_elements.previewCanvas->GetClientTop()));
        m_previewCanvasRect.width = static_cast<int>(std::lround(m_elements.previewCanvas->GetClientWidth()));
        m_previewCanvasRect.height = static_cast<int>(std::lround(m_elements.previewCanvas->GetClientHeight()));
    }
    else
    {
        m_previewCanvasRect = {};
    }

    m_previewPageRect = {};
    m_previewWindowRect = {};
    m_previewHostRect = {};
}

void UiBuilderLayoutManager::updatePreviewDocumentPlacement(Rml::ElementDocument* owningDocument)
{
    if (m_previewDocument == nullptr)
        return;

    if (!m_previewCanvasRect.isValid())
    {
        if (m_elements.previewWindow != nullptr)
            m_elements.previewWindow->SetProperty("display", "none");
        m_previewDocument->SetProperty("display", "none");
        m_previewPageRect = {};
        return;
    }

    const int maxDisplayedWidth = std::max(1, m_previewCanvasRect.width);
    const int maxDisplayedHeight = std::max(1, m_previewCanvasRect.height);
    const int displayedWidth = editor_ui::clampInt(
        std::max(1, static_cast<int>(std::lround(static_cast<float>(m_previewDocumentWidth) * m_previewZoom))),
        static_cast<int>(std::lround(editor_ui::MinPreviewDocumentWidth * m_previewZoom)),
        maxDisplayedWidth);
    const int displayedHeight = editor_ui::clampInt(
        std::max(1, static_cast<int>(std::lround(static_cast<float>(m_previewDocumentHeight) * m_previewZoom))),
        static_cast<int>(std::lround(editor_ui::MinPreviewDocumentHeight * m_previewZoom)),
        maxDisplayedHeight);

    const int pageLeft = m_previewCanvasRect.x + std::max(0, (m_previewCanvasRect.width - displayedWidth) / 2);
    const int pageTop = m_previewCanvasRect.y + std::max(0, (m_previewCanvasRect.height - displayedHeight) / 2);
    m_previewPageRect = {pageLeft, pageTop, displayedWidth, displayedHeight};

    if (m_elements.previewWindow != nullptr)
    {
        m_elements.previewWindow->SetProperty("display", "block");
        m_elements.previewWindow->SetProperty("left", editor_ui::pixels(std::max(0, (m_previewCanvasRect.width - displayedWidth) / 2)));
        m_elements.previewWindow->SetProperty("top", editor_ui::pixels(std::max(0, (m_previewCanvasRect.height - displayedHeight) / 2)));
        m_elements.previewWindow->SetProperty("width", editor_ui::pixels(displayedWidth));
        m_elements.previewWindow->SetProperty("height", editor_ui::pixels(displayedHeight));
    }

    if (owningDocument != nullptr)
        owningDocument->UpdateDocument();

    if (m_elements.previewCanvas != nullptr)
    {
        m_previewCanvasRect.x = static_cast<int>(std::lround(m_elements.previewCanvas->GetAbsoluteLeft() + m_elements.previewCanvas->GetClientLeft()));
        m_previewCanvasRect.y = static_cast<int>(std::lround(m_elements.previewCanvas->GetAbsoluteTop() + m_elements.previewCanvas->GetClientTop()));
        m_previewCanvasRect.width = static_cast<int>(std::lround(m_elements.previewCanvas->GetClientWidth()));
        m_previewCanvasRect.height = static_cast<int>(std::lround(m_elements.previewCanvas->GetClientHeight()));
    }

    if (m_elements.previewWindow != nullptr)
    {
        m_previewWindowRect.x = static_cast<int>(std::lround(m_elements.previewWindow->GetAbsoluteLeft() + m_elements.previewWindow->GetClientLeft()));
        m_previewWindowRect.y = static_cast<int>(std::lround(m_elements.previewWindow->GetAbsoluteTop() + m_elements.previewWindow->GetClientTop()));
        m_previewWindowRect.width = static_cast<int>(std::lround(m_elements.previewWindow->GetClientWidth()));
        m_previewWindowRect.height = static_cast<int>(std::lround(m_elements.previewWindow->GetClientHeight()));
    }

    if (m_elements.previewHost != nullptr)
    {
        m_previewHostRect.x = static_cast<int>(std::lround(m_elements.previewHost->GetAbsoluteLeft() + m_elements.previewHost->GetClientLeft()));
        m_previewHostRect.y = static_cast<int>(std::lround(m_elements.previewHost->GetAbsoluteTop() + m_elements.previewHost->GetClientTop()));
        m_previewHostRect.width = static_cast<int>(std::lround(m_elements.previewHost->GetClientWidth()));
        m_previewHostRect.height = static_cast<int>(std::lround(m_elements.previewHost->GetClientHeight()));
    }
    else
    {
        m_previewHostRect = m_previewPageRect;
    }

    if (!m_previewHostRect.isValid())
    {
        m_previewDocument->SetProperty("display", "none");
        return;
    }

    m_previewDocument->SetProperty("display", "block");
    m_previewDocument->SetProperty("position", "absolute");
    m_previewDocument->SetProperty("left", editor_ui::pixels(m_previewPageRect.x));
    m_previewDocument->SetProperty("top", editor_ui::pixels(m_previewPageRect.y));
    m_previewDocument->SetProperty("width", editor_ui::pixels(m_previewPageRect.width));
    m_previewDocument->SetProperty("height", editor_ui::pixels(m_previewPageRect.height));
    m_previewDocument->SetProperty("margin", "0px");
    m_previewDocument->SetProperty("z-index", "2");
    m_previewDocument->SetProperty("overflow", "hidden");
    m_previewDocument->SetProperty("background-color", "transparent");

    Rml::Element* bodyElement = nullptr;
    for (int childIndex = 0; childIndex < m_previewDocument->GetNumChildren(); ++childIndex)
    {
        Rml::Element* child = m_previewDocument->GetChild(childIndex);
        if (child != nullptr && child->GetTagName() == "body")
        {
            bodyElement = child;
            break;
        }
    }

    if (bodyElement != nullptr)
    {
        bodyElement->SetProperty("margin", "0px");
        bodyElement->SetProperty("position", "absolute");
        bodyElement->SetProperty("left", editor_ui::pixels(0));
        bodyElement->SetProperty("top", editor_ui::pixels(0));
        bodyElement->SetProperty("width", editor_ui::pixels(m_previewDocumentWidth));
        bodyElement->SetProperty("height", editor_ui::pixels(m_previewDocumentHeight));
        bodyElement->SetProperty("overflow", "hidden");
        bodyElement->SetProperty("transform-origin", "0px 0px");
        bodyElement->SetProperty("transform", "scale(" + std::to_string(m_previewZoom) + ")");
    }
    m_previewDocument->UpdateDocument();
}

UiBuilderLayoutManager::DragTarget UiBuilderLayoutManager::getPreviewResizeTargetAt(float mouseX, float mouseY) const
{
    if (!m_previewPageRect.isValid())
        return DragTarget::None;

    constexpr int ResizeMargin = 14;
    const int left = m_previewPageRect.x;
    const int top = m_previewPageRect.y;
    const int right = m_previewPageRect.x + m_previewPageRect.width;
    const int bottom = m_previewPageRect.y + m_previewPageRect.height;

    const bool insideExpandedBounds =
        mouseX >= (left - ResizeMargin) &&
        mouseX < (right + ResizeMargin) &&
        mouseY >= (top - ResizeMargin) &&
        mouseY < (bottom + ResizeMargin);

    if (!insideExpandedBounds)
        return DragTarget::None;

    const bool nearRight = mouseX >= (right - ResizeMargin) && mouseX < (right + ResizeMargin);
    const bool nearBottom = mouseY >= (bottom - ResizeMargin) && mouseY < (bottom + ResizeMargin);

    if (nearRight && nearBottom)
        return DragTarget::PreviewResizeCorner;
    if (nearRight)
        return DragTarget::PreviewResizeRight;
    if (nearBottom)
        return DragTarget::PreviewResizeBottom;

    return DragTarget::None;
}

void UiBuilderLayoutManager::updatePreviewCursor(float mouseX, float mouseY)
{
    if (m_elements.previewCanvas == nullptr)
        return;

    const char* cursor = "default";
    switch (getPreviewResizeTargetAt(mouseX, mouseY))
    {
    case DragTarget::PreviewResizeRight:
        cursor = "ew-resize";
        break;
    case DragTarget::PreviewResizeBottom:
        cursor = "ns-resize";
        break;
    case DragTarget::PreviewResizeCorner:
        cursor = "nwse-resize";
        break;
    default:
        break;
    }

    m_elements.previewCanvas->SetProperty("cursor", cursor);
    if (m_previewDocument != nullptr)
        m_previewDocument->SetProperty("cursor", cursor);
}

bool UiBuilderLayoutManager::dragColumnSplitter(DragTarget dragTarget, float mouseX)
{
    if (dragTarget != DragTarget::LeftSplitter && dragTarget != DragTarget::RightSplitter)
        return false;

    const int totalWidth = std::max(m_windowWidth, 1);
    const int minLeftWidth = editor_ui::MinColumnWidth;
    const int minCenterWidth = editor_ui::MinCenterWidth;
    const int minRightWidth = editor_ui::MinColumnWidth;

    int leftWidth = static_cast<int>(std::lround(totalWidth * m_leftRatio));
    int rightWidth = static_cast<int>(std::lround(totalWidth * m_rightRatio));
    rightWidth = editor_ui::clampInt(rightWidth, minRightWidth, totalWidth - leftWidth - minCenterWidth - (2 * editor_ui::SplitterThickness));

    if (dragTarget == DragTarget::LeftSplitter)
    {
        const int maxLeftWidth = totalWidth - minCenterWidth - rightWidth - (2 * editor_ui::SplitterThickness);
        leftWidth = editor_ui::clampInt(static_cast<int>(std::lround(mouseX)), minLeftWidth, maxLeftWidth);
        m_leftRatio = static_cast<float>(leftWidth) / static_cast<float>(totalWidth);
    }
    else
    {
        leftWidth = editor_ui::clampInt(leftWidth, minLeftWidth, totalWidth - minCenterWidth - minRightWidth - (2 * editor_ui::SplitterThickness));
        rightWidth = editor_ui::clampInt(
            totalWidth - static_cast<int>(std::lround(mouseX)) - editor_ui::SplitterThickness,
            minRightWidth,
            totalWidth - leftWidth - minCenterWidth - (2 * editor_ui::SplitterThickness));
        m_rightRatio = static_cast<float>(rightWidth) / static_cast<float>(totalWidth);
    }

    return true;
}

bool UiBuilderLayoutManager::dragLeftColumnSplitter(float mouseY)
{
    if (!m_leftPanelRect.isValid())
        return false;

    const int localY = static_cast<int>(std::lround(mouseY)) - m_leftPanelRect.y;
    const int maxTopHeight = m_leftPanelRect.height - editor_ui::MinLeftSectionHeight - editor_ui::SplitterThickness;
    const int topHeight = editor_ui::clampInt(localY, editor_ui::MinLeftSectionHeight, maxTopHeight);
    m_leftTopRatio = static_cast<float>(topHeight) / static_cast<float>(std::max(m_leftPanelRect.height, 1));
    return true;
}

bool UiBuilderLayoutManager::dragPreviewResize(DragTarget dragTarget, float mouseX, float mouseY)
{
    if ((dragTarget != DragTarget::PreviewResizeRight &&
         dragTarget != DragTarget::PreviewResizeBottom &&
         dragTarget != DragTarget::PreviewResizeCorner) ||
        !m_previewWindowRect.isValid())
    {
        return false;
    }

    int newDocumentWidth = m_previewDocumentWidth;
    int newDocumentHeight = m_previewDocumentHeight;
    const int maxDisplayedWidth = m_previewCanvasRect.isValid() ? m_previewCanvasRect.width : m_windowWidth;
    const int maxDisplayedHeight = m_previewCanvasRect.isValid() ? m_previewCanvasRect.height : m_windowHeight;

    if (dragTarget == DragTarget::PreviewResizeRight || dragTarget == DragTarget::PreviewResizeCorner)
    {
        const int displayedWidth = editor_ui::clampInt(
            static_cast<int>(std::lround(mouseX)) - m_previewWindowRect.x,
            static_cast<int>(std::lround(editor_ui::MinPreviewDocumentWidth * m_previewZoom)),
            std::max(static_cast<int>(std::lround(editor_ui::MinPreviewDocumentWidth * m_previewZoom)), maxDisplayedWidth));
        newDocumentWidth = std::max(editor_ui::MinPreviewDocumentWidth, static_cast<int>(std::lround(static_cast<float>(displayedWidth) / m_previewZoom)));
    }

    if (dragTarget == DragTarget::PreviewResizeBottom || dragTarget == DragTarget::PreviewResizeCorner)
    {
        const int displayedHeight = editor_ui::clampInt(
            static_cast<int>(std::lround(mouseY)) - m_previewWindowRect.y,
            static_cast<int>(std::lround(editor_ui::MinPreviewDocumentHeight * m_previewZoom)),
            std::max(static_cast<int>(std::lround(editor_ui::MinPreviewDocumentHeight * m_previewZoom)), maxDisplayedHeight));
        newDocumentHeight = std::max(editor_ui::MinPreviewDocumentHeight, static_cast<int>(std::lround(static_cast<float>(displayedHeight) / m_previewZoom)));
    }

    if (newDocumentWidth == m_previewDocumentWidth && newDocumentHeight == m_previewDocumentHeight)
        return false;

    m_previewDocumentWidth = newDocumentWidth;
    m_previewDocumentHeight = newDocumentHeight;
    return true;
}

const UiRect& UiBuilderLayoutManager::leftPanelRect() const
{
    return m_leftPanelRect;
}

const UiRect& UiBuilderLayoutManager::viewportRect() const
{
    return m_viewportRect;
}

const UiRect& UiBuilderLayoutManager::centerRect() const
{
    return m_centerRect;
}

const UiRect& UiBuilderLayoutManager::previewCanvasRect() const
{
    return m_previewCanvasRect;
}

const UiRect& UiBuilderLayoutManager::previewPageRect() const
{
    return m_previewPageRect;
}

const UiRect& UiBuilderLayoutManager::previewWindowRect() const
{
    return m_previewWindowRect;
}

const UiRect& UiBuilderLayoutManager::previewHostRect() const
{
    return m_previewHostRect;
}
}