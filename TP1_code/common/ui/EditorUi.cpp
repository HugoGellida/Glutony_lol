#include "EditorUi.hpp"
#include "EditorUiDocuments.hpp"
#include "EditorUiPreviewDocument.hpp"

#include <common/platform/NativeFileDialog.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

bool EditorUiController::initialize(Rml::Context* context)
{
    if (context == nullptr)
        return false;

    m_context = context;
    m_document = m_context->LoadDocumentFromMemory(getEditorLayoutDocument(), "[editor-layout]");
    if (m_document == nullptr)
        return false;

    m_root = m_document->GetElementById("root");
    m_builderHeader = m_document->GetElementById("builder_header");
    m_builderMenuFileButton = m_document->GetElementById("builder_menu_file_button");
    m_builderMenuFileDropdown = m_document->GetElementById("builder_menu_file_dropdown");
    m_leftPanel = m_document->GetElementById("left_panel");
    m_leftSplitter = m_document->GetElementById("left_splitter");
    m_centerPanel = m_document->GetElementById("center_panel");
    m_rightSplitter = m_document->GetElementById("right_splitter");
    m_rightPanel = m_document->GetElementById("right_panel");
    m_viewportPanel = m_document->GetElementById("viewport_panel");
    m_horizontalSplitter = m_document->GetElementById("horizontal_splitter");
    m_bottomPanel = m_document->GetElementById("bottom_panel");

    if (m_root == nullptr ||
        m_builderHeader == nullptr ||
        m_builderMenuFileButton == nullptr ||
        m_builderMenuFileDropdown == nullptr ||
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
    m_previewDocumentSource = createEmptyPreviewDocumentSource();
    refreshModePresentation();
    m_document->Show();
    applyLayout();
    reloadPreviewDocument();
    return true;
}

void EditorUiController::shutdown()
{
    unloadPreviewDocument();
    detachListeners();

    if (m_context != nullptr && m_document != nullptr)
        m_context->UnloadDocument(m_document);

    m_document = nullptr;
    m_context = nullptr;
    m_root = nullptr;
    m_builderHeader = nullptr;
    m_builderMenuFileButton = nullptr;
    m_builderMenuFileDropdown = nullptr;
    m_leftPanel = nullptr;
    m_leftTopPanel = nullptr;
    m_leftHorizontalSplitter = nullptr;
    m_leftBottomPanel = nullptr;
    m_leftSplitter = nullptr;
    m_centerPanel = nullptr;
    m_rightSplitter = nullptr;
    m_rightPanel = nullptr;
    m_viewportPanel = nullptr;
    m_previewCanvas = nullptr;
    m_previewWindow = nullptr;
    m_previewHost = nullptr;
    m_previewZoomLabel = nullptr;
    m_horizontalSplitter = nullptr;
    m_bottomPanel = nullptr;
    m_dragTarget = DragTarget::None;
    m_isFileMenuOpen = false;
    m_leftPanelRect = {};
    m_viewportRect = {};
    m_centerRect = {};
    m_previewCanvasRect = {};
    m_previewPageRect = {};
    m_previewWindowRect = {};
    m_previewHostRect = {};
}

void EditorUiController::syncToWindow(int width, int height)
{
    m_windowWidth = std::max(width, 1);
    m_windowHeight = std::max(height, 1);
}

void EditorUiController::setUiBuilderEnabled(bool enabled)
{
    if (m_uiBuilderEnabled == enabled)
        return;

    m_uiBuilderEnabled = enabled;
    if (!m_uiBuilderEnabled)
    {
        m_isFileMenuOpen = false;
        unloadPreviewDocument();
    }
    refreshModePresentation();
    if (m_uiBuilderEnabled)
        reloadPreviewDocument();
}

void EditorUiController::setUiBuilderShowStylePanel(bool showStylePanel)
{
    if (m_uiBuilderShowStylePanel == showStylePanel)
        return;

    m_uiBuilderShowStylePanel = showStylePanel;
    refreshModePresentation();
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
    Rml::Element* targetElement = event.GetTargetElement();
    const Rml::String elementId = targetElement ? targetElement->GetId() : "";
    const Rml::Vector2f mouseScreenPosition = event.GetUnprojectedMouseScreenPos();
    const float mouseX = mouseScreenPosition.x;
    const float mouseY = mouseScreenPosition.y;

    if (eventId == Rml::EventId::Click)
    {
        if (!m_uiBuilderEnabled)
            return;

        if (elementId == "builder_menu_file_button")
        {
            m_isFileMenuOpen = !m_isFileMenuOpen;
            refreshBuilderMenuState();
            event.StopPropagation();
            return;
        }

        if (elementId == "builder_menu_new")
        {
            m_previewDocumentSource = createEmptyPreviewDocumentSource();
            m_previewDocumentPath.clear();
            m_previewDocumentWidth = 1280;
            m_previewDocumentHeight = 720;
            m_previewZoom = 1.0f;
            reloadPreviewDocument();
            refreshPreviewZoomLabel();
            m_isFileMenuOpen = false;
            refreshBuilderMenuState();
            event.StopPropagation();
            return;
        }

        if (elementId == "builder_menu_open")
        {
            const std::optional<std::string> selectedPath = platform::showNativeFileDialog(
                platform::FileDialogMode::OpenFile,
                "Open UI Document",
                m_previewDocumentPath.empty() ? std::filesystem::current_path().string() : m_previewDocumentPath,
                {
                    {"Rml Documents", {"*.rml", "*.xml"}},
                    {"All Files", {"*.*"}},
                });

            if (selectedPath.has_value())
                loadPreviewDocumentFromFile(*selectedPath);

            m_isFileMenuOpen = false;
            refreshBuilderMenuState();
            event.StopPropagation();
            return;
        }

        if (elementId == "builder_menu_save_as")
        {
            const std::string suggestedPath = m_previewDocumentPath.empty()
                ? (std::filesystem::current_path() / "ui-document.rml").string()
                : m_previewDocumentPath;

            const std::optional<std::string> selectedPath = platform::showNativeFileDialog(
                platform::FileDialogMode::SaveFile,
                "Save UI Document As",
                suggestedPath,
                {
                    {"Rml Documents", {"*.rml"}},
                    {"All Files", {"*.*"}},
                });

            if (selectedPath.has_value() && savePreviewDocumentToFile(*selectedPath))
                m_previewDocumentPath = *selectedPath;

            m_isFileMenuOpen = false;
            refreshBuilderMenuState();
            event.StopPropagation();
            return;
        }

        if (elementId == "preview_zoom_out")
        {
            setPreviewZoom(m_previewZoom / 1.1f);
            event.StopPropagation();
            return;
        }

        if (elementId == "preview_zoom_fit")
        {
            fitPreviewZoom();
            event.StopPropagation();
            return;
        }

        if (elementId == "preview_zoom_in")
        {
            setPreviewZoom(m_previewZoom * 1.1f);
            event.StopPropagation();
            return;
        }

        m_isFileMenuOpen = false;
        refreshBuilderMenuState();
        return;
    }

    if (eventId == Rml::EventId::Mousescroll)
    {
        if (!m_uiBuilderEnabled || !m_previewPageRect.isValid())
            return;

        if (!m_previewPageRect.contains(mouseX, mouseY))
            return;

        const float wheelDelta = event.GetParameter<float>("wheel_delta_y", 0.0f);
        if (std::abs(wheelDelta) < 0.001f)
            return;

        const float zoomFactor = wheelDelta < 0.0f ? 1.1f : (1.0f / 1.1f);
        setPreviewZoom(m_previewZoom * zoomFactor);
        event.StopPropagation();
        return;
    }

    if (eventId == Rml::EventId::Mousedown)
    {
        const DragTarget previewResizeTarget = getPreviewResizeTargetAt(mouseX, mouseY);
        if (previewResizeTarget == DragTarget::PreviewResizeRight ||
            previewResizeTarget == DragTarget::PreviewResizeBottom ||
            previewResizeTarget == DragTarget::PreviewResizeCorner)
        {
            m_dragTarget = previewResizeTarget;
            event.StopPropagation();
            return;
        }

        if (elementId == "left_splitter")
        {
            m_dragTarget = DragTarget::LeftSplitter;
            event.StopPropagation();
        }
        else if (elementId == "left_horizontal_splitter")
        {
            m_dragTarget = DragTarget::LeftHorizontalSplitter;
            event.StopPropagation();
        }
        else if (elementId == "right_splitter")
        {
            m_dragTarget = DragTarget::RightSplitter;
            event.StopPropagation();
        }
        else if (elementId == "horizontal_splitter")
        {
            if (!m_uiBuilderEnabled)
            {
                m_dragTarget = DragTarget::HorizontalSplitter;
                event.StopPropagation();
            }
        }
        return;
    }

    if (eventId == Rml::EventId::Mouseup)
    {
        m_dragTarget = DragTarget::None;
        updatePreviewCursor(mouseX, mouseY);
        return;
    }

    if (eventId != Rml::EventId::Mousemove)
        return;

    if (m_dragTarget == DragTarget::None)
    {
        updatePreviewCursor(mouseX, mouseY);
        return;
    }

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

    if (m_dragTarget == DragTarget::LeftHorizontalSplitter && m_leftPanelRect.isValid())
    {
        const int localY = static_cast<int>(std::lround(mouseY)) - m_leftPanelRect.y;
        const int maxTopHeight = m_leftPanelRect.height - MinLeftSectionHeight - SplitterThickness;
        const int topHeight = clampInt(localY, MinLeftSectionHeight, maxTopHeight);
        m_leftTopRatio = static_cast<float>(topHeight) / static_cast<float>(std::max(m_leftPanelRect.height, 1));
        applyLayout();
        event.StopPropagation();
        return;
    }

    if ((m_dragTarget == DragTarget::PreviewResizeRight ||
         m_dragTarget == DragTarget::PreviewResizeBottom ||
         m_dragTarget == DragTarget::PreviewResizeCorner) &&
        m_previewWindowRect.isValid())
    {
        int newDocumentWidth = m_previewDocumentWidth;
        int newDocumentHeight = m_previewDocumentHeight;
        const int maxDisplayedWidth = m_previewCanvasRect.isValid() ? m_previewCanvasRect.width : m_windowWidth;
        const int maxDisplayedHeight = m_previewCanvasRect.isValid() ? m_previewCanvasRect.height : m_windowHeight;

        if (m_dragTarget == DragTarget::PreviewResizeRight || m_dragTarget == DragTarget::PreviewResizeCorner)
        {
            const int displayedWidth = clampInt(
                static_cast<int>(std::lround(mouseX)) - m_previewWindowRect.x,
                static_cast<int>(std::lround(MinPreviewDocumentWidth * m_previewZoom)),
                std::max(static_cast<int>(std::lround(MinPreviewDocumentWidth * m_previewZoom)), maxDisplayedWidth));
            newDocumentWidth = std::max(MinPreviewDocumentWidth, static_cast<int>(std::lround(static_cast<float>(displayedWidth) / m_previewZoom)));
        }

        if (m_dragTarget == DragTarget::PreviewResizeBottom || m_dragTarget == DragTarget::PreviewResizeCorner)
        {
            const int displayedHeight = clampInt(
                static_cast<int>(std::lround(mouseY)) - m_previewWindowRect.y,
                static_cast<int>(std::lround(MinPreviewDocumentHeight * m_previewZoom)),
                std::max(static_cast<int>(std::lround(MinPreviewDocumentHeight * m_previewZoom)), maxDisplayedHeight));
            newDocumentHeight = std::max(MinPreviewDocumentHeight, static_cast<int>(std::lround(static_cast<float>(displayedHeight) / m_previewZoom)));
        }

        if (newDocumentWidth != m_previewDocumentWidth || newDocumentHeight != m_previewDocumentHeight)
        {
            m_previewDocumentWidth = newDocumentWidth;
            m_previewDocumentHeight = newDocumentHeight;
            updatePreviewDocumentPlacement();
        }

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
    const int contentTop = m_uiBuilderEnabled ? BuilderHeaderHeight : 0;
    const int contentHeight = std::max(1, totalHeight - contentTop);

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

    m_centerRect = {centerX, contentTop, centerWidth, contentHeight};

    m_root->SetProperty("width", pixels(totalWidth));
    m_root->SetProperty("height", pixels(totalHeight));

    m_builderHeader->SetProperty("display", m_uiBuilderEnabled ? "block" : "none");
    m_builderHeader->SetProperty("left", pixels(0));
    m_builderHeader->SetProperty("top", pixels(0));
    m_builderHeader->SetProperty("width", pixels(totalWidth));
    m_builderHeader->SetProperty("height", pixels(BuilderHeaderHeight));

    m_leftPanel->SetProperty("left", pixels(0));
    m_leftPanel->SetProperty("top", pixels(contentTop));
    m_leftPanel->SetProperty("width", pixels(leftWidth));
    m_leftPanel->SetProperty("height", pixels(contentHeight));

    if (m_uiBuilderEnabled &&
        m_leftTopPanel != nullptr &&
        m_leftHorizontalSplitter != nullptr &&
        m_leftBottomPanel != nullptr)
    {
        const int topHeight = clampInt(
            static_cast<int>(std::lround(static_cast<float>(contentHeight) * m_leftTopRatio)),
            MinLeftSectionHeight,
            contentHeight - MinLeftSectionHeight - SplitterThickness);
        const int bottomHeight = contentHeight - topHeight - SplitterThickness;

        m_leftTopPanel->SetProperty("left", pixels(0));
        m_leftTopPanel->SetProperty("top", pixels(0));
        m_leftTopPanel->SetProperty("width", pixels(leftWidth));
        m_leftTopPanel->SetProperty("height", pixels(topHeight));

        m_leftHorizontalSplitter->SetProperty("left", pixels(0));
        m_leftHorizontalSplitter->SetProperty("top", pixels(topHeight));
        m_leftHorizontalSplitter->SetProperty("width", pixels(leftWidth));
        m_leftHorizontalSplitter->SetProperty("height", pixels(SplitterThickness));

        m_leftBottomPanel->SetProperty("left", pixels(0));
        m_leftBottomPanel->SetProperty("top", pixels(topHeight + SplitterThickness));
        m_leftBottomPanel->SetProperty("width", pixels(leftWidth));
        m_leftBottomPanel->SetProperty("height", pixels(bottomHeight));
    }

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

    if (m_uiBuilderEnabled)
    {
        m_viewportPanel->SetProperty("display", "block");
        m_viewportPanel->SetProperty("left", pixels(0));
        m_viewportPanel->SetProperty("top", pixels(0));
        m_viewportPanel->SetProperty("width", pixels(centerWidth));
        m_viewportPanel->SetProperty("height", pixels(totalHeight));

        m_horizontalSplitter->SetProperty("display", "none");
        m_bottomPanel->SetProperty("display", "none");
        return;
    }

    const int viewportHeight = clampInt(
        static_cast<int>(std::lround(static_cast<float>(contentHeight) * m_viewportRatio)),
        MinViewportHeight,
        contentHeight - MinBottomHeight - SplitterThickness
    );
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

void EditorUiController::refreshCachedRects()
{
    if (m_viewportPanel == nullptr || m_centerPanel == nullptr)
        return;

    if (m_uiBuilderEnabled)
    {
        m_leftPanelRect.x = static_cast<int>(std::lround(m_leftPanel->GetAbsoluteLeft() + m_leftPanel->GetClientLeft()));
        m_leftPanelRect.y = static_cast<int>(std::lround(m_leftPanel->GetAbsoluteTop() + m_leftPanel->GetClientTop()));
        m_leftPanelRect.width = static_cast<int>(std::lround(m_leftPanel->GetClientWidth()));
        m_leftPanelRect.height = static_cast<int>(std::lround(m_leftPanel->GetClientHeight()));
        m_viewportRect = {};
        m_centerRect.x = static_cast<int>(std::lround(m_centerPanel->GetAbsoluteLeft() + m_centerPanel->GetClientLeft()));
        m_centerRect.y = static_cast<int>(std::lround(m_centerPanel->GetAbsoluteTop() + m_centerPanel->GetClientTop()));
        m_centerRect.width = static_cast<int>(std::lround(m_centerPanel->GetClientWidth()));
        m_centerRect.height = static_cast<int>(std::lround(m_centerPanel->GetClientHeight()));
        if (m_previewCanvas != nullptr)
        {
            m_previewCanvasRect.x = static_cast<int>(std::lround(m_previewCanvas->GetAbsoluteLeft() + m_previewCanvas->GetClientLeft()));
            m_previewCanvasRect.y = static_cast<int>(std::lround(m_previewCanvas->GetAbsoluteTop() + m_previewCanvas->GetClientTop()));
            m_previewCanvasRect.width = static_cast<int>(std::lround(m_previewCanvas->GetClientWidth()));
            m_previewCanvasRect.height = static_cast<int>(std::lround(m_previewCanvas->GetClientHeight()));
        }
        else
        {
            m_previewCanvasRect = {};
        }
        m_previewPageRect = {};
        m_previewWindowRect = {};
        m_previewHostRect = {};
        updatePreviewDocumentPlacement();
        return;
    }

    m_leftPanelRect = {};
    m_viewportRect.x = static_cast<int>(std::lround(m_viewportPanel->GetAbsoluteLeft() + m_viewportPanel->GetClientLeft()));
    m_viewportRect.y = static_cast<int>(std::lround(m_viewportPanel->GetAbsoluteTop() + m_viewportPanel->GetClientTop()));
    m_viewportRect.width = static_cast<int>(std::lround(m_viewportPanel->GetClientWidth()));
    m_viewportRect.height = static_cast<int>(std::lround(m_viewportPanel->GetClientHeight()));

    m_centerRect.x = static_cast<int>(std::lround(m_centerPanel->GetAbsoluteLeft() + m_centerPanel->GetClientLeft()));
    m_centerRect.y = static_cast<int>(std::lround(m_centerPanel->GetAbsoluteTop() + m_centerPanel->GetClientTop()));
    m_centerRect.width = static_cast<int>(std::lround(m_centerPanel->GetClientWidth()));
    m_centerRect.height = static_cast<int>(std::lround(m_centerPanel->GetClientHeight()));
    m_previewCanvasRect = {};
    m_previewPageRect = {};
    m_previewWindowRect = {};
    m_previewHostRect = {};
}

void EditorUiController::refreshModePresentation()
{
    if (m_leftPanel == nullptr ||
        m_rightPanel == nullptr ||
        m_viewportPanel == nullptr ||
        m_bottomPanel == nullptr ||
        m_builderHeader == nullptr)
        return;

    const char* leftTitle = "Scene";
    const char* leftBody = "Scene hierarchy and scene-side tools will live here.";
    const char* rightTitle = "Inspector";
    const char* rightBody = "Selection details and editable properties will appear here.";
    const char* centerMarkup = "";
    const char* bottomMarkup =
        R"RML(<div class='panel_shell'>
    <div class='panel_header'>Bottom Panel</div>
    <div class='panel_body'>
        <div class='placeholder_block'>
            <div class='placeholder_title'>Logs / Assets / Timeline</div>
            <div class='placeholder_text'>This region stays available for the classic editor layout.</div>
        </div>
    </div>
</div>)RML";

    if (m_uiBuilderEnabled)
    {
        rightTitle = "Inspector";
        rightBody = "The selected UI element will expose its editable properties here.";
        centerMarkup =
            R"RML(<div class='preview_shell'>
    <div class='preview_toolbar'>
        <div class='preview_toolbar_group'>
            <div id='preview_zoom_out' class='preview_toolbar_button'>-</div>
            <div id='preview_zoom_label' class='preview_zoom_label'>100%</div>
            <div id='preview_zoom_in' class='preview_toolbar_button'>+</div>
        </div>
        <div id='preview_zoom_fit' class='preview_toolbar_button'>Fit</div>
    </div>
    <div id='preview_canvas' class='preview_canvas'>
        <div id='preview_window' class='preview_window'>
            <div id='preview_host' class='preview_host'></div>
            <div id='preview_resize_right' class='preview_resize_handle preview_resize_right'></div>
            <div id='preview_resize_bottom' class='preview_resize_handle preview_resize_bottom'></div>
            <div id='preview_resize_corner' class='preview_resize_handle preview_resize_corner'></div>
        </div>
    </div>
</div>)RML";
        bottomMarkup = "";
    }

    if (m_uiBuilderEnabled)
    {
        m_leftPanel->SetInnerRML(
            R"RML(<div id='left_top_panel' class='panel panel_nested'>
    <div class='panel_shell'>
        <div class='panel_header'>Hierarchy</div>
        <div class='panel_body'>
            <div class='placeholder_block'>
                <div class='placeholder_title'>Document tree</div>
                <div class='placeholder_text'>The live hierarchy of the previewed document will appear here.</div>
            </div>
        </div>
    </div>
</div>
<div id='left_horizontal_splitter' class='splitter splitter_horizontal_nested'></div>
<div id='left_bottom_panel' class='panel panel_nested'>
    <div class='panel_shell'>
        <div class='panel_header'>Widgets &amp; Templates</div>
        <div class='panel_body'>
            <div class='placeholder_block'>
                <div class='placeholder_title'>Widget catalog</div>
                <div class='placeholder_text'>Reusable widgets and templates will be listed here for drag and drop.</div>
            </div>
        </div>
    </div>
</div>)RML"
        );
    }
    else
    {
        m_leftPanel->SetInnerRML(
            "<div class='panel_shell'><div class='panel_header'>" +
            Rml::String(leftTitle) +
            "</div><div class='panel_body'><div class='placeholder_block'><div class='placeholder_title'>" +
            Rml::String(leftTitle) +
            " panel</div><div class='placeholder_text'>" +
            Rml::String(leftBody) +
            "</div></div></div></div>"
        );
    }

    m_rightPanel->SetInnerRML(
        "<div class='panel_shell'><div class='panel_header'>" +
        Rml::String(rightTitle) +
        "</div><div class='panel_body'><div class='placeholder_block'><div class='placeholder_title'>" +
        Rml::String(rightTitle) +
        " panel</div><div class='placeholder_text'>" +
        Rml::String(rightBody) +
        "</div></div></div></div>"
    );

    m_viewportPanel->SetInnerRML(centerMarkup);
    m_leftTopPanel = m_document->GetElementById("left_top_panel");
    m_leftHorizontalSplitter = m_document->GetElementById("left_horizontal_splitter");
    m_leftBottomPanel = m_document->GetElementById("left_bottom_panel");
    m_previewCanvas = m_document->GetElementById("preview_canvas");
    m_previewWindow = m_document->GetElementById("preview_window");
    m_previewHost = m_document->GetElementById("preview_host");
    m_previewZoomLabel = m_document->GetElementById("preview_zoom_label");
    m_bottomPanel->SetInnerRML(bottomMarkup);
    refreshBuilderMenuState();
    refreshPreviewZoomLabel();
    updatePreviewDocumentPlacement();
}

void EditorUiController::refreshBuilderMenuState()
{
    if (m_builderHeader == nullptr || m_builderMenuFileDropdown == nullptr)
        return;

    m_builderHeader->SetProperty("display", m_uiBuilderEnabled ? "block" : "none");
    m_builderMenuFileDropdown->SetProperty("display", (m_uiBuilderEnabled && m_isFileMenuOpen) ? "block" : "none");
}

void EditorUiController::refreshPreviewZoomLabel()
{
    if (m_previewZoomLabel == nullptr)
        return;

    const int zoomPercent = static_cast<int>(std::lround(m_previewZoom * 100.0f));
    m_previewZoomLabel->SetInnerRML(std::to_string(zoomPercent) + "%");
}

EditorUiController::DragTarget EditorUiController::getPreviewResizeTargetAt(float mouseX, float mouseY) const
{
    if (!m_uiBuilderEnabled || !m_previewPageRect.isValid())
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

void EditorUiController::updatePreviewCursor(float mouseX, float mouseY)
{
    if (m_previewCanvas == nullptr)
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

    m_previewCanvas->SetProperty("cursor", cursor);
    if (m_previewDocument != nullptr)
        m_previewDocument->SetProperty("cursor", cursor);
}

void EditorUiController::fitPreviewZoom()
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

void EditorUiController::reloadPreviewDocument()
{
    if (m_context == nullptr)
        return;

    unloadPreviewDocument();

    if (!m_uiBuilderEnabled || m_previewDocumentSource.empty())
        return;

    const std::string sourceUrl = m_previewDocumentPath.empty() ? "[ui-builder-preview]" : m_previewDocumentPath;
    m_previewDocument = m_context->LoadDocumentFromMemory(m_previewDocumentSource, sourceUrl);
    if (m_previewDocument == nullptr)
    {
        std::cerr << "Failed to load preview RML document." << std::endl;
        return;
    }

    m_previewDocument->Show();
    m_previewDocument->SetProperty("display", "none");
    m_previewDocument->AddEventListener(Rml::EventId::Click, this);
    m_previewDocument->AddEventListener(Rml::EventId::Mousedown, this);
    m_previewDocument->AddEventListener(Rml::EventId::Mousemove, this);
    m_previewDocument->AddEventListener(Rml::EventId::Mousescroll, this);
    m_previewDocument->AddEventListener(Rml::EventId::Mouseup, this);
    m_previewDocument->PullToFront();
    m_previewDocument->UpdateDocument();
    if (m_previewCanvasRect.isValid())
        fitPreviewZoom();
    updatePreviewDocumentPlacement();
}

void EditorUiController::unloadPreviewDocument()
{
    if (m_previewDocument != nullptr)
    {
        m_previewDocument->RemoveEventListener(Rml::EventId::Click, this);
        m_previewDocument->RemoveEventListener(Rml::EventId::Mousedown, this);
        m_previewDocument->RemoveEventListener(Rml::EventId::Mousemove, this);
        m_previewDocument->RemoveEventListener(Rml::EventId::Mousescroll, this);
        m_previewDocument->RemoveEventListener(Rml::EventId::Mouseup, this);
    }

    if (m_context != nullptr && m_previewDocument != nullptr)
        m_context->UnloadDocument(m_previewDocument);

    m_previewDocument = nullptr;
}

void EditorUiController::updatePreviewDocumentPlacement()
{
    if (m_previewDocument == nullptr)
        return;

    if (!m_uiBuilderEnabled || !m_previewCanvasRect.isValid())
    {
        if (m_previewWindow != nullptr)
            m_previewWindow->SetProperty("display", "none");
        m_previewDocument->SetProperty("display", "none");
        m_previewPageRect = {};
        return;
    }

    const int maxDisplayedWidth = std::max(1, m_previewCanvasRect.width);
    const int maxDisplayedHeight = std::max(1, m_previewCanvasRect.height);
    const int displayedWidth = clampInt(
        std::max(1, static_cast<int>(std::lround(static_cast<float>(m_previewDocumentWidth) * m_previewZoom))),
        static_cast<int>(std::lround(MinPreviewDocumentWidth * m_previewZoom)),
        maxDisplayedWidth);
    const int displayedHeight = clampInt(
        std::max(1, static_cast<int>(std::lround(static_cast<float>(m_previewDocumentHeight) * m_previewZoom))),
        static_cast<int>(std::lround(MinPreviewDocumentHeight * m_previewZoom)),
        maxDisplayedHeight);

    const int pageLeft = m_previewCanvasRect.x + std::max(0, (m_previewCanvasRect.width - displayedWidth) / 2);
    const int pageTop = m_previewCanvasRect.y + std::max(0, (m_previewCanvasRect.height - displayedHeight) / 2);
    m_previewPageRect = {pageLeft, pageTop, displayedWidth, displayedHeight};

    if (m_previewWindow != nullptr)
    {
        m_previewWindow->SetProperty("display", "block");
        m_previewWindow->SetProperty("left", pixels(std::max(0, (m_previewCanvasRect.width - displayedWidth) / 2)));
        m_previewWindow->SetProperty("top", pixels(std::max(0, (m_previewCanvasRect.height - displayedHeight) / 2)));
        m_previewWindow->SetProperty("width", pixels(displayedWidth));
        m_previewWindow->SetProperty("height", pixels(displayedHeight));
    }

    if (m_document != nullptr)
        m_document->UpdateDocument();

    if (m_previewCanvas != nullptr)
    {
        m_previewCanvasRect.x = static_cast<int>(std::lround(m_previewCanvas->GetAbsoluteLeft() + m_previewCanvas->GetClientLeft()));
        m_previewCanvasRect.y = static_cast<int>(std::lround(m_previewCanvas->GetAbsoluteTop() + m_previewCanvas->GetClientTop()));
        m_previewCanvasRect.width = static_cast<int>(std::lround(m_previewCanvas->GetClientWidth()));
        m_previewCanvasRect.height = static_cast<int>(std::lround(m_previewCanvas->GetClientHeight()));
    }

    if (m_previewWindow != nullptr)
    {
        m_previewWindowRect.x = static_cast<int>(std::lround(m_previewWindow->GetAbsoluteLeft() + m_previewWindow->GetClientLeft()));
        m_previewWindowRect.y = static_cast<int>(std::lround(m_previewWindow->GetAbsoluteTop() + m_previewWindow->GetClientTop()));
        m_previewWindowRect.width = static_cast<int>(std::lround(m_previewWindow->GetClientWidth()));
        m_previewWindowRect.height = static_cast<int>(std::lround(m_previewWindow->GetClientHeight()));
    }

    if (m_previewHost != nullptr)
    {
        m_previewHostRect.x = static_cast<int>(std::lround(m_previewHost->GetAbsoluteLeft() + m_previewHost->GetClientLeft()));
        m_previewHostRect.y = static_cast<int>(std::lround(m_previewHost->GetAbsoluteTop() + m_previewHost->GetClientTop()));
        m_previewHostRect.width = static_cast<int>(std::lround(m_previewHost->GetClientWidth()));
        m_previewHostRect.height = static_cast<int>(std::lround(m_previewHost->GetClientHeight()));
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
    m_previewDocument->SetProperty("left", pixels(m_previewCanvasRect.x));
    m_previewDocument->SetProperty("top", pixels(m_previewCanvasRect.y));
    m_previewDocument->SetProperty("width", pixels(m_previewCanvasRect.width));
    m_previewDocument->SetProperty("height", pixels(m_previewCanvasRect.height));
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
        bodyElement->SetProperty("left", pixels(std::max(0, (m_previewCanvasRect.width - displayedWidth) / 2)));
        bodyElement->SetProperty("top", pixels(std::max(0, (m_previewCanvasRect.height - displayedHeight) / 2)));
        bodyElement->SetProperty("width", pixels(m_previewDocumentWidth));
        bodyElement->SetProperty("height", pixels(m_previewDocumentHeight));
        bodyElement->SetProperty("overflow", "hidden");
        bodyElement->SetProperty("transform-origin", "0px 0px");
        bodyElement->SetProperty("transform", "scale(" + std::to_string(m_previewZoom) + ")");
    }
    m_previewDocument->UpdateDocument();
}

bool EditorUiController::loadPreviewDocumentFromFile(const std::string& filePath)
{
    std::ifstream stream(filePath);
    if (!stream.is_open())
    {
        std::cerr << "Unable to open UI document: " << filePath << std::endl;
        return false;
    }

    std::ostringstream buffer;
    buffer << stream.rdbuf();

    const std::string previousSource = m_previewDocumentSource;
    const std::string previousPath = m_previewDocumentPath;

    m_previewDocumentSource = buffer.str();
    m_previewDocumentPath = filePath;
    reloadPreviewDocument();

    if (m_uiBuilderEnabled && m_previewDocument == nullptr)
    {
        m_previewDocumentSource = previousSource;
        m_previewDocumentPath = previousPath;
        reloadPreviewDocument();
        return false;
    }

    return true;
}

bool EditorUiController::savePreviewDocumentToFile(const std::string& filePath) const
{
    std::ofstream stream(filePath);
    if (!stream.is_open())
    {
        std::cerr << "Unable to save UI document: " << filePath << std::endl;
        return false;
    }

    stream << m_previewDocumentSource;
    return stream.good();
}

void EditorUiController::setPreviewZoom(float zoom)
{
    m_previewZoom = std::max(0.25f, std::min(zoom, 3.0f));
    refreshPreviewZoomLabel();
    updatePreviewDocumentPlacement();
}

void EditorUiController::attachListeners()
{
    if (m_document == nullptr)
        return;

    m_document->AddEventListener(Rml::EventId::Click, this);
    m_document->AddEventListener(Rml::EventId::Mousedown, this);
    m_document->AddEventListener(Rml::EventId::Mousemove, this);
    m_document->AddEventListener(Rml::EventId::Mousescroll, this);
    m_document->AddEventListener(Rml::EventId::Mouseup, this);
}

void EditorUiController::detachListeners()
{
    if (m_document != nullptr)
    {
        m_document->RemoveEventListener(Rml::EventId::Click, this);
        m_document->RemoveEventListener(Rml::EventId::Mousedown, this);
        m_document->RemoveEventListener(Rml::EventId::Mousemove, this);
        m_document->RemoveEventListener(Rml::EventId::Mousescroll, this);
        m_document->RemoveEventListener(Rml::EventId::Mouseup, this);
    }
}

Rml::String EditorUiController::pixels(int value)
{
    return std::to_string(value) + "px";
}

int EditorUiController::clampInt(int value, int minValue, int maxValue)
{
    return std::max(minValue, std::min(value, maxValue));
}