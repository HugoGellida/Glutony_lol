#include "UiBuilderController.hpp"
#include "EditorUiDocuments.hpp"
#include "EditorUiPreviewDocument.hpp"

#include <RmlUi/Core/Elements/ElementFormControl.h>

#include <common/UI/Container.hpp>
#include <common/UI/MarkupBlock.hpp>
#include <common/UI/Panel.hpp>
#include <common/UI/PanelHeader.hpp>
#include <common/UI/Placeholder.hpp>
#include <common/UI/SplitContainer.hpp>
#include <common/UI/TextBlock.hpp>
#include <common/UI/ToolbarButton.hpp>
#include <common/UI/ToolbarGroup.hpp>
#include <common/UI/UiBuilderPanelMarkupFactory.hpp>


#include <common/ui/widget/WidgetRegistry.hpp>

#include <common/platform/NativeFileDialog.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

using editor_ui::BuilderHeaderHeight;
using editor_ui::clampInt;
using editor_ui::MinBottomHeight;
using editor_ui::MinCenterWidth;
using editor_ui::MinColumnWidth;
using editor_ui::MinLeftSectionHeight;
using editor_ui::MinPreviewDocumentHeight;
using editor_ui::MinPreviewDocumentWidth;
using editor_ui::MinViewportHeight;
using editor_ui::pixels;
using editor_ui::SplitterThickness;

namespace
{
bool localStartsWith(const std::string& value, const std::string& prefix)
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

void applyPreviewShell(UIBinder& binder, Rml::Element* mountPoint)
{
    if (mountPoint == nullptr)
        return;

    UI::Container shell(0, 0, UI::VERTICAL);
    shell.addClassName("preview_shell");

    UI::Container toolbar(0, 0, UI::HORIZONTAL);
    toolbar.addClassName("preview_toolbar");

    UI::ToolbarGroup zoomGroup(0, 0);
    UI::ToolbarButton zoomOut(0, 0, "-");
    zoomOut.setDomIdOverride("preview_zoom_out");
    UI::TextBlock zoomLabel(0, 0, "100%");
    zoomLabel.setDomIdOverride("preview_zoom_label");
    zoomLabel.addClassName("preview_zoom_label");
    UI::ToolbarButton zoomIn(0, 0, "+");
    zoomIn.setDomIdOverride("preview_zoom_in");
    zoomGroup.addChild(&zoomOut);
    zoomGroup.addChild(&zoomLabel);
    zoomGroup.addChild(&zoomIn);

    UI::ToolbarGroup modeGroup(0, 0);
    UI::ToolbarButton modeToggle(0, 0, "Render");
    modeToggle.setDomIdOverride("preview_mode_toggle");
    UI::ToolbarButton zoomFit(0, 0, "Fit");
    zoomFit.setDomIdOverride("preview_zoom_fit");
    modeGroup.addChild(&modeToggle);
    modeGroup.addChild(&zoomFit);

    toolbar.addChild(&zoomGroup);
    toolbar.addChild(&modeGroup);

    UI::Container canvas(0, 0, UI::VERTICAL);
    canvas.setDomIdOverride("preview_canvas");
    canvas.addClassName("preview_canvas");

    UI::Container window(0, 0, UI::VERTICAL);
    window.setDomIdOverride("preview_window");
    window.addClassName("preview_window");

    UI::Container host(0, 0, UI::VERTICAL);
    host.setDomIdOverride("preview_host");
    host.addClassName("preview_host");

    UI::Container resizeRight(0, 0, UI::VERTICAL);
    resizeRight.setDomIdOverride("preview_resize_right");
    resizeRight.addClassName("preview_resize_handle");
    resizeRight.addClassName("preview_resize_right");

    UI::Container resizeBottom(0, 0, UI::VERTICAL);
    resizeBottom.setDomIdOverride("preview_resize_bottom");
    resizeBottom.addClassName("preview_resize_handle");
    resizeBottom.addClassName("preview_resize_bottom");

    UI::Container resizeCorner(0, 0, UI::VERTICAL);
    resizeCorner.setDomIdOverride("preview_resize_corner");
    resizeCorner.addClassName("preview_resize_handle");
    resizeCorner.addClassName("preview_resize_corner");

    window.addChild(&host);
    window.addChild(&resizeRight);
    window.addChild(&resizeBottom);
    window.addChild(&resizeCorner);
    canvas.addChild(&window);

    shell.addChild(&toolbar);
    shell.addChild(&canvas);
    binder.apply(mountPoint, shell);
}

void applyUiBuilderLeftColumnShell(UIBinder& binder, Rml::Element* mountPoint)
{
    if (mountPoint == nullptr)
        return;

    UI::SplitContainer split(0, 0, UI::VERTICAL);
    split.setSplitterDomIdOverride("left_horizontal_splitter");
    split.addSplitterClassName("splitter_horizontal_nested");

    UI::Container topHost(0, 0, UI::VERTICAL);
    topHost.setDomIdOverride("left_top_panel");
    topHost.addClassName("panel");
    topHost.addClassName("panel_nested");

    UI::Panel topPanel(0, 0);
    UI::PanelHeader topHeader(0, 0, "Hierarchy");
    UI::Placeholder topPlaceholder(0, 0);
    topPlaceholder.setTitle("Document tree");
    topPlaceholder.setDescription("The live hierarchy of the previewed document will appear here.");
    topPanel.addChild(&topHeader);
    topPanel.addChild(&topPlaceholder);
    topHost.addChild(&topPanel);

    UI::Container bottomHost(0, 0, UI::VERTICAL);
    bottomHost.setDomIdOverride("left_bottom_panel");
    bottomHost.addClassName("panel");
    bottomHost.addClassName("panel_nested");

    UI::Panel bottomPanel(0, 0);
    UI::PanelHeader bottomHeader(0, 0, "Widgets & Templates");
    UI::Placeholder bottomPlaceholder(0, 0);
    bottomPlaceholder.setTitle("Widget catalog");
    bottomPlaceholder.setDescription("Reusable widgets and templates will be listed here for drag and drop.");
    bottomPanel.addChild(&bottomHeader);
    bottomPanel.addChild(&bottomPlaceholder);
    bottomHost.addChild(&bottomPanel);

    split.addChild(&topHost);
    split.addChild(&bottomHost);
    binder.apply(mountPoint, split);
}
}

bool UiBuilderController::initialize(Rml::Context* context)
{
    if (context == nullptr)
        return false;

    m_context = context;
    const Rml::Vector2i contextDimensions = m_context->GetDimensions();
    m_windowWidth = std::max(contextDimensions.x, 1);
    m_windowHeight = std::max(contextDimensions.y, 1);
    m_layoutManager.setWindowSize(m_windowWidth, m_windowHeight);
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
    m_layoutManager.applyLayout();
    reloadPreviewDocument();
    return true;
}

void UiBuilderController::shutdown()
{
    unloadPreviewDocument();
    m_previewRenderer.shutdown();
    m_leftShellBinder.clear();
    m_previewShellBinder.clear();
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
    m_dragTarget = UI::UiBuilderLayoutManager::DragTarget::None;
    m_isFileMenuOpen = false;
    m_isWindowMenuOpen = false;
    m_layoutManager.setElements({});
    m_layoutManager.setPreviewDocument(nullptr);
}

void UiBuilderController::activate()
{
    if (m_document != nullptr)
        m_document->Show();
}

void UiBuilderController::deactivate()
{
    if (m_document != nullptr)
        m_document->Hide();
}

void UiBuilderController::setModeChangeCallback(const std::function<void(editor_ui::EditorMode)>& callback)
{
    m_modeChangeCallback = callback;
}

void UiBuilderController::syncToWindow(int width, int height)
{
    m_windowWidth = std::max(width, 1);
    m_windowHeight = std::max(height, 1);
    m_layoutManager.setWindowSize(m_windowWidth, m_windowHeight);
}

void UiBuilderController::setUiBuilderShowStylePanel(bool showStylePanel)
{
    if (m_uiBuilderShowStylePanel == showStylePanel)
        return;

    m_uiBuilderShowStylePanel = showStylePanel;
    refreshModePresentation();
}

void UiBuilderController::setShowStylePanel(bool showStylePanel)
{
    setUiBuilderShowStylePanel(showStylePanel);
}

void UiBuilderController::update()
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

    m_layoutManager.applyLayout();
    m_context->Update();
    m_layoutManager.refreshCachedRects();
    m_layoutManager.updatePreviewDocumentPlacement(m_document);
}

void UiBuilderController::render()
{
    if (m_context != nullptr)
        m_context->Render();
}

UiRect UiBuilderController::getViewportRect() const
{
    return m_layoutManager.viewportRect();
}

bool UiBuilderController::isViewportHovered(double mouseX, double mouseY) const
{
    return m_layoutManager.viewportRect().contains(mouseX, mouseY);
}

bool UiBuilderController::isDragging() const
{
    return m_dragTarget != UI::UiBuilderLayoutManager::DragTarget::None;
}

void UiBuilderController::ProcessEvent(Rml::Event& event)
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
        [&](const Rml::String& candidateId) { return UI::UiBuilderIdCodec::parseHierarchyNodeId(candidateId).has_value(); });
    const Rml::String hierarchyDropElementId = findAncestorElementId(
        [&](const Rml::String& candidateId) { return UI::UiBuilderIdCodec::parseHierarchyDropId(candidateId).has_value(); });
    const Rml::String widgetCatalogElementId = findAncestorElementId(
        [&](const Rml::String& candidateId) { return localStartsWith(std::string(candidateId), "widget_catalog_item_"); });
    const Rml::String hierarchyContextItemElementId = findAncestorElementId(
        [&](const Rml::String& candidateId) {
            return candidateId == "hierarchy_context_move_up" ||
                candidateId == "hierarchy_context_move_down" ||
                candidateId == "hierarchy_context_delete";
        });
    const Rml::String inspectorToggleElementId = findAncestorElementId(
        [&](const Rml::String& candidateId) { return localStartsWith(std::string(candidateId), "inspector_group_toggle_"); });

    if (eventId == Rml::EventId::Change || eventId == Rml::EventId::Blur)
    {
        const auto inspectorField = UI::UiBuilderIdCodec::parseInspectorFieldElementId(elementId);
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

        if (m_hierarchyModel.applyInspectorFieldValue(inspectorField->first, inspectorField->second, formControl->GetValue().c_str()))
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

        if (elementId == "builder_menu_back_to_editor")
        {
            m_isFileMenuOpen = false;
            m_isWindowMenuOpen = false;
            refreshBuilderMenuState();
            if (m_modeChangeCallback)
                m_modeChangeCallback(editor_ui::EditorMode::SceneEditor);
            event.StopPropagation();
            return;
        }

        if (elementId == "hierarchy_context_move_up")
        {
            const bool didMove = m_hierarchyModel.moveNodeUp(m_hierarchyContextNodeId);
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
            const bool didMove = m_hierarchyModel.moveNodeDown(m_hierarchyContextNodeId);
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
            const bool didDelete = m_hierarchyContextNodeId != m_hierarchyModel.root().id && m_hierarchyModel.removeNodeById(m_hierarchyContextNodeId, nullptr);
            if (didDelete)
                m_hierarchyModel.setSelectedNodeId(m_hierarchyModel.root().id);
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

        if (const std::optional<int> hierarchyNodeId = UI::UiBuilderIdCodec::parseHierarchyNodeId(hierarchyNodeElementId))
        {
            m_hierarchyModel.setSelectedNodeId(*hierarchyNodeId);
            closeHierarchyContextMenu();
            requestHierarchyRefresh();
            event.StopPropagation();
            return;
        }

        if (elementId == "builder_menu_new")
        {
            m_hierarchyModel.reset();
            m_dragPayloadKind = DragPayloadKind::None;
            m_draggedWidgetKey.clear();
            m_draggedHierarchyNodeId = 0;
            m_dropTargetNodeId = 0;
            m_dropMode = HierarchyDropMode::None;
            requestHierarchyRefresh();
            requestPreviewRefreshFromHierarchy();
            m_previewDocumentPath.clear();
            m_layoutManager.setPreviewDocumentSize(1280, 720);
            m_layoutManager.setPreviewZoom(1.0f);
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
            setPreviewZoom(m_layoutManager.previewZoom() / 1.1f);
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
            setPreviewZoom(m_layoutManager.previewZoom() * 1.1f);
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
        if (localStartsWith(targetId, "widget_catalog_item_"))
        {
            m_dragPayloadKind = DragPayloadKind::Widget;
            m_draggedWidgetKey = targetId.substr(std::string("widget_catalog_item_").size());
            m_draggedHierarchyNodeId = 0;
            m_dropTargetNodeId = 0;
            m_dropMode = HierarchyDropMode::None;
            event.StopPropagation();
            return;
        }

        if (const std::optional<int> hierarchyNodeId = UI::UiBuilderIdCodec::parseHierarchyNodeId(hierarchyNodeElementId))
        {
            if (*hierarchyNodeId != m_hierarchyModel.root().id)
            {
                m_dragPayloadKind = DragPayloadKind::Node;
                m_draggedHierarchyNodeId = *hierarchyNodeId;
                m_draggedWidgetKey.clear();
                m_dropTargetNodeId = 0;
                m_dropMode = HierarchyDropMode::None;
                m_hierarchyModel.setSelectedNodeId(*hierarchyNodeId);
                event.StopPropagation();
                return;
            }
        }
    }

    if (eventId == Rml::EventId::Dragover)
    {
        if (m_dragPayloadKind == DragPayloadKind::None)
            return;

        if (const std::optional<std::pair<int, HierarchyDropMode>> dropTarget = UI::UiBuilderIdCodec::parseHierarchyDropId(hierarchyDropElementId))
        {
            m_dropTargetNodeId = dropTarget->first;
            m_dropMode = dropTarget->second;
            event.StopPropagation();
            return;
        }

        if (const std::optional<int> hierarchyNodeId = UI::UiBuilderIdCodec::parseHierarchyNodeId(hierarchyNodeElementId))
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

        std::optional<std::pair<int, HierarchyDropMode>> dropTarget = UI::UiBuilderIdCodec::parseHierarchyDropId(hierarchyDropElementId);
        if (!dropTarget.has_value())
        {
            if (const std::optional<int> hierarchyNodeId = UI::UiBuilderIdCodec::parseHierarchyNodeId(hierarchyNodeElementId))
                dropTarget = std::make_pair(*hierarchyNodeId, HierarchyDropMode::Inside);
        }

        if (!dropTarget.has_value())
            return;

        const int targetNodeId = dropTarget->first;
        const HierarchyDropMode dropMode = dropTarget->second;
        bool didApplyDrop = false;

        if (m_dragPayloadKind == DragPayloadKind::Widget)
        {
            UI::UiBuilderHierarchyModel::Node newNode = m_hierarchyModel.makeNodeForElementKey(m_draggedWidgetKey);
            const int newNodeId = newNode.id;
            if (dropMode == HierarchyDropMode::Before)
                didApplyDrop = m_hierarchyModel.insertNodeBefore(targetNodeId, std::move(newNode));
            else if (dropMode == HierarchyDropMode::After)
                didApplyDrop = m_hierarchyModel.insertNodeAfter(targetNodeId, std::move(newNode));
            else
                didApplyDrop = m_hierarchyModel.insertNodeInside(targetNodeId, std::move(newNode));

            if (didApplyDrop)
                m_hierarchyModel.setSelectedNodeId(newNodeId);
        }
        else if (m_dragPayloadKind == DragPayloadKind::Node)
        {
            if (m_draggedHierarchyNodeId != targetNodeId && !m_hierarchyModel.isNodeDescendantOf(targetNodeId, m_draggedHierarchyNodeId))
            {
                UI::UiBuilderHierarchyModel::Node movedNode;
                if (m_hierarchyModel.removeNodeById(m_draggedHierarchyNodeId, &movedNode))
                {
                    if (dropMode == HierarchyDropMode::Before)
                        didApplyDrop = m_hierarchyModel.insertNodeBefore(targetNodeId, std::move(movedNode));
                    else if (dropMode == HierarchyDropMode::After)
                        didApplyDrop = m_hierarchyModel.insertNodeAfter(targetNodeId, std::move(movedNode));
                    else
                        didApplyDrop = m_hierarchyModel.insertNodeInside(targetNodeId, std::move(movedNode));

                    if (didApplyDrop)
                        m_hierarchyModel.setSelectedNodeId(m_draggedHierarchyNodeId);
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
        if (!m_layoutManager.previewPageRect().isValid())
            return;

        if (!m_layoutManager.previewPageRect().contains(mouseX, mouseY))
            return;

        const float wheelDelta = event.GetParameter<float>("wheel_delta_y", 0.0f);
        if (std::abs(wheelDelta) < 0.001f)
            return;

        const float zoomFactor = wheelDelta < 0.0f ? 1.1f : (1.0f / 1.1f);
        setPreviewZoom(m_layoutManager.previewZoom() * zoomFactor);
        event.StopPropagation();
        return;
    }

    if (eventId == Rml::EventId::Mousedown)
    {
        const int mouseButton = event.GetParameter<int>("button", 0);
        if (mouseButton == 1)
        {
            if (const std::optional<int> hierarchyNodeId = UI::UiBuilderIdCodec::parseHierarchyNodeId(hierarchyNodeElementId))
            {
                if (*hierarchyNodeId != m_hierarchyModel.root().id && m_leftTopPanel != nullptr)
                {
                    m_hierarchyModel.setSelectedNodeId(*hierarchyNodeId);
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

        const UI::UiBuilderLayoutManager::DragTarget previewResizeTarget = m_layoutManager.getPreviewResizeTargetAt(mouseX, mouseY);
        if (previewResizeTarget == UI::UiBuilderLayoutManager::DragTarget::PreviewResizeRight ||
            previewResizeTarget == UI::UiBuilderLayoutManager::DragTarget::PreviewResizeBottom ||
            previewResizeTarget == UI::UiBuilderLayoutManager::DragTarget::PreviewResizeCorner)
        {
            m_dragTarget = previewResizeTarget;
            event.StopPropagation();
            return;
        }

        if (elementId == "left_splitter")
        {
            m_dragTarget = UI::UiBuilderLayoutManager::DragTarget::LeftSplitter;
            event.StopPropagation();
        }
        else if (elementId == "left_horizontal_splitter")
        {
            m_dragTarget = UI::UiBuilderLayoutManager::DragTarget::LeftHorizontalSplitter;
            event.StopPropagation();
        }
        else if (elementId == "right_splitter")
        {
            m_dragTarget = UI::UiBuilderLayoutManager::DragTarget::RightSplitter;
            event.StopPropagation();
        }
        return;
    }

    if (eventId == Rml::EventId::Mouseup)
    {
        m_dragTarget = UI::UiBuilderLayoutManager::DragTarget::None;
        m_layoutManager.updatePreviewCursor(mouseX, mouseY);
        return;
    }

    if (eventId != Rml::EventId::Mousemove)
        return;

    if (m_dragTarget == UI::UiBuilderLayoutManager::DragTarget::None)
    {
        m_layoutManager.updatePreviewCursor(mouseX, mouseY);
        return;
    }

    if (m_dragTarget == UI::UiBuilderLayoutManager::DragTarget::LeftSplitter || m_dragTarget == UI::UiBuilderLayoutManager::DragTarget::RightSplitter)
    {
        m_layoutManager.dragColumnSplitter(m_dragTarget, mouseX);
        m_layoutManager.applyLayout();
        event.StopPropagation();
        return;
    }

    if (m_dragTarget == UI::UiBuilderLayoutManager::DragTarget::LeftHorizontalSplitter)
    {
        m_layoutManager.dragLeftColumnSplitter(mouseY);
        m_layoutManager.applyLayout();
        event.StopPropagation();
        return;
    }

    if (m_dragTarget == UI::UiBuilderLayoutManager::DragTarget::PreviewResizeRight ||
        m_dragTarget == UI::UiBuilderLayoutManager::DragTarget::PreviewResizeBottom ||
        m_dragTarget == UI::UiBuilderLayoutManager::DragTarget::PreviewResizeCorner)
    {
        if (m_layoutManager.dragPreviewResize(m_dragTarget, mouseX, mouseY))
            m_layoutManager.updatePreviewDocumentPlacement(m_document);
        event.StopPropagation();
        return;
    }

    if (m_dragTarget == UI::UiBuilderLayoutManager::DragTarget::HorizontalSplitter)
        event.StopPropagation();
}

void UiBuilderController::refreshModePresentation()
{
    if (m_leftPanel == nullptr ||
        m_rightPanel == nullptr ||
        m_viewportPanel == nullptr ||
        m_bottomPanel == nullptr ||
        m_builderHeader == nullptr)
        return;

    applyUiBuilderLeftColumnShell(m_leftShellBinder, m_leftPanel);

    m_rightPanel->SetInnerRML(
        UI::UiBuilderPanelMarkupFactory::buildPlaceholderPanelMarkup(
            "Inspector",
            "Inspector panel",
            "The selected UI element will expose its editable properties here.")
    );

    applyPreviewShell(m_previewShellBinder, m_viewportPanel);

    m_leftTopPanel = m_document->GetElementById("left_top_panel");
    m_leftHorizontalSplitter = m_document->GetElementById("left_horizontal_splitter");
    m_leftBottomPanel = m_document->GetElementById("left_bottom_panel");
    m_previewCanvas = m_document->GetElementById("preview_canvas");
    m_previewWindow = m_document->GetElementById("preview_window");
    m_previewHost = m_document->GetElementById("preview_host");
    m_previewZoomLabel = m_document->GetElementById("preview_zoom_label");
    m_previewModeButton = m_document->GetElementById("preview_mode_toggle");
    m_bottomPanel->SetInnerRML("");
    m_layoutManager.setElements({
        m_root,
        m_builderHeader,
        m_leftPanel,
        m_leftSplitter,
        m_centerPanel,
        m_rightSplitter,
        m_rightPanel,
        m_viewportPanel,
        m_horizontalSplitter,
        m_bottomPanel,
        m_leftTopPanel,
        m_leftHorizontalSplitter,
        m_leftBottomPanel,
        m_previewCanvas,
        m_previewWindow,
        m_previewHost,
    });
    refreshBuilderMenuState();
    refreshPreviewZoomLabel();
    refreshPreviewModeButtonLabel();
    refreshHierarchyPresentation();
    m_layoutManager.updatePreviewDocumentPlacement(m_document);
}

void UiBuilderController::refreshBuilderMenuState()
{
    if (m_builderHeader == nullptr ||
        m_builderMenuFileButton == nullptr ||
        m_builderMenuFileDropdown == nullptr ||
        m_builderMenuWindowButton == nullptr ||
        m_builderMenuWindowDropdown == nullptr)
        return;

    m_builderHeader->SetProperty("display", "block");
    m_builderMenuFileButton->SetProperty("display", "block");
    m_builderMenuFileDropdown->SetProperty("display", m_isFileMenuOpen ? "block" : "none");
    m_builderMenuWindowButton->SetProperty("display", "none");
    m_builderMenuWindowDropdown->SetProperty("display", "none");
}

void UiBuilderController::refreshPreviewZoomLabel()
{
    if (m_previewZoomLabel == nullptr)
        return;

    const int zoomPercent = static_cast<int>(std::lround(m_layoutManager.previewZoom() * 100.0f));
    m_previewZoomLabel->SetInnerRML(std::to_string(zoomPercent) + "%");
}

void UiBuilderController::refreshPreviewModeButtonLabel()
{
    if (m_previewModeButton == nullptr)
        return;

    m_previewModeButton->SetInnerRML(
        m_previewDisplayMode == PreviewDisplayMode::Preview ? "Render" : "Preview");
}

void UiBuilderController::refreshHierarchyPresentation()
{
    if (m_leftTopPanel == nullptr || m_leftBottomPanel == nullptr)
        return;

    m_leftTopPanel->SetInnerRML(buildHierarchyMarkup());
    m_leftBottomPanel->SetInnerRML(buildWidgetCatalogMarkup());
    refreshInspectorPresentation();
}

void UiBuilderController::refreshInspectorPresentation()
{
    if (m_rightPanel == nullptr)
        return;

    m_rightPanel->SetInnerRML(buildInspectorPanelMarkup());
}

void UiBuilderController::requestHierarchyRefresh()
{
    m_hierarchyRefreshPending = true;
}

void UiBuilderController::requestPreviewRefreshFromHierarchy()
{
    m_previewDrivenByHierarchy = true;
    m_previewRefreshPending = true;
}

void UiBuilderController::refreshPreviewFromHierarchy()
{
    m_previewDrivenByHierarchy = true;
    m_previewDocumentSource = m_previewDisplayMode == PreviewDisplayMode::Preview
        ? m_documentSerializer.buildPreviewDocumentSourceFromHierarchy(m_hierarchyModel)
        : m_documentSerializer.buildRenderDocumentSourceFromHierarchy(m_hierarchyModel);
    reloadPreviewDocument();
}

std::string UiBuilderController::buildInspectorPanelMarkup() const
{
    return m_inspectorPresenter.buildInspectorPanelMarkup(m_hierarchyModel, m_collapsedInspectorGroups);
}

std::string UiBuilderController::buildHierarchyMarkup() const
{
    return m_hierarchyPresenter.buildHierarchyMarkup(
        m_hierarchyModel,
        m_dragPayloadKind,
        m_draggedHierarchyNodeId,
        m_dropTargetNodeId,
        m_dropMode,
        {m_hierarchyContextMenuOpen, m_hierarchyContextNodeId, m_hierarchyContextMenuX, m_hierarchyContextMenuY});
}

std::string UiBuilderController::buildWidgetCatalogMarkup() const
{
    return m_widgetCatalogPresenter.buildWidgetCatalogMarkup();
}

std::string UiBuilderController::buildPreviewDocumentSourceFromHierarchy() const
{
    return m_documentSerializer.buildPreviewDocumentSourceFromHierarchy(m_hierarchyModel);
}

std::string UiBuilderController::buildExportDocumentSourceFromHierarchy() const
{
    return m_documentSerializer.buildExportDocumentSourceFromHierarchy(m_hierarchyModel);
}

std::string UiBuilderController::buildRenderDocumentSourceFromHierarchy() const
{
    return m_documentSerializer.buildRenderDocumentSourceFromHierarchy(m_hierarchyModel);
}

void UiBuilderController::closeHierarchyContextMenu()
{
    m_hierarchyContextMenuOpen = false;
    m_hierarchyContextNodeId = 0;
}

void UiBuilderController::toggleInspectorGroup(const std::string& groupId)
{
    const auto collapsedGroup = m_collapsedInspectorGroups.find(groupId);
    if (collapsedGroup != m_collapsedInspectorGroups.end())
        m_collapsedInspectorGroups.erase(collapsedGroup);
    else
        m_collapsedInspectorGroups.insert(groupId);
}

bool UiBuilderController::isInspectorGroupCollapsed(const std::string& groupId) const
{
    return m_collapsedInspectorGroups.find(groupId) != m_collapsedInspectorGroups.end();
}

void UiBuilderController::fitPreviewZoom()
{
    m_layoutManager.fitPreviewZoom();
    refreshPreviewZoomLabel();
    m_layoutManager.updatePreviewDocumentPlacement(m_document);
}

void UiBuilderController::reloadPreviewDocument()
{
    if (m_context == nullptr)
        return;

    unloadPreviewDocument();

    if (m_previewDocumentSource.empty())
        return;

    const std::string sourceUrl = m_previewDocumentPath.empty() ? "[ui-builder-preview]" : m_previewDocumentPath;
    if (!m_previewRenderer.loadFromMemory(m_previewDocumentSource, sourceUrl))
    {
        m_previewDocument = nullptr;
        return;
    }

    m_previewDocument = m_previewRenderer.document();
    m_layoutManager.setPreviewDocument(m_previewDocument);
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
    if (m_layoutManager.previewCanvasRect().isValid())
        fitPreviewZoom();
    m_layoutManager.updatePreviewDocumentPlacement(m_document);
}

void UiBuilderController::unloadPreviewDocument()
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
    m_layoutManager.setPreviewDocument(nullptr);
}

bool UiBuilderController::loadPreviewDocumentFromFile(const std::string& filePath)
{
    const std::string previousSource = m_previewDocumentSource;
    const std::string previousPath = m_previewDocumentPath;

    if (!m_previewRenderer.loadFromFile(filePath))
        return false;

    m_previewDocumentSource = m_previewRenderer.source();
    m_previewDocumentPath = m_previewRenderer.sourcePath();
    m_previewDocument = m_previewRenderer.document();
    m_layoutManager.setPreviewDocument(m_previewDocument);
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
        if (m_layoutManager.previewCanvasRect().isValid())
            fitPreviewZoom();
        m_layoutManager.updatePreviewDocumentPlacement(m_document);
    }

    if (m_previewDocument == nullptr)
    {
        m_previewDocumentSource = previousSource;
        m_previewDocumentPath = previousPath;
        reloadPreviewDocument();
        return false;
    }

    if (m_hierarchyModel.rebuildFromPreviewDocument(m_previewDocument))
    {
        m_previewDrivenByHierarchy = true;
        m_hierarchyRefreshPending = false;
        m_previewRefreshPending = false;
        refreshHierarchyPresentation();
        refreshPreviewFromHierarchy();
    }

    return true;
}

bool UiBuilderController::savePreviewDocumentToFile(const std::string& filePath) const
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

void UiBuilderController::setPreviewZoom(float zoom)
{
    m_layoutManager.setPreviewZoom(zoom);
    refreshPreviewZoomLabel();
    m_layoutManager.updatePreviewDocumentPlacement(m_document);
}

void UiBuilderController::attachListeners()
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

void UiBuilderController::detachListeners()
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