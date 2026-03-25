#include "EditorUi.hpp"
#include "EditorUiDocuments.hpp"
#include "EditorUiPreviewDocument.hpp"

#include <RmlUi/Core/Elements/ElementFormControl.h>

#include <common/ui/widget/WidgetRegistry.hpp>

#include <common/platform/NativeFileDialog.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

namespace
{
bool startsWith(const std::string& value, const std::string& prefix)
{
    return value.rfind(prefix, 0) == 0;
}

const Rml::Element* findFirstElementByTagName(const Rml::Element* root, const Rml::String& tagName)
{
    if (root == nullptr)
        return nullptr;

    if (root->GetTagName() == tagName)
        return root;

    for (int childIndex = 0; childIndex < root->GetNumChildren(true); ++childIndex)
    {
        if (const Rml::Element* found = findFirstElementByTagName(root->GetChild(childIndex), tagName))
            return found;
    }

    return nullptr;
}

std::string escapeRmlText(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char character : value)
    {
        switch (character)
        {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        case '\'': escaped += "&apos;"; break;
        default: escaped.push_back(character); break;
        }
    }
    return escaped;
}
}

bool EditorUiController::initialize(Rml::Context* context)
{
    if (context == nullptr)
        return false;

    m_context = context;
    const Rml::Vector2i contextDimensions = m_context->GetDimensions();
    m_windowWidth = std::max(contextDimensions.x, 1);
    m_windowHeight = std::max(contextDimensions.y, 1);
    m_previewRenderer.initialize(context);
    m_document = m_context->LoadDocumentFromMemory(getEditorLayoutDocument(), "[editor-layout]");
    if (m_document == nullptr)
        return false;

    m_root = m_document->GetElementById("root");
    m_builderHeader = m_document->GetElementById("builder_header");
    m_builderMenuFileButton = m_document->GetElementById("builder_menu_file_button");
    m_builderMenuFileDropdown = m_document->GetElementById("builder_menu_file_dropdown");
    m_builderMenuWindowButton = m_document->GetElementById("builder_menu_window_button");
    m_builderMenuWindowDropdown = m_document->GetElementById("builder_menu_window_dropdown");
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
        m_builderMenuWindowButton == nullptr ||
        m_builderMenuWindowDropdown == nullptr ||
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
    m_previewDocumentSource = buildPreviewDocumentSourceFromHierarchy();
    refreshModePresentation();
    m_document->Show();
    applyLayout();
    reloadPreviewDocument();
    return true;
}

void EditorUiController::shutdown()
{
    unloadPreviewDocument();
    m_previewRenderer.shutdown();
    detachListeners();

    if (m_context != nullptr && m_document != nullptr)
        m_context->UnloadDocument(m_document);

    m_document = nullptr;
    m_context = nullptr;
    m_root = nullptr;
    m_builderHeader = nullptr;
    m_builderMenuFileButton = nullptr;
    m_builderMenuFileDropdown = nullptr;
    m_builderMenuWindowButton = nullptr;
    m_builderMenuWindowDropdown = nullptr;
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
    m_previewModeButton = nullptr;
    m_horizontalSplitter = nullptr;
    m_bottomPanel = nullptr;
    m_dragTarget = DragTarget::None;
    m_isFileMenuOpen = false;
    m_isWindowMenuOpen = false;
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
        m_isWindowMenuOpen = false;
        unloadPreviewDocument();
    }
    else
    {
        m_isWindowMenuOpen = false;
    }
    refreshModePresentation();
    if (m_uiBuilderEnabled)
        reloadPreviewDocument();
}

bool EditorUiController::isUiBuilderEnabled() const
{
    return m_uiBuilderEnabled;
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

    if (m_hierarchyRefreshPending)
    {
        refreshHierarchyPresentation();
        m_hierarchyRefreshPending = false;
    }

    if (m_previewRefreshPending)
    {
        refreshPreviewFromHierarchy();
        m_previewRefreshPending = false;
    }

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
    const auto findAncestorElementId = [&](const std::function<bool(const Rml::String&)>& predicate) -> Rml::String
    {
        for (Rml::Element* element = targetElement; element != nullptr; element = element->GetParentNode())
        {
            const Rml::String candidateId = element->GetId();
            if (!candidateId.empty() && predicate(candidateId))
                return candidateId;
        }
        return "";
    };
    const Rml::String hierarchyNodeElementId = findAncestorElementId(
        [&](const Rml::String& candidateId) { return parseHierarchyNodeId(candidateId).has_value(); });
    const Rml::String hierarchyDropElementId = findAncestorElementId(
        [&](const Rml::String& candidateId) { return parseHierarchyDropId(candidateId).has_value(); });
    const Rml::String widgetCatalogElementId = findAncestorElementId(
        [&](const Rml::String& candidateId) { return startsWith(std::string(candidateId), "widget_catalog_item_"); });
    const Rml::String hierarchyContextItemElementId = findAncestorElementId(
        [&](const Rml::String& candidateId) {
            return candidateId == "hierarchy_context_move_up" ||
                candidateId == "hierarchy_context_move_down" ||
                candidateId == "hierarchy_context_delete";
        });
    const Rml::String inspectorToggleElementId = findAncestorElementId(
        [&](const Rml::String& candidateId) { return startsWith(std::string(candidateId), "inspector_group_toggle_"); });

    if (eventId == Rml::EventId::Change || eventId == Rml::EventId::Blur)
    {
        if (!m_uiBuilderEnabled)
            return;

        const auto inspectorField = parseInspectorFieldElementId(elementId);
        if (!inspectorField.has_value())
            return;

        const Rml::ElementFormControl* formControl = dynamic_cast<const Rml::ElementFormControl*>(targetElement);
        if (formControl == nullptr)
            return;

        const std::string tagName = targetElement->GetTagName();
        if (eventId == Rml::EventId::Change && tagName == "input")
        {
            const std::string inputType = targetElement->GetAttribute<Rml::String>("type", "text").c_str();
            if (inputType == "text" || inputType == "number")
                return;
        }

        if (applyInspectorFieldValue(inspectorField->first, inspectorField->second, formControl->GetValue().c_str()))
        {
            requestHierarchyRefresh();
            requestPreviewRefreshFromHierarchy();
        }
        event.StopPropagation();
        return;
    }

    if (eventId == Rml::EventId::Click)
    {
        if (elementId == "builder_menu_file_button")
        {
            m_isFileMenuOpen = !m_isFileMenuOpen;
            m_isWindowMenuOpen = false;
            refreshBuilderMenuState();
            event.StopPropagation();
            return;
        }

        if (elementId == "builder_menu_window_button")
        {
            m_isWindowMenuOpen = !m_isWindowMenuOpen;
            m_isFileMenuOpen = false;
            refreshBuilderMenuState();
            event.StopPropagation();
            return;
        }

        if (elementId == "builder_menu_back_to_editor")
        {
            m_isFileMenuOpen = false;
            m_isWindowMenuOpen = false;
            refreshBuilderMenuState();
            setUiBuilderEnabled(false);
            event.StopPropagation();
            return;
        }

        if (elementId == "builder_menu_open_ui_builder")
        {
            m_isWindowMenuOpen = false;
            m_isFileMenuOpen = false;
            refreshBuilderMenuState();
            setUiBuilderEnabled(true);
            event.StopPropagation();
            return;
        }

        if (!m_uiBuilderEnabled)
        {
            m_isFileMenuOpen = false;
            m_isWindowMenuOpen = false;
            refreshBuilderMenuState();
            return;
        }

        if (elementId == "hierarchy_context_move_up")
        {
            const bool didMove = moveHierarchyNodeUp(m_hierarchyContextNodeId);
            closeHierarchyContextMenu();
            if (didMove)
            {
                m_hierarchyRefreshPending = false;
                m_previewRefreshPending = false;
                refreshHierarchyPresentation();
                refreshPreviewFromHierarchy();
            }
            else
            {
                refreshHierarchyPresentation();
            }
            event.StopPropagation();
            return;
        }

        if (elementId == "hierarchy_context_move_down")
        {
            const bool didMove = moveHierarchyNodeDown(m_hierarchyContextNodeId);
            closeHierarchyContextMenu();
            if (didMove)
            {
                m_hierarchyRefreshPending = false;
                m_previewRefreshPending = false;
                refreshHierarchyPresentation();
                refreshPreviewFromHierarchy();
            }
            else
            {
                refreshHierarchyPresentation();
            }
            event.StopPropagation();
            return;
        }

        if (elementId == "hierarchy_context_delete")
        {
            const bool didDelete = m_hierarchyContextNodeId != m_hierarchyRoot.id && removeHierarchyNodeById(m_hierarchyContextNodeId, nullptr);
            if (didDelete)
                m_selectedHierarchyNodeId = m_hierarchyRoot.id;
            closeHierarchyContextMenu();
            if (didDelete)
            {
                m_hierarchyRefreshPending = false;
                m_previewRefreshPending = false;
                refreshHierarchyPresentation();
                refreshPreviewFromHierarchy();
            }
            else
            {
                refreshHierarchyPresentation();
            }
            event.StopPropagation();
            return;
        }

        if (!inspectorToggleElementId.empty())
        {
            toggleInspectorGroup(std::string(inspectorToggleElementId).substr(std::string("inspector_group_toggle_").size()));
            refreshInspectorPresentation();
            event.StopPropagation();
            return;
        }

        if (const std::optional<int> hierarchyNodeId = parseHierarchyNodeId(hierarchyNodeElementId))
        {
            m_selectedHierarchyNodeId = *hierarchyNodeId;
            closeHierarchyContextMenu();
            requestHierarchyRefresh();
            event.StopPropagation();
            return;
        }

        if (elementId == "builder_menu_new")
        {
            resetHierarchyModel();
            requestHierarchyRefresh();
            requestPreviewRefreshFromHierarchy();
            m_previewDocumentPath.clear();
            m_previewDocumentWidth = 1280;
            m_previewDocumentHeight = 720;
            m_previewZoom = 1.0f;
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
            if (m_previewDrivenByHierarchy && m_previewRefreshPending)
            {
                refreshPreviewFromHierarchy();
                m_previewRefreshPending = false;
            }

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

        if (elementId == "preview_mode_toggle")
        {
            m_previewDisplayMode = m_previewDisplayMode == PreviewDisplayMode::Preview
                ? PreviewDisplayMode::Render
                : PreviewDisplayMode::Preview;
            refreshPreviewModeButtonLabel();
            requestPreviewRefreshFromHierarchy();
            event.StopPropagation();
            return;
        }

        if (elementId == "preview_zoom_in")
        {
            setPreviewZoom(m_previewZoom * 1.1f);
            event.StopPropagation();
            return;
        }

        closeHierarchyContextMenu();
        m_isFileMenuOpen = false;
        m_isWindowMenuOpen = false;
        refreshBuilderMenuState();
        return;
    }

    if (eventId == Rml::EventId::Dragstart)
    {
        const std::string targetId = widgetCatalogElementId.empty() ? std::string(hierarchyNodeElementId) : std::string(widgetCatalogElementId);
        if (startsWith(targetId, "widget_catalog_item_"))
        {
            m_dragPayloadKind = DragPayloadKind::Widget;
            m_draggedWidgetKey = targetId.substr(std::string("widget_catalog_item_").size());
            m_draggedHierarchyNodeId = 0;
            m_dropTargetNodeId = 0;
            m_dropMode = HierarchyDropMode::None;
            event.StopPropagation();
            return;
        }

        if (const std::optional<int> hierarchyNodeId = parseHierarchyNodeId(hierarchyNodeElementId))
        {
            if (*hierarchyNodeId != m_hierarchyRoot.id)
            {
                m_dragPayloadKind = DragPayloadKind::Node;
                m_draggedHierarchyNodeId = *hierarchyNodeId;
                m_draggedWidgetKey.clear();
                m_dropTargetNodeId = 0;
                m_dropMode = HierarchyDropMode::None;
                m_selectedHierarchyNodeId = *hierarchyNodeId;
                event.StopPropagation();
                return;
            }
        }
    }

    if (eventId == Rml::EventId::Dragover)
    {
        if (m_dragPayloadKind == DragPayloadKind::None)
            return;

        if (const std::optional<std::pair<int, HierarchyDropMode>> dropTarget = parseHierarchyDropId(hierarchyDropElementId))
        {
            m_dropTargetNodeId = dropTarget->first;
            m_dropMode = dropTarget->second;
            event.StopPropagation();
            return;
        }

        if (const std::optional<int> hierarchyNodeId = parseHierarchyNodeId(hierarchyNodeElementId))
        {
            m_dropTargetNodeId = *hierarchyNodeId;
            m_dropMode = HierarchyDropMode::Inside;
            event.StopPropagation();
            return;
        }
    }

    if (eventId == Rml::EventId::Dragdrop)
    {
        if (m_dragPayloadKind == DragPayloadKind::None)
            return;

        std::optional<std::pair<int, HierarchyDropMode>> dropTarget = parseHierarchyDropId(hierarchyDropElementId);
        if (!dropTarget.has_value())
        {
            if (const std::optional<int> hierarchyNodeId = parseHierarchyNodeId(hierarchyNodeElementId))
                dropTarget = std::make_pair(*hierarchyNodeId, HierarchyDropMode::Inside);
        }

        if (!dropTarget.has_value())
            return;

        const int targetNodeId = dropTarget->first;
        const HierarchyDropMode dropMode = dropTarget->second;
        bool didApplyDrop = false;

        if (m_dragPayloadKind == DragPayloadKind::Widget)
        {
            UiHierarchyNode newNode = makeHierarchyNodeForElementKey(m_draggedWidgetKey);
            if (dropMode == HierarchyDropMode::Before)
                didApplyDrop = insertHierarchyNodeBefore(targetNodeId, std::move(newNode));
            else if (dropMode == HierarchyDropMode::After)
                didApplyDrop = insertHierarchyNodeAfter(targetNodeId, std::move(newNode));
            else
                didApplyDrop = insertHierarchyNodeInside(targetNodeId, std::move(newNode));

            if (didApplyDrop)
                m_selectedHierarchyNodeId = m_nextHierarchyNodeId - 1;
        }
        else if (m_dragPayloadKind == DragPayloadKind::Node)
        {
            if (m_draggedHierarchyNodeId != targetNodeId && !isHierarchyNodeDescendantOf(targetNodeId, m_draggedHierarchyNodeId))
            {
                UiHierarchyNode movedNode;
                if (removeHierarchyNodeById(m_draggedHierarchyNodeId, &movedNode))
                {
                    if (dropMode == HierarchyDropMode::Before)
                        didApplyDrop = insertHierarchyNodeBefore(targetNodeId, std::move(movedNode));
                    else if (dropMode == HierarchyDropMode::After)
                        didApplyDrop = insertHierarchyNodeAfter(targetNodeId, std::move(movedNode));
                    else
                        didApplyDrop = insertHierarchyNodeInside(targetNodeId, std::move(movedNode));

                    if (didApplyDrop)
                        m_selectedHierarchyNodeId = m_draggedHierarchyNodeId;
                }
            }
        }

        m_dragPayloadKind = DragPayloadKind::None;
        m_draggedWidgetKey.clear();
        m_draggedHierarchyNodeId = 0;
        m_dropTargetNodeId = 0;
        m_dropMode = HierarchyDropMode::None;
        if (didApplyDrop)
            requestPreviewRefreshFromHierarchy();
        requestHierarchyRefresh();
        event.StopPropagation();
        return;
    }

    if (eventId == Rml::EventId::Dragend)
    {
        m_dragPayloadKind = DragPayloadKind::None;
        m_draggedWidgetKey.clear();
        m_draggedHierarchyNodeId = 0;
        m_dropTargetNodeId = 0;
        m_dropMode = HierarchyDropMode::None;
        requestHierarchyRefresh();
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
        const int mouseButton = event.GetParameter<int>("button", 0);
        if (mouseButton == 1)
        {
            if (const std::optional<int> hierarchyNodeId = parseHierarchyNodeId(hierarchyNodeElementId))
            {
                if (*hierarchyNodeId != m_hierarchyRoot.id && m_leftTopPanel != nullptr)
                {
                    m_selectedHierarchyNodeId = *hierarchyNodeId;
                    m_hierarchyContextNodeId = *hierarchyNodeId;
                    m_hierarchyContextMenuOpen = true;
                    m_hierarchyContextMenuX = static_cast<int>(std::lround(mouseX - m_leftTopPanel->GetAbsoluteLeft()));
                    m_hierarchyContextMenuY = static_cast<int>(std::lround(mouseY - m_leftTopPanel->GetAbsoluteTop()));
                    requestHierarchyRefresh();
                    event.StopPropagation();
                    return;
                }
            }

            if (m_hierarchyContextMenuOpen)
            {
                closeHierarchyContextMenu();
                requestHierarchyRefresh();
                event.StopPropagation();
                return;
            }
        }
        else if (m_hierarchyContextMenuOpen)
        {
            if (hierarchyContextItemElementId.empty())
            {
                closeHierarchyContextMenu();
                requestHierarchyRefresh();
            }
        }

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
        int rightWidth = static_cast<int>(std::lround(totalWidth * m_rightRatio));
        rightWidth = clampInt(rightWidth, minRightWidth, totalWidth - leftWidth - minCenterWidth - (2 * SplitterThickness));

        if (m_dragTarget == DragTarget::LeftSplitter)
        {
            const int maxLeftWidth = totalWidth - minCenterWidth - rightWidth - (2 * SplitterThickness);
            leftWidth = clampInt(static_cast<int>(std::lround(mouseX)), minLeftWidth, maxLeftWidth);
            m_leftRatio = static_cast<float>(leftWidth) / static_cast<float>(totalWidth);
        }
        else if (m_dragTarget == DragTarget::RightSplitter)
        {
            leftWidth = clampInt(leftWidth, minLeftWidth, totalWidth - minCenterWidth - minRightWidth - (2 * SplitterThickness));
            rightWidth = clampInt(
                totalWidth - static_cast<int>(std::lround(mouseX)) - SplitterThickness,
                minRightWidth,
                totalWidth - leftWidth - minCenterWidth - (2 * SplitterThickness));
            m_rightRatio = static_cast<float>(rightWidth) / static_cast<float>(totalWidth);
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
        <div class='preview_toolbar_group'>
            <div id='preview_mode_toggle' class='preview_toolbar_button'>Render</div>
            <div id='preview_zoom_fit' class='preview_toolbar_button'>Fit</div>
        </div>
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
    m_previewModeButton = m_document->GetElementById("preview_mode_toggle");
    m_bottomPanel->SetInnerRML(bottomMarkup);
    refreshBuilderMenuState();
    refreshPreviewZoomLabel();
    refreshPreviewModeButtonLabel();
    refreshHierarchyPresentation();
    updatePreviewDocumentPlacement();
}

void EditorUiController::refreshBuilderMenuState()
{
    if (m_builderHeader == nullptr ||
        m_builderMenuFileButton == nullptr ||
        m_builderMenuFileDropdown == nullptr ||
        m_builderMenuWindowButton == nullptr ||
        m_builderMenuWindowDropdown == nullptr)
        return;

    m_builderHeader->SetProperty("display", "block");
    m_builderMenuFileButton->SetProperty("display", m_uiBuilderEnabled ? "block" : "none");
    m_builderMenuFileDropdown->SetProperty("display", (m_uiBuilderEnabled && m_isFileMenuOpen) ? "block" : "none");
    m_builderMenuWindowButton->SetProperty("display", m_uiBuilderEnabled ? "none" : "block");
    m_builderMenuWindowDropdown->SetProperty("display", (!m_uiBuilderEnabled && m_isWindowMenuOpen) ? "block" : "none");
}

void EditorUiController::refreshPreviewZoomLabel()
{
    if (m_previewZoomLabel == nullptr)
        return;

    const int zoomPercent = static_cast<int>(std::lround(m_previewZoom * 100.0f));
    m_previewZoomLabel->SetInnerRML(std::to_string(zoomPercent) + "%");
}

void EditorUiController::refreshPreviewModeButtonLabel()
{
    if (m_previewModeButton == nullptr)
        return;

    m_previewModeButton->SetInnerRML(
        m_previewDisplayMode == PreviewDisplayMode::Preview ? "Render" : "Preview");
}

void EditorUiController::refreshHierarchyPresentation()
{
    if (m_leftTopPanel == nullptr || m_leftBottomPanel == nullptr)
        return;

    m_leftTopPanel->SetInnerRML(buildHierarchyMarkup());
    m_leftBottomPanel->SetInnerRML(buildWidgetCatalogMarkup());
    refreshInspectorPresentation();
}

void EditorUiController::refreshInspectorPresentation()
{
    if (m_rightPanel == nullptr)
        return;

    m_rightPanel->SetInnerRML(buildInspectorPanelMarkup());
}

void EditorUiController::requestHierarchyRefresh()
{
    m_hierarchyRefreshPending = true;
}

void EditorUiController::requestPreviewRefreshFromHierarchy()
{
    m_previewDrivenByHierarchy = true;
    m_previewRefreshPending = true;
}

void EditorUiController::refreshPreviewFromHierarchy()
{
    m_previewDrivenByHierarchy = true;
    m_previewDocumentSource = m_previewDisplayMode == PreviewDisplayMode::Preview
        ? buildPreviewDocumentSourceFromHierarchy()
        : buildRenderDocumentSourceFromHierarchy();
    reloadPreviewDocument();
}

void EditorUiController::resetHierarchyModel()
{
    m_hierarchyRoot.children.clear();
    m_selectedHierarchyNodeId = m_hierarchyRoot.id;
    m_nextHierarchyNodeId = m_hierarchyRoot.id + 1;
    m_dragPayloadKind = DragPayloadKind::None;
    m_draggedWidgetKey.clear();
    m_draggedHierarchyNodeId = 0;
    m_dropTargetNodeId = 0;
    m_dropMode = HierarchyDropMode::None;
}

std::string EditorUiController::buildInspectorPanelMarkup() const
{
    if (!m_uiBuilderEnabled)
    {
        return R"RML(<div class='panel_shell'><div class='panel_header'>Inspector</div><div class='panel_body'><div class='placeholder_block'><div class='placeholder_title'>Inspector panel</div><div class='placeholder_text'>Selection details and editable properties will appear here.</div></div></div></div>)RML";
    }

    const UiHierarchyNode* selectedNode = findSelectedHierarchyNode();
    if (selectedNode == nullptr || selectedNode->id == m_hierarchyRoot.id)
    {
        return R"RML(<div class='panel_shell'><div class='panel_header'>Inspector</div><div class='panel_body'><div class='placeholder_block'><div class='placeholder_title'>Root</div><div class='placeholder_text'>Select a non-root widget to inspect its metadata and widget-specific settings.</div></div></div></div>)RML";
    }

    const ui::widget::Widget* widget = ui::widget::findWidgetByKey(selectedNode->elementKey);

    if (widget == nullptr)
    {
        const std::string fallback = std::string("<div class='placeholder_block'><div class='placeholder_title'>") + escapeRmlText(selectedNode->label) + "</div><div class='placeholder_text'>Unknown widget type.</div></div>";
        return std::string("<div class='panel_shell'><div class='panel_header'>Inspector</div><div class='panel_body'>") + fallback + "</div></div>";
    }

    const ui::widget::InspectorModel model = widget->buildInspectorModel(selectedNode->label, selectedNode->properties);
    const auto buildFieldMarkup = [&](const ui::widget::InspectorField& field) {
        std::ostringstream fieldStream;
        fieldStream << "<div class='inspector_field_row'><div class='inspector_field_name'>" << escapeRmlText(field.label) << "</div>";
        const std::string fieldId = makeInspectorFieldElementId(selectedNode->id, field.key);
        if (field.inputKind == ui::widget::InspectorInputKind::Enum)
        {
            fieldStream << "<select id='" << fieldId << "' class='inspector_field_input'>";
            for (const ui::widget::InspectorOption& option : field.options)
            {
                fieldStream << "<option value='" << escapeRmlText(option.value) << "'";
                if (option.value == field.value)
                    fieldStream << " selected='selected'";
                fieldStream << ">" << escapeRmlText(option.label) << "</option>";
            }
            fieldStream << "</select>";
        }
        else
        {
            fieldStream << "<input id='" << fieldId << "' class='inspector_field_input' type='";
            fieldStream << (field.inputKind == ui::widget::InspectorInputKind::Number ? "number" : "text");
            fieldStream << "' value='" << escapeRmlText(field.value) << "' />";
        }
        fieldStream << "</div>";
        return fieldStream.str();
    };

    std::ostringstream bodyStream;
    bodyStream << "<div class='inspector_summary'>";
    bodyStream << "<div class='inspector_summary_title'>" << escapeRmlText(widget->name()) << "</div>";
    bodyStream << "<div class='inspector_summary_text'>" << escapeRmlText(widget->description()) << "</div>";
    bodyStream << "</div>";

    if (!model.fields.empty())
    {
        bodyStream << "<div class='inspector_section'>";
        for (const ui::widget::InspectorField& field : model.fields)
            bodyStream << buildFieldMarkup(field);
        bodyStream << "</div>";
    }

    for (const ui::widget::InspectorGroup& group : model.childGroups)
    {
        const std::string groupId = "node_" + std::to_string(selectedNode->id) + "_" + group.key;
        const bool collapsed = isInspectorGroupCollapsed(groupId);
        bodyStream << "<div class='inspector_foldout'>";
        bodyStream << "<div id='inspector_group_toggle_" << groupId << "' class='inspector_foldout_header'>";
        bodyStream << "<div class='inspector_foldout_icon'>" << (collapsed ? ">" : "v") << "</div>";
        bodyStream << "<div class='inspector_foldout_title'>" << escapeRmlText(group.title) << "</div>";
        bodyStream << "</div>";
        if (!collapsed)
        {
            bodyStream << "<div class='inspector_foldout_body'>";
            for (const ui::widget::InspectorField& field : group.fields)
                bodyStream << buildFieldMarkup(field);
            bodyStream << "</div>";
        }
        bodyStream << "</div>";
    }

    return std::string("<div class='panel_shell'><div class='panel_header'>Inspector</div><div class='panel_body inspector_panel_body'>") + bodyStream.str() + "</div></div>";
}

std::string EditorUiController::buildHierarchyMarkup() const
{
    return std::string(
    "<div class='panel_shell hierarchy_shell'><div class='panel_header'>Hierarchy</div><div class='panel_body hierarchy_body'>") +
        buildHierarchyNodeMarkup(m_hierarchyRoot, 0) +
    "</div>" +
    buildHierarchyContextMenuMarkup() +
    "</div>";
}

std::string EditorUiController::buildHierarchyNodeMarkup(const UiHierarchyNode& node, int depth) const
{
    const bool isSelected = node.id == m_selectedHierarchyNodeId;
    const bool dropBefore = m_dropTargetNodeId == node.id && m_dropMode == HierarchyDropMode::Before;
    const bool dropInside = m_dropTargetNodeId == node.id && m_dropMode == HierarchyDropMode::Inside;
    const bool dropAfter = m_dropTargetNodeId == node.id && m_dropMode == HierarchyDropMode::After;

    std::ostringstream stream;
    stream << "<div class='hierarchy_node depth_" << depth << "'>";
    stream << "<div id='" << makeHierarchyDropElementId(node.id, HierarchyDropMode::Before) << "' class='hierarchy_drop_zone";
    if (dropBefore)
        stream << " drop_active";
    stream << "'></div>";
    stream << "<div id='" << makeHierarchyNodeElementId(node.id) << "' class='hierarchy_row";
    if (isSelected)
        stream << " selected";
    if (dropInside)
        stream << " drop_active";
    if (m_dragPayloadKind == DragPayloadKind::Node && m_draggedHierarchyNodeId == node.id)
        stream << " dragging";
    stream << "'>";
    stream << "<div class='hierarchy_label'>" << node.label << "</div>";
    stream << "<div class='hierarchy_meta'>&lt;" << node.tagName << "&gt;</div>";
    stream << "</div>";

    if (!node.children.empty())
    {
        stream << "<div class='hierarchy_children'>";
        for (const UiHierarchyNode& child : node.children)
            stream << buildHierarchyNodeMarkup(child, depth + 1);
        stream << "</div>";
    }

    stream << "<div id='" << makeHierarchyDropElementId(node.id, HierarchyDropMode::After) << "' class='hierarchy_drop_zone";
    if (dropAfter)
        stream << " drop_active";
    stream << "'></div>";
    stream << "</div>";
    return stream.str();
}

std::string EditorUiController::buildWidgetCatalogMarkup() const
{
    std::ostringstream stream;
    stream << "<div class='panel_shell'><div class='panel_header'>Elements &amp; Classes</div><div class='panel_body widget_catalog_body'>";
    for (const ui::widget::Widget* widget : ui::widget::getWidgets())
    {
        stream << "<div id='widget_catalog_item_" << widget->key() << "' class='widget_catalog_item'>";
        stream << "<div class='widget_catalog_label'>" << widget->name() << "</div>";
        stream << "<div class='widget_catalog_text'>" << widget->description() << "</div>";
        stream << "</div>";
    }
    stream << "</div></div>";
    return stream.str();
}

std::string EditorUiController::buildHierarchyContextMenuMarkup() const
{
    if (!m_hierarchyContextMenuOpen || m_hierarchyContextNodeId == 0 || m_hierarchyContextNodeId == m_hierarchyRoot.id)
        return "";

    const bool canMoveUpValue = canMoveHierarchyNodeUp(m_hierarchyContextNodeId);
    const bool canMoveDownValue = canMoveHierarchyNodeDown(m_hierarchyContextNodeId);

    std::ostringstream stream;
    stream << "<div class='hierarchy_context_menu' style='left: " << m_hierarchyContextMenuX << "px; top: " << m_hierarchyContextMenuY << "px;'>";
    stream << "<div id='hierarchy_context_move_up' class='hierarchy_context_item";
    if (!canMoveUpValue)
        stream << " disabled";
    stream << "'>Move up</div>";
    stream << "<div id='hierarchy_context_move_down' class='hierarchy_context_item";
    if (!canMoveDownValue)
        stream << " disabled";
    stream << "'>Move down</div>";
    stream << "<div id='hierarchy_context_delete' class='hierarchy_context_item danger'>Delete</div>";
    stream << "</div>";
    return stream.str();
}

std::string EditorUiController::buildPreviewDocumentSourceFromHierarchy() const
{
    return buildExportDocumentSourceFromHierarchy();
}

std::string EditorUiController::buildPreviewNodeMarkup(const UiHierarchyNode& node, int depth) const
{
    return buildExportNodeMarkup(node, depth);
}

std::string EditorUiController::buildExportDocumentSourceFromHierarchy() const
{
    std::ostringstream stream;
    stream << "<rml>\n";
    stream << "<head>\n";
    stream << "    <style>\n";
    stream << "        body { margin: 0px; width: 100%; height: 100%; padding: 16px; box-sizing: border-box; font-family: LatoLatin; background-color: #f8f4ea; color: #172028; overflow: auto; }\n";
    stream << "        button { font-family: LatoLatin; }\n";
    for (const ui::widget::Widget* widget : ui::widget::getWidgets())
        stream << "        " << widget->buildStyleRules();
    stream << "    </style>\n";
    stream << "</head>\n";
    stream << "<body>\n";
    for (const UiHierarchyNode& child : m_hierarchyRoot.children)
        stream << buildExportNodeMarkup(child, 1);
    stream << "</body>\n";
    stream << "</rml>\n";
    return stream.str();
}

std::string EditorUiController::buildRenderDocumentSourceFromHierarchy() const
{
    return buildExportDocumentSourceFromHierarchy();
}

std::string EditorUiController::buildExportNodeMarkup(const UiHierarchyNode& node, int depth) const
{
    const ui::widget::Widget* widget = ui::widget::findWidgetByKey(node.elementKey);
    if (widget == nullptr)
        return "";

    std::vector<std::string> childMarkup;
    childMarkup.reserve(node.children.size());
    for (const UiHierarchyNode& child : node.children)
        childMarkup.push_back(buildExportNodeMarkup(child, depth + 1));

    return widget->buildMarkup(node.label, node.properties, childMarkup, depth, ui::widget::DocumentBuildMode::Export);
}

bool EditorUiController::rebuildHierarchyFromCurrentPreviewDocument()
{
    if (m_previewDocument == nullptr)
        return false;

    const Rml::Element* bodyElement = findFirstElementByTagName(m_previewDocument, "body");

    if (bodyElement == nullptr)
        return false;

    resetHierarchyModel();

    for (int childIndex = 0; childIndex < bodyElement->GetNumChildren(); ++childIndex)
    {
        const Rml::Element* child = bodyElement->GetChild(childIndex);
        if (child == nullptr)
            continue;

        appendHierarchyNodesFromElement(m_hierarchyRoot, child);
    }

    m_selectedHierarchyNodeId = m_hierarchyRoot.id;
    return true;
}

bool EditorUiController::appendHierarchyNodesFromElement(UiHierarchyNode& parentNode, const Rml::Element* element)
{
    if (element == nullptr)
        return false;

    if (const ui::widget::Widget* widget = ui::widget::findWidgetByElement(*element))
    {
        UiHierarchyNode node = makeHierarchyNodeForElementKey(widget->key());
        node.label = widget->labelFromElement(*element);
        node.tagName = widget->tagName();
        node.properties = widget->capturePropertiesFromElement(*element);

        if (widget->canHaveEditorChildren())
        {
            for (int childIndex = 0; childIndex < element->GetNumChildren(); ++childIndex)
            {
                const Rml::Element* child = element->GetChild(childIndex);
                if (child != nullptr)
                    appendHierarchyNodesFromElement(node, child);
            }
        }

        parentNode.children.push_back(std::move(node));
        return true;
    }

    for (int childIndex = 0; childIndex < element->GetNumChildren(); ++childIndex)
    {
        const Rml::Element* child = element->GetChild(childIndex);
        if (child != nullptr)
            appendHierarchyNodesFromElement(parentNode, child);
    }
    return false;
}

std::string EditorUiController::makeHierarchyNodeElementId(int nodeId)
{
    return "hierarchy_node_" + std::to_string(nodeId);
}

std::string EditorUiController::makeHierarchyDropElementId(int nodeId, HierarchyDropMode dropMode)
{
    const char* suffix = "inside";
    if (dropMode == HierarchyDropMode::Before)
        suffix = "before";
    else if (dropMode == HierarchyDropMode::After)
        suffix = "after";
    return "hierarchy_drop_" + std::to_string(nodeId) + "_" + suffix;
}

std::string EditorUiController::makeInspectorFieldElementId(int nodeId, const std::string& fieldKey)
{
    return "inspector_field_" + std::to_string(nodeId) + "__" + fieldKey;
}

std::optional<int> EditorUiController::parseHierarchyNodeId(const Rml::String& elementId)
{
    const std::string value = elementId;
    const std::string prefix = "hierarchy_node_";
    if (!startsWith(value, prefix))
        return std::nullopt;

    return std::stoi(value.substr(prefix.size()));
}

std::optional<std::pair<int, EditorUiController::HierarchyDropMode>> EditorUiController::parseHierarchyDropId(const Rml::String& elementId)
{
    const std::string value = elementId;
    const std::string prefix = "hierarchy_drop_";
    if (!startsWith(value, prefix))
        return std::nullopt;

    const std::size_t separator = value.find('_', prefix.size());
    if (separator == std::string::npos)
        return std::nullopt;

    const int nodeId = std::stoi(value.substr(prefix.size(), separator - prefix.size()));
    const std::string suffix = value.substr(separator + 1);
    if (suffix == "before")
        return std::make_pair(nodeId, HierarchyDropMode::Before);
    if (suffix == "after")
        return std::make_pair(nodeId, HierarchyDropMode::After);
    if (suffix == "inside")
        return std::make_pair(nodeId, HierarchyDropMode::Inside);
    return std::nullopt;
}

std::optional<std::pair<int, std::string>> EditorUiController::parseInspectorFieldElementId(const Rml::String& elementId)
{
    const std::string value = elementId;
    const std::string prefix = "inspector_field_";
    if (!startsWith(value, prefix))
        return std::nullopt;

    const std::size_t separator = value.find("__", prefix.size());
    if (separator == std::string::npos)
        return std::nullopt;

    const int nodeId = std::stoi(value.substr(prefix.size(), separator - prefix.size()));
    return std::make_pair(nodeId, value.substr(separator + 2));
}

EditorUiController::UiHierarchyNode* EditorUiController::findHierarchyNodeById(int nodeId)
{
    if (m_hierarchyRoot.id == nodeId)
        return &m_hierarchyRoot;

    std::function<UiHierarchyNode*(UiHierarchyNode&)> findInChildren = [&](UiHierarchyNode& node) -> UiHierarchyNode*
    {
        for (UiHierarchyNode& child : node.children)
        {
            if (child.id == nodeId)
                return &child;
            if (UiHierarchyNode* foundChild = findInChildren(child))
                return foundChild;
        }
        return nullptr;
    };

    return findInChildren(m_hierarchyRoot);
}

const EditorUiController::UiHierarchyNode* EditorUiController::findHierarchyNodeById(int nodeId) const
{
    return const_cast<EditorUiController*>(this)->findHierarchyNodeById(nodeId);
}

EditorUiController::UiHierarchyNode* EditorUiController::findParentNodeOf(int nodeId)
{
    std::function<UiHierarchyNode*(UiHierarchyNode&)> findParent = [&](UiHierarchyNode& node) -> UiHierarchyNode*
    {
        for (UiHierarchyNode& child : node.children)
        {
            if (child.id == nodeId)
                return &node;
            if (UiHierarchyNode* parent = findParent(child))
                return parent;
        }
        return nullptr;
    };

    return findParent(m_hierarchyRoot);
}

std::optional<ui::widget::InspectorField> EditorUiController::findInspectorFieldDefinition(const UiHierarchyNode& node, const std::string& fieldKey) const
{
    const ui::widget::Widget* widget = ui::widget::findWidgetByKey(node.elementKey);
    if (widget == nullptr)
        return std::nullopt;

    const ui::widget::InspectorModel model = widget->buildInspectorModel(node.label, node.properties);
    for (const ui::widget::InspectorField& field : model.fields)
    {
        if (field.key == fieldKey)
            return field;
    }

    for (const ui::widget::InspectorGroup& group : model.childGroups)
    {
        for (const ui::widget::InspectorField& field : group.fields)
        {
            if (field.key == fieldKey)
                return field;
        }
    }

    return std::nullopt;
}

bool EditorUiController::applyInspectorFieldValue(int nodeId, const std::string& fieldKey, const std::string& value)
{
    UiHierarchyNode* node = findHierarchyNodeById(nodeId);
    if (node == nullptr)
        return false;

    const ui::widget::Widget* widget = ui::widget::findWidgetByKey(node->elementKey);
    if (widget == nullptr)
        return false;

    const std::optional<ui::widget::InspectorField> field = findInspectorFieldDefinition(*node, fieldKey);
    if (!field.has_value())
        return false;

    if (const std::optional<std::string> primaryTextField = widget->primaryTextPropertyKey())
    {
        if (*primaryTextField == fieldKey)
        {
            if (node->label == value)
                return false;
            node->label = value;
            if (value == field->defaultValue)
                node->properties.erase(fieldKey);
            else
                node->properties[fieldKey] = value;
            return true;
        }
    }

    if (value == field->defaultValue)
    {
        const auto propertyIt = node->properties.find(fieldKey);
        if (propertyIt == node->properties.end())
            return false;
        node->properties.erase(propertyIt);
        return true;
    }

    const auto propertyIt = node->properties.find(fieldKey);
    if (propertyIt != node->properties.end() && propertyIt->second == value)
        return false;

    node->properties[fieldKey] = value;
    return true;
}

bool EditorUiController::removeHierarchyNodeById(int nodeId, UiHierarchyNode* removedNode)
{
    UiHierarchyNode* parent = findParentNodeOf(nodeId);
    if (parent == nullptr)
        return false;

    for (auto childIt = parent->children.begin(); childIt != parent->children.end(); ++childIt)
    {
        if (childIt->id == nodeId)
        {
            if (removedNode != nullptr)
                *removedNode = std::move(*childIt);
            parent->children.erase(childIt);
            return true;
        }
    }

    return false;
}

bool EditorUiController::insertHierarchyNodeBefore(int targetNodeId, UiHierarchyNode node)
{
    UiHierarchyNode* parent = findParentNodeOf(targetNodeId);
    if (parent == nullptr)
        return false;

    for (auto childIt = parent->children.begin(); childIt != parent->children.end(); ++childIt)
    {
        if (childIt->id == targetNodeId)
        {
            parent->children.insert(childIt, std::move(node));
            return true;
        }
    }

    return false;
}

bool EditorUiController::insertHierarchyNodeAfter(int targetNodeId, UiHierarchyNode node)
{
    UiHierarchyNode* parent = findParentNodeOf(targetNodeId);
    if (parent == nullptr)
        return false;

    for (auto childIt = parent->children.begin(); childIt != parent->children.end(); ++childIt)
    {
        if (childIt->id == targetNodeId)
        {
            parent->children.insert(std::next(childIt), std::move(node));
            return true;
        }
    }

    return false;
}

bool EditorUiController::insertHierarchyNodeInside(int targetNodeId, UiHierarchyNode node)
{
    UiHierarchyNode* targetNode = findHierarchyNodeById(targetNodeId);
    if (targetNode == nullptr)
        return false;

    if (targetNode->id != m_hierarchyRoot.id)
    {
        const ui::widget::Widget* widget = ui::widget::findWidgetByKey(targetNode->elementKey);
        if (widget == nullptr || !widget->canHaveEditorChildren())
            return false;
    }

    targetNode->children.push_back(std::move(node));
    return true;
}

bool EditorUiController::isHierarchyNodeDescendantOf(int nodeId, int ancestorNodeId) const
{
    const UiHierarchyNode* ancestorNode = findHierarchyNodeById(ancestorNodeId);
    if (ancestorNode == nullptr)
        return false;

    std::function<bool(const UiHierarchyNode&)> search = [&](const UiHierarchyNode& node) -> bool
    {
        if (node.id == nodeId)
            return true;
        for (const UiHierarchyNode& child : node.children)
        {
            if (search(child))
                return true;
        }
        return false;
    };

    for (const UiHierarchyNode& child : ancestorNode->children)
    {
        if (search(child))
            return true;
    }
    return false;
}

bool EditorUiController::moveHierarchyNodeUp(int nodeId)
{
    UiHierarchyNode* parent = findParentNodeOf(nodeId);
    if (parent == nullptr || parent->children.size() <= 1)
        return false;

    for (std::size_t childIndex = 1; childIndex < parent->children.size(); ++childIndex)
    {
        if (parent->children[childIndex].id == nodeId)
        {
            std::swap(parent->children[childIndex - 1], parent->children[childIndex]);
            return true;
        }
    }

    return false;
}

bool EditorUiController::moveHierarchyNodeDown(int nodeId)
{
    UiHierarchyNode* parent = findParentNodeOf(nodeId);
    if (parent == nullptr || parent->children.size() <= 1)
        return false;

    for (std::size_t childIndex = 0; childIndex + 1 < parent->children.size(); ++childIndex)
    {
        if (parent->children[childIndex].id == nodeId)
        {
            std::swap(parent->children[childIndex], parent->children[childIndex + 1]);
            return true;
        }
    }

    return false;
}

bool EditorUiController::canMoveHierarchyNodeUp(int nodeId) const
{
    UiHierarchyNode* parent = const_cast<EditorUiController*>(this)->findParentNodeOf(nodeId);
    if (parent == nullptr || parent->children.size() <= 1)
        return false;

    for (std::size_t childIndex = 1; childIndex < parent->children.size(); ++childIndex)
    {
        if (parent->children[childIndex].id == nodeId)
            return true;
    }

    return false;
}

bool EditorUiController::canMoveHierarchyNodeDown(int nodeId) const
{
    UiHierarchyNode* parent = const_cast<EditorUiController*>(this)->findParentNodeOf(nodeId);
    if (parent == nullptr || parent->children.size() <= 1)
        return false;

    for (std::size_t childIndex = 0; childIndex + 1 < parent->children.size(); ++childIndex)
    {
        if (parent->children[childIndex].id == nodeId)
            return true;
    }

    return false;
}

void EditorUiController::closeHierarchyContextMenu()
{
    m_hierarchyContextMenuOpen = false;
    m_hierarchyContextNodeId = 0;
}

void EditorUiController::toggleInspectorGroup(const std::string& groupId)
{
    const auto collapsedGroup = m_collapsedInspectorGroups.find(groupId);
    if (collapsedGroup != m_collapsedInspectorGroups.end())
        m_collapsedInspectorGroups.erase(collapsedGroup);
    else
        m_collapsedInspectorGroups.insert(groupId);
}

bool EditorUiController::isInspectorGroupCollapsed(const std::string& groupId) const
{
    return m_collapsedInspectorGroups.find(groupId) != m_collapsedInspectorGroups.end();
}

const EditorUiController::UiHierarchyNode* EditorUiController::findSelectedHierarchyNode() const
{
    return findHierarchyNodeById(m_selectedHierarchyNodeId);
}

EditorUiController::UiHierarchyNode EditorUiController::makeHierarchyNodeForElementKey(const std::string& elementKey)
{
    UiHierarchyNode node;
    node.id = m_nextHierarchyNodeId++;
    node.elementKey = elementKey;
    if (const ui::widget::Widget* widget = ui::widget::findWidgetByKey(elementKey))
    {
        node.tagName = widget->tagName();
        node.label = widget->defaultLabel();
    }
    else
    {
        node.tagName = "div";
        node.label = elementKey;
    }
    return node;
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
    if (!m_previewRenderer.loadFromMemory(m_previewDocumentSource, sourceUrl))
    {
        m_previewDocument = nullptr;
        return;
    }

    m_previewDocument = m_previewRenderer.document();
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

    m_previewRenderer.unload();

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
    m_previewDocument->SetProperty("left", pixels(m_previewPageRect.x));
    m_previewDocument->SetProperty("top", pixels(m_previewPageRect.y));
    m_previewDocument->SetProperty("width", pixels(m_previewPageRect.width));
    m_previewDocument->SetProperty("height", pixels(m_previewPageRect.height));
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
        bodyElement->SetProperty("left", pixels(0));
        bodyElement->SetProperty("top", pixels(0));
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
    const std::string previousSource = m_previewDocumentSource;
    const std::string previousPath = m_previewDocumentPath;

    if (!m_previewRenderer.loadFromFile(filePath))
        return false;

    m_previewDocumentSource = m_previewRenderer.source();
    m_previewDocumentPath = m_previewRenderer.sourcePath();
    m_previewDocument = m_previewRenderer.document();
    m_previewDrivenByHierarchy = false;
    m_previewRefreshPending = false;

    if (m_previewDocument != nullptr)
    {
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

    if (m_uiBuilderEnabled && m_previewDocument == nullptr)
    {
        m_previewDocumentSource = previousSource;
        m_previewDocumentPath = previousPath;
        reloadPreviewDocument();
        return false;
    }

    if (m_uiBuilderEnabled && rebuildHierarchyFromCurrentPreviewDocument())
    {
        m_previewDrivenByHierarchy = true;
        m_hierarchyRefreshPending = false;
        m_previewRefreshPending = false;
        refreshHierarchyPresentation();
        refreshPreviewFromHierarchy();
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

    stream << (m_previewDrivenByHierarchy ? buildExportDocumentSourceFromHierarchy() : m_previewDocumentSource);
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
    m_document->AddEventListener(Rml::EventId::Change, this);
    m_document->AddEventListener(Rml::EventId::Blur, this, true);
    m_document->AddEventListener(Rml::EventId::Dragstart, this);
    m_document->AddEventListener(Rml::EventId::Dragover, this);
    m_document->AddEventListener(Rml::EventId::Dragdrop, this);
    m_document->AddEventListener(Rml::EventId::Dragend, this);
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
        m_document->RemoveEventListener(Rml::EventId::Change, this);
        m_document->RemoveEventListener(Rml::EventId::Blur, this, true);
        m_document->RemoveEventListener(Rml::EventId::Dragstart, this);
        m_document->RemoveEventListener(Rml::EventId::Dragover, this);
        m_document->RemoveEventListener(Rml::EventId::Dragdrop, this);
        m_document->RemoveEventListener(Rml::EventId::Dragend, this);
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
    if (maxValue < minValue)
        maxValue = minValue;
    return std::max(minValue, std::min(value, maxValue));
}