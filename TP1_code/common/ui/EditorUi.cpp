#include "EditorUi.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace
{
const char* kEditorDocument = R"RML(
<rml>
<head>
    <style>
        body {
            margin: 0px;
            color: #c7d0d9;
            font-family: LatoLatin;
            font-size: 14px;
            overflow: hidden;
        }

        #root {
            position: absolute;
            left: 0px;
            top: 0px;
            width: 100%;
            height: 100%;
            overflow: hidden;
        }

        .panel {
            position: absolute;
            background-color: #171c21;
            overflow: hidden;
        }

        .splitter {
            position: absolute;
            background-color: #28313a;
            border: 0px;
            cursor: resize;
        }

        .splitter:hover {
            background-color: #4a6578;
        }

        #center_panel {
            position: absolute;
            overflow: hidden;
        }

        #viewport_panel {
            position: absolute;
            overflow: hidden;
        }

        #bottom_panel {
            position: absolute;
            background-color: #151a1f;
            overflow: hidden;
        }
    </style>
</head>
<body>
    <div id="root">
        <div id="left_panel" class="panel"></div>
        <div id="left_splitter" class="splitter"></div>
        <div id="center_panel">
            <div id="viewport_panel"></div>
            <div id="horizontal_splitter" class="splitter"></div>
            <div id="bottom_panel"></div>
        </div>
        <div id="right_splitter" class="splitter"></div>
        <div id="right_panel" class="panel"></div>
    </div>
</body>
</rml>
)RML";
}

bool EditorUiController::initialize(Rml::Context* context)
{
    if (context == nullptr)
        return false;

    m_context = context;
    m_document = m_context->LoadDocumentFromMemory(kEditorDocument, "[editor-layout]");
    if (m_document == nullptr)
        return false;

    m_root = m_document->GetElementById("root");
    m_leftPanel = m_document->GetElementById("left_panel");
    m_leftSplitter = m_document->GetElementById("left_splitter");
    m_centerPanel = m_document->GetElementById("center_panel");
    m_rightSplitter = m_document->GetElementById("right_splitter");
    m_rightPanel = m_document->GetElementById("right_panel");
    m_viewportPanel = m_document->GetElementById("viewport_panel");
    m_horizontalSplitter = m_document->GetElementById("horizontal_splitter");
    m_bottomPanel = m_document->GetElementById("bottom_panel");

    if (m_root == nullptr ||
        m_leftPanel == nullptr ||
        m_leftSplitter == nullptr ||
        m_centerPanel == nullptr ||
        m_rightSplitter == nullptr ||
        m_rightPanel == nullptr ||
        m_viewportPanel == nullptr ||
        m_horizontalSplitter == nullptr ||
        m_bottomPanel == nullptr)
    {
        shutdown();
        return false;
    }

    attachListeners();
    m_document->Show();
    applyLayout();
    return true;
}

void EditorUiController::shutdown()
{
    detachListeners();

    if (m_context != nullptr && m_document != nullptr)
        m_context->UnloadDocument(m_document);

    m_document = nullptr;
    m_context = nullptr;
    m_root = nullptr;
    m_leftPanel = nullptr;
    m_leftSplitter = nullptr;
    m_centerPanel = nullptr;
    m_rightSplitter = nullptr;
    m_rightPanel = nullptr;
    m_viewportPanel = nullptr;
    m_horizontalSplitter = nullptr;
    m_bottomPanel = nullptr;
    m_dragTarget = DragTarget::None;
    m_viewportRect = {};
    m_centerRect = {};
}

void EditorUiController::syncToWindow(int width, int height)
{
    m_windowWidth = std::max(width, 1);
    m_windowHeight = std::max(height, 1);
}

void EditorUiController::update()
{
    if (m_context == nullptr || m_document == nullptr)
        return;

    applyLayout();
    m_context->Update();
    refreshCachedRects();
}

void EditorUiController::render()
{
    if (m_context != nullptr)
        m_context->Render();
}

UiRect EditorUiController::getViewportRect() const
{
    return m_viewportRect;
}

bool EditorUiController::isViewportHovered(double mouseX, double mouseY) const
{
    return m_viewportRect.contains(mouseX, mouseY);
}

bool EditorUiController::isDragging() const
{
    return m_dragTarget != DragTarget::None;
}

void EditorUiController::ProcessEvent(Rml::Event& event)
{
    if (m_root == nullptr)
        return;

    const Rml::EventId eventId = event.GetId();
    Rml::Element* currentElement = event.GetCurrentElement();
    const Rml::String elementId = currentElement ? currentElement->GetId() : "";

    if (eventId == Rml::EventId::Mousedown)
    {
        if (elementId == "left_splitter")
        {
            m_dragTarget = DragTarget::LeftSplitter;
            event.StopPropagation();
        }
        else if (elementId == "right_splitter")
        {
            m_dragTarget = DragTarget::RightSplitter;
            event.StopPropagation();
        }
        else if (elementId == "horizontal_splitter")
        {
            m_dragTarget = DragTarget::HorizontalSplitter;
            event.StopPropagation();
        }
        return;
    }

    if (eventId == Rml::EventId::Mouseup)
    {
        m_dragTarget = DragTarget::None;
        return;
    }

    if (eventId != Rml::EventId::Mousemove || m_dragTarget == DragTarget::None)
        return;

    const float mouseX = event.GetParameter<float>("mouse_x", 0.0f);
    const float mouseY = event.GetParameter<float>("mouse_y", 0.0f);

    if (m_dragTarget == DragTarget::LeftSplitter || m_dragTarget == DragTarget::RightSplitter)
    {
        const int totalWidth = std::max(m_windowWidth, 1);
        const int minLeftWidth = MinColumnWidth;
        const int minCenterWidth = MinCenterWidth;
        const int minRightWidth = MinColumnWidth;

        int leftWidth = static_cast<int>(std::lround(totalWidth * m_leftRatio));
        int centerWidth = static_cast<int>(std::lround(totalWidth * m_centerRatio));

        if (m_dragTarget == DragTarget::LeftSplitter)
        {
            const int maxLeftWidth = totalWidth - minCenterWidth - minRightWidth - (2 * SplitterThickness);
            leftWidth = clampInt(static_cast<int>(std::lround(mouseX)), minLeftWidth, maxLeftWidth);

            const int maxCenterWidth = totalWidth - leftWidth - minRightWidth - (2 * SplitterThickness);
            centerWidth = clampInt(centerWidth, minCenterWidth, maxCenterWidth);
            m_leftRatio = static_cast<float>(leftWidth) / static_cast<float>(totalWidth);
            m_centerRatio = static_cast<float>(centerWidth) / static_cast<float>(totalWidth);
        }
        else if (m_dragTarget == DragTarget::RightSplitter)
        {
            leftWidth = clampInt(leftWidth, minLeftWidth, totalWidth - minCenterWidth - minRightWidth - (2 * SplitterThickness));
            const int maxCenterWidth = totalWidth - leftWidth - minRightWidth - (2 * SplitterThickness);
            centerWidth = clampInt(static_cast<int>(std::lround(mouseX)) - leftWidth - SplitterThickness, minCenterWidth, maxCenterWidth);
            m_centerRatio = static_cast<float>(centerWidth) / static_cast<float>(totalWidth);
        }

        applyLayout();
        event.StopPropagation();
        return;
    }

    if (m_dragTarget == DragTarget::HorizontalSplitter && m_centerRect.isValid())
    {
        const int localY = static_cast<int>(std::lround(mouseY)) - m_centerRect.y;
        const int maxViewportHeight = m_centerRect.height - MinBottomHeight - SplitterThickness;
        const int viewportHeight = clampInt(localY, MinViewportHeight, maxViewportHeight);
        m_viewportRatio = static_cast<float>(viewportHeight) / static_cast<float>(std::max(m_centerRect.height, 1));
        applyLayout();
        event.StopPropagation();
    }
}

void EditorUiController::applyLayout()
{
    if (m_root == nullptr)
        return;

    const int totalWidth = std::max(m_windowWidth, 1);
    const int totalHeight = std::max(m_windowHeight, 1);

    int leftWidth = static_cast<int>(std::lround(totalWidth * m_leftRatio));
    leftWidth = clampInt(leftWidth, MinColumnWidth, totalWidth - MinCenterWidth - MinColumnWidth - (2 * SplitterThickness));

    int centerWidth = static_cast<int>(std::lround(totalWidth * m_centerRatio));
    const int maxCenterWidth = totalWidth - leftWidth - MinColumnWidth - (2 * SplitterThickness);
    centerWidth = clampInt(centerWidth, MinCenterWidth, maxCenterWidth);

    int rightWidth = totalWidth - leftWidth - centerWidth - (2 * SplitterThickness);
    if (rightWidth < MinColumnWidth)
    {
        const int deficit = MinColumnWidth - rightWidth;
        centerWidth = std::max(MinCenterWidth, centerWidth - deficit);
        rightWidth = totalWidth - leftWidth - centerWidth - (2 * SplitterThickness);
    }

    const int centerX = leftWidth + SplitterThickness;
    const int rightSplitterX = centerX + centerWidth;
    const int rightX = rightSplitterX + SplitterThickness;

    m_centerRect = {centerX, 0, centerWidth, totalHeight};

    int viewportHeight = static_cast<int>(std::lround(static_cast<float>(totalHeight) * m_viewportRatio));
    viewportHeight = clampInt(viewportHeight, MinViewportHeight, totalHeight - MinBottomHeight - SplitterThickness);
    const int bottomHeight = totalHeight - viewportHeight - SplitterThickness;

    m_root->SetProperty("width", pixels(totalWidth));
    m_root->SetProperty("height", pixels(totalHeight));

    m_leftPanel->SetProperty("left", pixels(0));
    m_leftPanel->SetProperty("top", pixels(0));
    m_leftPanel->SetProperty("width", pixels(leftWidth));
    m_leftPanel->SetProperty("height", pixels(totalHeight));

    m_leftSplitter->SetProperty("left", pixels(leftWidth));
    m_leftSplitter->SetProperty("top", pixels(0));
    m_leftSplitter->SetProperty("width", pixels(SplitterThickness));
    m_leftSplitter->SetProperty("height", pixels(totalHeight));

    m_centerPanel->SetProperty("left", pixels(centerX));
    m_centerPanel->SetProperty("top", pixels(0));
    m_centerPanel->SetProperty("width", pixels(centerWidth));
    m_centerPanel->SetProperty("height", pixels(totalHeight));

    m_viewportPanel->SetProperty("left", pixels(0));
    m_viewportPanel->SetProperty("top", pixels(0));
    m_viewportPanel->SetProperty("width", pixels(centerWidth));
    m_viewportPanel->SetProperty("height", pixels(viewportHeight));

    m_horizontalSplitter->SetProperty("left", pixels(0));
    m_horizontalSplitter->SetProperty("top", pixels(viewportHeight));
    m_horizontalSplitter->SetProperty("width", pixels(centerWidth));
    m_horizontalSplitter->SetProperty("height", pixels(SplitterThickness));

    m_bottomPanel->SetProperty("left", pixels(0));
    m_bottomPanel->SetProperty("top", pixels(viewportHeight + SplitterThickness));
    m_bottomPanel->SetProperty("width", pixels(centerWidth));
    m_bottomPanel->SetProperty("height", pixels(bottomHeight));

    m_rightSplitter->SetProperty("left", pixels(rightSplitterX));
    m_rightSplitter->SetProperty("top", pixels(0));
    m_rightSplitter->SetProperty("width", pixels(SplitterThickness));
    m_rightSplitter->SetProperty("height", pixels(totalHeight));

    m_rightPanel->SetProperty("left", pixels(rightX));
    m_rightPanel->SetProperty("top", pixels(0));
    m_rightPanel->SetProperty("width", pixels(rightWidth));
    m_rightPanel->SetProperty("height", pixels(totalHeight));
}

void EditorUiController::refreshCachedRects()
{
    if (m_viewportPanel == nullptr || m_centerPanel == nullptr)
        return;

    m_viewportRect.x = static_cast<int>(std::lround(m_viewportPanel->GetAbsoluteLeft() + m_viewportPanel->GetClientLeft()));
    m_viewportRect.y = static_cast<int>(std::lround(m_viewportPanel->GetAbsoluteTop() + m_viewportPanel->GetClientTop()));
    m_viewportRect.width = static_cast<int>(std::lround(m_viewportPanel->GetClientWidth()));
    m_viewportRect.height = static_cast<int>(std::lround(m_viewportPanel->GetClientHeight()));

    m_centerRect.x = static_cast<int>(std::lround(m_centerPanel->GetAbsoluteLeft() + m_centerPanel->GetClientLeft()));
    m_centerRect.y = static_cast<int>(std::lround(m_centerPanel->GetAbsoluteTop() + m_centerPanel->GetClientTop()));
    m_centerRect.width = static_cast<int>(std::lround(m_centerPanel->GetClientWidth()));
    m_centerRect.height = static_cast<int>(std::lround(m_centerPanel->GetClientHeight()));
}

void EditorUiController::attachListeners()
{
    if (m_document == nullptr)
        return;

    m_document->AddEventListener(Rml::EventId::Mousemove, this);
    m_document->AddEventListener(Rml::EventId::Mouseup, this);

    if (m_leftSplitter != nullptr)
        m_leftSplitter->AddEventListener(Rml::EventId::Mousedown, this);
    if (m_rightSplitter != nullptr)
        m_rightSplitter->AddEventListener(Rml::EventId::Mousedown, this);
    if (m_horizontalSplitter != nullptr)
        m_horizontalSplitter->AddEventListener(Rml::EventId::Mousedown, this);
}

void EditorUiController::detachListeners()
{
    if (m_document != nullptr)
    {
        m_document->RemoveEventListener(Rml::EventId::Mousemove, this);
        m_document->RemoveEventListener(Rml::EventId::Mouseup, this);
    }

    if (m_leftSplitter != nullptr)
        m_leftSplitter->RemoveEventListener(Rml::EventId::Mousedown, this);
    if (m_rightSplitter != nullptr)
        m_rightSplitter->RemoveEventListener(Rml::EventId::Mousedown, this);
    if (m_horizontalSplitter != nullptr)
        m_horizontalSplitter->RemoveEventListener(Rml::EventId::Mousedown, this);
}

Rml::String EditorUiController::pixels(int value)
{
    return std::to_string(value) + "px";
}

int EditorUiController::clampInt(int value, int minValue, int maxValue)
{
    return std::max(minValue, std::min(value, maxValue));
}