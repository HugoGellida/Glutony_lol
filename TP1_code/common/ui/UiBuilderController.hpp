#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <common/ui/UIRenderer.hpp>
#include <common/ui/widget/Widget.hpp>

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

    struct UiHierarchyNode
    {
        int id = 0;
        std::string elementKey;
        std::string tagName;
        std::string label;
        ui::widget::WidgetPropertyMap properties;
        std::vector<UiHierarchyNode> children;
    };

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

    void applyLayout();
    void refreshCachedRects();
    void refreshModePresentation();
    void refreshBuilderMenuState();
    void refreshHierarchyPresentation();
    void refreshInspectorPresentation();
    void refreshPreviewZoomLabel();
    void refreshPreviewModeButtonLabel();
    void requestHierarchyRefresh();
    void requestPreviewRefreshFromHierarchy();
    void refreshPreviewFromHierarchy();
    void resetHierarchyModel();
    void fitPreviewZoom();
    DragTarget getPreviewResizeTargetAt(float mouseX, float mouseY) const;
    void updatePreviewCursor(float mouseX, float mouseY);
    void reloadPreviewDocument();
    void unloadPreviewDocument();
    void updatePreviewDocumentPlacement();
    bool loadPreviewDocumentFromFile(const std::string& filePath);
    bool savePreviewDocumentToFile(const std::string& filePath) const;
    void setPreviewZoom(float zoom);
    std::string buildInspectorPanelMarkup() const;
    std::string buildHierarchyMarkup() const;
    std::string buildHierarchyNodeMarkup(const UiHierarchyNode& node, int depth) const;
    std::string buildWidgetCatalogMarkup() const;
    std::string buildHierarchyContextMenuMarkup() const;
    std::string buildPreviewDocumentSourceFromHierarchy() const;
    std::string buildPreviewNodeMarkup(const UiHierarchyNode& node, int depth) const;
    std::string buildRenderDocumentSourceFromHierarchy() const;
    std::string buildExportDocumentSourceFromHierarchy() const;
    std::string buildExportNodeMarkup(const UiHierarchyNode& node, int depth) const;
    bool rebuildHierarchyFromCurrentPreviewDocument();
    bool appendHierarchyNodesFromElement(UiHierarchyNode& parentNode, const Rml::Element* element);
    static std::string makeHierarchyNodeElementId(int nodeId);
    static std::string makeHierarchyDropElementId(int nodeId, HierarchyDropMode dropMode);
    static std::string makeInspectorFieldElementId(int nodeId, const std::string& fieldKey);
    static std::optional<int> parseHierarchyNodeId(const Rml::String& elementId);
    static std::optional<std::pair<int, HierarchyDropMode>> parseHierarchyDropId(const Rml::String& elementId);
    static std::optional<std::pair<int, std::string>> parseInspectorFieldElementId(const Rml::String& elementId);
    UiHierarchyNode* findHierarchyNodeById(int nodeId);
    const UiHierarchyNode* findHierarchyNodeById(int nodeId) const;
    UiHierarchyNode* findParentNodeOf(int nodeId);
    std::optional<ui::widget::InspectorField> findInspectorFieldDefinition(const UiHierarchyNode& node, const std::string& fieldKey) const;
    bool applyInspectorFieldValue(int nodeId, const std::string& fieldKey, const std::string& value);
    bool removeHierarchyNodeById(int nodeId, UiHierarchyNode* removedNode);
    bool insertHierarchyNodeBefore(int targetNodeId, UiHierarchyNode node);
    bool insertHierarchyNodeAfter(int targetNodeId, UiHierarchyNode node);
    bool insertHierarchyNodeInside(int targetNodeId, UiHierarchyNode node);
    bool isHierarchyNodeDescendantOf(int nodeId, int ancestorNodeId) const;
    bool moveHierarchyNodeUp(int nodeId);
    bool moveHierarchyNodeDown(int nodeId);
    bool canMoveHierarchyNodeUp(int nodeId) const;
    bool canMoveHierarchyNodeDown(int nodeId) const;
    void closeHierarchyContextMenu();
    void toggleInspectorGroup(const std::string& groupId);
    bool isInspectorGroupCollapsed(const std::string& groupId) const;
    const UiHierarchyNode* findSelectedHierarchyNode() const;
    UiHierarchyNode makeHierarchyNodeForElementKey(const std::string& elementKey);
    void attachListeners();
    void detachListeners();

    Rml::Context* m_context = nullptr;
    Rml::ElementDocument* m_document = nullptr;
    Rml::ElementDocument* m_previewDocument = nullptr;
    UIRenderer m_previewRenderer;

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
    int m_selectedHierarchyNodeId = 1;
    int m_nextHierarchyNodeId = 2;
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

    float m_leftTopRatio = 0.62f;
    float m_leftRatio = 0.22f;
    float m_rightRatio = 0.22f;
    float m_viewportRatio = 0.78f;
    float m_previewZoom = 1.0f;
    int m_previewDocumentWidth = 1280;
    int m_previewDocumentHeight = 720;
    DragTarget m_dragTarget = DragTarget::None;

    UiRect m_leftPanelRect;
    UiRect m_viewportRect;
    UiRect m_centerRect;
    UiRect m_previewCanvasRect;
    UiRect m_previewPageRect;
    UiRect m_previewWindowRect;
    UiRect m_previewHostRect;
    UiHierarchyNode m_hierarchyRoot = {1, "root", "root", "Root", {}, {}};
};