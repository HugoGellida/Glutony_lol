#include "SceneEditorController.hpp"

#include "EditorUiDocuments.hpp"

#include <algorithm>
#include <cmath>

using namespace editor_ui;

namespace
{
Rml::String findAncestorElementId(Rml::Element* targetElement, const std::function<bool(const Rml::String&)>& predicate)
{
    for (Rml::Element* element = targetElement; element != nullptr; element = element->GetParentNode())
    {
        const Rml::String elementId = element->GetId();
        if (!elementId.empty() && predicate(elementId))
            return elementId;
    }
    return "";
}
}

bool SceneEditorController::initialize(Rml::Context* context)
{
    m_context = context;
    return m_context != nullptr;
}

void SceneEditorController::shutdown()
{
    deactivate();
    m_context = nullptr;
}

void SceneEditorController::activate()
{
    if (m_context == nullptr || m_document != nullptr)
        return;

    m_document = m_context->LoadDocumentFromMemory(getEditorLayoutDocument(), "[scene-editor]");
    if (m_document == nullptr)
        return;

    m_root = m_document->GetElementById("root");
    m_builderHeader = m_document->GetElementById("builder_header");
    m_leftPanel = m_document->GetElementById("left_panel");
    m_leftSplitter = m_document->GetElementById("left_splitter");
    m_centerPanel = m_document->GetElementById("center_panel");
    m_viewportPanel = m_document->GetElementById("viewport_panel");
    m_horizontalSplitter = m_document->GetElementById("horizontal_splitter");
    m_bottomPanel = m_document->GetElementById("bottom_panel");
    m_rightSplitter = m_document->GetElementById("right_splitter");
    m_rightPanel = m_document->GetElementById("right_panel");

    if (m_root == nullptr ||
        m_builderHeader == nullptr ||
        m_leftPanel == nullptr ||
        m_leftSplitter == nullptr ||
        m_centerPanel == nullptr ||
        m_viewportPanel == nullptr ||
        m_horizontalSplitter == nullptr ||
        m_bottomPanel == nullptr ||
        m_rightSplitter == nullptr ||
        m_rightPanel == nullptr)
    {
        deactivate();
        return;
    }

    const Rml::Vector2i dimensions = m_context->GetDimensions();
    m_windowWidth = std::max(dimensions.x, 1);
    m_windowHeight = std::max(dimensions.y, 1);

    attachListeners();
    refreshPresentation();
    m_document->Show();
    applyLayout();
    refreshCachedRects();
}

void SceneEditorController::deactivate()
{
    detachListeners();

    if (m_context != nullptr && m_document != nullptr)
        m_context->UnloadDocument(m_document);

    m_document = nullptr;
    m_root = nullptr;
    m_builderHeader = nullptr;
    m_leftPanel = nullptr;
    m_leftSplitter = nullptr;
    m_centerPanel = nullptr;
    m_viewportPanel = nullptr;
    m_horizontalSplitter = nullptr;
    m_bottomPanel = nullptr;
    m_rightSplitter = nullptr;
    m_rightPanel = nullptr;
    m_isWindowMenuOpen = false;
    m_dragTarget = DragTarget::None;
    m_viewportRect = {};
    m_centerRect = {};
}

void SceneEditorController::setModeChangeCallback(const std::function<void(EditorMode)>& callback)
{
    m_modeChangeCallback = callback;
}

void SceneEditorController::syncToWindow(int width, int height)
{
    m_windowWidth = std::max(width, 1);
    m_windowHeight = std::max(height, 1);
}

void SceneEditorController::setShowStylePanel(bool showStylePanel)
{
    (void)showStylePanel;
}

void SceneEditorController::update()
{
    if (m_context == nullptr || m_document == nullptr)
        return;

    applyLayout();
    m_context->Update();
    refreshCachedRects();
}

void SceneEditorController::render()
{
    if (m_context != nullptr)
        m_context->Render();
}

UiRect SceneEditorController::getViewportRect() const
{
    return m_viewportRect;
}

bool SceneEditorController::isViewportHovered(double mouseX, double mouseY) const
{
    return m_viewportRect.contains(mouseX, mouseY);
}

bool SceneEditorController::isDragging() const
{
    return m_dragTarget != DragTarget::None;
}

void SceneEditorController::ProcessEvent(Rml::Event& event)
{
    if (m_root == nullptr)
        return;

    Rml::Element* targetElement = event.GetTargetElement();
    const Rml::String elementId = targetElement ? targetElement->GetId() : "";
    const Rml::Vector2f mousePosition = event.GetUnprojectedMouseScreenPos();
    const float mouseX = mousePosition.x;
    const float mouseY = mousePosition.y;

    if (event.GetId() == Rml::EventId::Click)
    {
        if (elementId == "builder_menu_window_button")
        {
            m_isWindowMenuOpen = !m_isWindowMenuOpen;
            refreshPresentation();
            event.StopPropagation();
            return;
        }

        if (elementId == "builder_menu_open_ui_builder")
        {
            m_isWindowMenuOpen = false;
            refreshPresentation();
            if (m_modeChangeCallback)
                m_modeChangeCallback(EditorMode::UiBuilder);
            event.StopPropagation();
            return;
        }

        if (m_isWindowMenuOpen)
        {
            const Rml::String menuHit = findAncestorElementId(targetElement, [](const Rml::String& candidateId) {
                return candidateId == "builder_menu_window";
            });
            if (menuHit.empty())
            {
                m_isWindowMenuOpen = false;
                refreshPresentation();
            }
        }
        return;
    }

    if (event.GetId() == Rml::EventId::Mousedown)
    {
        if (elementId == "left_splitter")
            m_dragTarget = DragTarget::LeftSplitter;
        else if (elementId == "right_splitter")
            m_dragTarget = DragTarget::RightSplitter;
        else if (elementId == "horizontal_splitter")
            m_dragTarget = DragTarget::HorizontalSplitter;

        if (m_dragTarget != DragTarget::None)
            event.StopPropagation();
        return;
    }

    if (event.GetId() == Rml::EventId::Mouseup)
    {
        m_dragTarget = DragTarget::None;
        return;
    }

    if (event.GetId() != Rml::EventId::Mousemove || m_dragTarget == DragTarget::None)
        return;

    if (m_dragTarget == DragTarget::LeftSplitter || m_dragTarget == DragTarget::RightSplitter)
    {
        const int totalWidth = std::max(m_windowWidth, 1);
        int leftWidth = static_cast<int>(std::lround(totalWidth * m_leftRatio));
        int rightWidth = static_cast<int>(std::lround(totalWidth * m_rightRatio));
        rightWidth = clampInt(rightWidth, MinColumnWidth, totalWidth - leftWidth - MinCenterWidth - (2 * SplitterThickness));

        if (m_dragTarget == DragTarget::LeftSplitter)
        {
            const int maxLeftWidth = totalWidth - MinCenterWidth - rightWidth - (2 * SplitterThickness);
            leftWidth = clampInt(static_cast<int>(std::lround(mouseX)), MinColumnWidth, maxLeftWidth);
            m_leftRatio = static_cast<float>(leftWidth) / static_cast<float>(totalWidth);
        }
        else
        {
            leftWidth = clampInt(leftWidth, MinColumnWidth, totalWidth - MinCenterWidth - MinColumnWidth - (2 * SplitterThickness));
            rightWidth = clampInt(
                totalWidth - static_cast<int>(std::lround(mouseX)) - SplitterThickness,
                MinColumnWidth,
                totalWidth - leftWidth - MinCenterWidth - (2 * SplitterThickness));
            m_rightRatio = static_cast<float>(rightWidth) / static_cast<float>(totalWidth);
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

void SceneEditorController::attachListeners()
{
    if (m_document == nullptr)
        return;

    m_document->AddEventListener(Rml::EventId::Click, this);
    m_document->AddEventListener(Rml::EventId::Mousedown, this);
    m_document->AddEventListener(Rml::EventId::Mousemove, this);
    m_document->AddEventListener(Rml::EventId::Mouseup, this);
}

void SceneEditorController::detachListeners()
{
    if (m_document == nullptr)
        return;

    m_document->RemoveEventListener(Rml::EventId::Click, this);
    m_document->RemoveEventListener(Rml::EventId::Mousedown, this);
    m_document->RemoveEventListener(Rml::EventId::Mousemove, this);
    m_document->RemoveEventListener(Rml::EventId::Mouseup, this);
}

void SceneEditorController::applyLayout()
{
    if (m_root == nullptr)
        return;

    const int totalWidth = std::max(m_windowWidth, 1);
    const int totalHeight = std::max(m_windowHeight, 1);
    const int contentTop = BuilderHeaderHeight;
    const int contentHeight = std::max(1, totalHeight - contentTop);

    int leftWidth = static_cast<int>(std::lround(totalWidth * m_leftRatio));
    int rightWidth = static_cast<int>(std::lround(totalWidth * m_rightRatio));

    leftWidth = clampInt(leftWidth, MinColumnWidth, totalWidth - MinCenterWidth - MinColumnWidth - (2 * SplitterThickness));
    rightWidth = clampInt(rightWidth, MinColumnWidth, totalWidth - leftWidth - MinCenterWidth - (2 * SplitterThickness));

    int centerWidth = totalWidth - leftWidth - rightWidth - (2 * SplitterThickness);
    if (centerWidth < MinCenterWidth)
    {
        const int deficit = MinCenterWidth - centerWidth;
        if (rightWidth - deficit >= MinColumnWidth)
            rightWidth -= deficit;
        else
            leftWidth = std::max(MinColumnWidth, leftWidth - (deficit - (rightWidth - MinColumnWidth)));

        rightWidth = clampInt(rightWidth, MinColumnWidth, totalWidth - leftWidth - MinCenterWidth - (2 * SplitterThickness));
        centerWidth = totalWidth - leftWidth - rightWidth - (2 * SplitterThickness);
    }

    m_leftRatio = static_cast<float>(leftWidth) / static_cast<float>(totalWidth);
    m_rightRatio = static_cast<float>(rightWidth) / static_cast<float>(totalWidth);

    const int centerX = leftWidth + SplitterThickness;
    const int rightSplitterX = centerX + centerWidth;
    const int rightX = rightSplitterX + SplitterThickness;

    m_centerRect = {centerX, contentTop, centerWidth, contentHeight};

    m_root->SetProperty("width", pixels(totalWidth));
    m_root->SetProperty("height", pixels(totalHeight));
    m_builderHeader->SetProperty("display", "block");
    m_builderHeader->SetProperty("left", pixels(0));
    m_builderHeader->SetProperty("top", pixels(0));
    m_builderHeader->SetProperty("width", pixels(totalWidth));
    m_builderHeader->SetProperty("height", pixels(BuilderHeaderHeight));

    m_leftPanel->SetProperty("left", pixels(0));
    m_leftPanel->SetProperty("top", pixels(contentTop));
    m_leftPanel->SetProperty("width", pixels(leftWidth));
    m_leftPanel->SetProperty("height", pixels(contentHeight));

    m_leftSplitter->SetProperty("left", pixels(leftWidth));
    m_leftSplitter->SetProperty("top", pixels(contentTop));
    m_leftSplitter->SetProperty("width", pixels(SplitterThickness));
    m_leftSplitter->SetProperty("height", pixels(contentHeight));

    m_centerPanel->SetProperty("left", pixels(centerX));
    m_centerPanel->SetProperty("top", pixels(contentTop));
    m_centerPanel->SetProperty("width", pixels(centerWidth));
    m_centerPanel->SetProperty("height", pixels(contentHeight));

    m_rightSplitter->SetProperty("left", pixels(rightSplitterX));
    m_rightSplitter->SetProperty("top", pixels(contentTop));
    m_rightSplitter->SetProperty("width", pixels(SplitterThickness));
    m_rightSplitter->SetProperty("height", pixels(contentHeight));

    m_rightPanel->SetProperty("left", pixels(rightX));
    m_rightPanel->SetProperty("top", pixels(contentTop));
    m_rightPanel->SetProperty("width", pixels(rightWidth));
    m_rightPanel->SetProperty("height", pixels(contentHeight));

    const int viewportHeight = clampInt(
        static_cast<int>(std::lround(static_cast<float>(contentHeight) * m_viewportRatio)),
        MinViewportHeight,
        contentHeight - MinBottomHeight - SplitterThickness);
    const int bottomHeight = contentHeight - viewportHeight - SplitterThickness;

    m_viewportPanel->SetProperty("display", "block");
    m_viewportPanel->SetProperty("left", pixels(0));
    m_viewportPanel->SetProperty("top", pixels(0));
    m_viewportPanel->SetProperty("width", pixels(centerWidth));
    m_viewportPanel->SetProperty("height", pixels(viewportHeight));

    m_horizontalSplitter->SetProperty("display", "block");
    m_horizontalSplitter->SetProperty("left", pixels(0));
    m_horizontalSplitter->SetProperty("top", pixels(viewportHeight));
    m_horizontalSplitter->SetProperty("width", pixels(centerWidth));
    m_horizontalSplitter->SetProperty("height", pixels(SplitterThickness));

    m_bottomPanel->SetProperty("display", "block");
    m_bottomPanel->SetProperty("left", pixels(0));
    m_bottomPanel->SetProperty("top", pixels(viewportHeight + SplitterThickness));
    m_bottomPanel->SetProperty("width", pixels(centerWidth));
    m_bottomPanel->SetProperty("height", pixels(bottomHeight));
}

void SceneEditorController::refreshPresentation()
{
    if (m_builderHeader == nullptr || m_leftPanel == nullptr || m_rightPanel == nullptr || m_bottomPanel == nullptr)
        return;

    m_builderHeader->SetInnerRML(
        std::string(
            R"RML(<div class='builder_menu_bar'><div id='builder_menu_window' class='builder_menu'><div id='builder_menu_window_button' class='builder_menu_button'>Window</div><div id='builder_menu_window_dropdown' class='builder_menu_dropdown' style='display: )RML") +
        (m_isWindowMenuOpen ? "block" : "none") +
        R"RML(;'><div id='builder_menu_open_ui_builder' class='builder_menu_item'>UI Builder</div></div></div></div>)RML"
    );

    m_leftPanel->SetInnerRML(
        R"RML(<div class='panel_shell'><div class='panel_header'>Scene</div><div class='panel_body'><div class='placeholder_block'><div class='placeholder_title'>Scene panel</div><div class='placeholder_text'>Scene hierarchy and scene-side tools will live here.</div></div></div></div>)RML"
    );
    m_rightPanel->SetInnerRML(
        R"RML(<div class='panel_shell'><div class='panel_header'>Inspector</div><div class='panel_body'><div class='placeholder_block'><div class='placeholder_title'>Inspector panel</div><div class='placeholder_text'>Selection details and editable properties will appear here.</div></div></div></div>)RML"
    );
    m_bottomPanel->SetInnerRML(
        R"RML(<div class='panel_shell'><div class='panel_header'>Bottom Panel</div><div class='panel_body'><div class='placeholder_block'><div class='placeholder_title'>Logs / Assets / Timeline</div><div class='placeholder_text'>This region stays available for the classic editor layout.</div></div></div></div>)RML"
    );
}

void SceneEditorController::refreshCachedRects()
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