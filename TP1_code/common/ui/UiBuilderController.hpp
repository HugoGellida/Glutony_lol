#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_set>

#include <common/UI/UiBuilderHierarchyModel.hpp>
#include <common/UI/UiBuilderHierarchyPresenter.hpp>
#include <common/UI/UiBuilderIdCodec.hpp>
#include <common/UI/UiBuilderInspectorPresenter.hpp>
#include <common/UI/UiBuilderDocumentSerializer.hpp>
#include <common/UI/UiBuilderLayoutManager.hpp>
#include <common/UI/UiBuilderWidgetCatalogPresenter.hpp>
#include <common/ui/UIRenderer.hpp>
#include <common/ui/UIBinder.hpp>

#include "EditorUiModeController.hpp"

class UiBuilderController : public EditorUiModeController
{
public:
    bool initialize(Rml::Context* context) override;
    void shutdown() override;
    void activate() override;
    void deactivate() override;
    void setModeChangeCallback(const std::function<void(editor_ui::EditorMode)>& callback) override;
    void syncToWindow(int width, int height) override;
    void setShowStylePanel(bool showStylePanel) override;
    void setUiBuilderShowStylePanel(bool showStylePanel);
    void update() override;
    void render() override;
    UiRect getViewportRect() const override;
    bool isViewportHovered(double mouseX, double mouseY) const override;
    bool isDragging() const override;
    void ProcessEvent(Rml::Event& event) override;

private:
    enum class PreviewDisplayMode
    {
        Preview,
        Render,
    };

    void refreshModePresentation();
    void refreshBuilderMenuState();
    void refreshHierarchyPresentation();
    void refreshInspectorPresentation();
    void refreshPreviewZoomLabel();
    void refreshPreviewModeButtonLabel();
    void requestHierarchyRefresh();
    void requestPreviewRefreshFromHierarchy();
    void refreshPreviewFromHierarchy();
    void fitPreviewZoom();
    void reloadPreviewDocument();
    void unloadPreviewDocument();
    bool loadPreviewDocumentFromFile(const std::string& filePath);
    bool savePreviewDocumentToFile(const std::string& filePath) const;
    void setPreviewZoom(float zoom);
    std::string buildInspectorPanelMarkup() const;
    std::string buildHierarchyMarkup() const;
    std::string buildWidgetCatalogMarkup() const;
    std::string buildPreviewDocumentSourceFromHierarchy() const;
    std::string buildRenderDocumentSourceFromHierarchy() const;
    std::string buildExportDocumentSourceFromHierarchy() const;
    void closeHierarchyContextMenu();
    void toggleInspectorGroup(const std::string& groupId);
    bool isInspectorGroupCollapsed(const std::string& groupId) const;
    void attachListeners();
    void detachListeners();

    Rml::Context* m_context = nullptr;
    Rml::ElementDocument* m_document = nullptr;
    Rml::ElementDocument* m_previewDocument = nullptr;
    UIRenderer m_previewRenderer;
    UIBinder m_leftShellBinder;
    UIBinder m_previewShellBinder;
    UI::UiBuilderHierarchyModel m_hierarchyModel;
    UI::UiBuilderHierarchyPresenter m_hierarchyPresenter;
    UI::UiBuilderInspectorPresenter m_inspectorPresenter;
    UI::UiBuilderDocumentSerializer m_documentSerializer;
    UI::UiBuilderLayoutManager m_layoutManager;
    UI::UiBuilderWidgetCatalogPresenter m_widgetCatalogPresenter;

    Rml::Element* m_root = nullptr;
    Rml::Element* m_builderHeader = nullptr;
    Rml::Element* m_builderMenuFileButton = nullptr;
    Rml::Element* m_builderMenuWindowButton = nullptr;
    Rml::Element* m_builderMenuWindowDropdown = nullptr;
    Rml::Element* m_leftPanel = nullptr;
    Rml::Element* m_leftSplitter = nullptr;
    Rml::Element* m_centerPanel = nullptr;
    Rml::Element* m_rightSplitter = nullptr;
    Rml::Element* m_rightPanel = nullptr;
    Rml::Element* m_viewportPanel = nullptr;
    Rml::Element* m_horizontalSplitter = nullptr;
    Rml::Element* m_bottomPanel = nullptr;

    Rml::Element* m_builderMenuFileDropdown = nullptr;
    Rml::Element* m_leftTopPanel = nullptr;
    Rml::Element* m_leftHorizontalSplitter = nullptr;
    Rml::Element* m_leftBottomPanel = nullptr;
    Rml::Element* m_previewCanvas = nullptr;
    Rml::Element* m_previewWindow = nullptr;
    Rml::Element* m_previewHost = nullptr;
    Rml::Element* m_previewZoomLabel = nullptr;
    Rml::Element* m_previewModeButton = nullptr;

    std::function<void(editor_ui::EditorMode)> m_modeChangeCallback;
    int m_windowWidth = 1;
    int m_windowHeight = 1;
    bool m_uiBuilderShowStylePanel = false;
    bool m_isFileMenuOpen = false;
    bool m_isWindowMenuOpen = false;
    DragPayloadKind m_dragPayloadKind = DragPayloadKind::None;
    std::string m_draggedWidgetKey;
    int m_draggedHierarchyNodeId = 0;
    int m_dropTargetNodeId = 0;
    HierarchyDropMode m_dropMode = HierarchyDropMode::None;
    bool m_hierarchyRefreshPending = false;
    bool m_previewRefreshPending = false;
    bool m_previewDrivenByHierarchy = true;
    PreviewDisplayMode m_previewDisplayMode = PreviewDisplayMode::Preview;
    bool m_hierarchyContextMenuOpen = false;
    int m_hierarchyContextNodeId = 0;
    int m_hierarchyContextMenuX = 0;
    int m_hierarchyContextMenuY = 0;
    std::unordered_set<std::string> m_collapsedInspectorGroups;

    std::string m_previewDocumentSource;
    std::string m_previewDocumentPath;

    UI::UiBuilderLayoutManager::DragTarget m_dragTarget = UI::UiBuilderLayoutManager::DragTarget::None;
};