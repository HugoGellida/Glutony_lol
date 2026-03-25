#pragma once

#include <RmlUi/Core.h>

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <common/ui/UIRenderer.hpp>
#include <common/ui/widget/Widget.hpp>

struct UiRect
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    bool isValid() const
    {
        return width > 0 && height > 0;
    }

    bool contains(double px, double py) const
    {
        return px >= x && py >= y && px < (x + width) && py < (y + height);
    }

    int centerX() const
    {
        return x + width / 2;
    }

    int centerY() const
    {
        return y + height / 2;
    }
};

class EditorUiController : public Rml::EventListener
{
public:
    EditorUiController() = default;
    ~EditorUiController() override = default;

    bool initialize(Rml::Context* context);
    void shutdown();

    void syncToWindow(int width, int height);
    void setUiBuilderEnabled(bool enabled);
    bool isUiBuilderEnabled() const;
    void setUiBuilderShowStylePanel(bool showStylePanel);
    void update();
    void render();

    UiRect getViewportRect() const;
    bool isViewportHovered(double mouseX, double mouseY) const;
    bool isDragging() const;

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

    enum class DragPayloadKind
    {
        None,
        Widget,
        Node,
    };

    enum class HierarchyDropMode
    {
        None,
        Before,
        Inside,
        After,
    };

    enum class HierarchyContextAction
    {
        MoveUp,
        MoveDown,
        Delete,
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
    void refreshPreviewZoomLabel();
    void refreshPreviewModeButtonLabel();
    void refreshHierarchyPresentation();
    void refreshInspectorPresentation();
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

    static Rml::String pixels(int value);
    static int clampInt(int value, int minValue, int maxValue);

    Rml::Context* m_context = nullptr;
    Rml::ElementDocument* m_document = nullptr;
    Rml::ElementDocument* m_previewDocument = nullptr;
    UIRenderer m_previewRenderer;

    Rml::Element* m_root = nullptr;
    Rml::Element* m_builderHeader = nullptr;
    Rml::Element* m_builderMenuFileButton = nullptr;
    Rml::Element* m_builderMenuFileDropdown = nullptr;
    Rml::Element* m_builderMenuWindowButton = nullptr;
    Rml::Element* m_builderMenuWindowDropdown = nullptr;
    Rml::Element* m_leftPanel = nullptr;
    Rml::Element* m_leftTopPanel = nullptr;
    Rml::Element* m_leftHorizontalSplitter = nullptr;
    Rml::Element* m_leftBottomPanel = nullptr;
    Rml::Element* m_leftSplitter = nullptr;
    Rml::Element* m_centerPanel = nullptr;
    Rml::Element* m_rightSplitter = nullptr;
    Rml::Element* m_rightPanel = nullptr;
    Rml::Element* m_viewportPanel = nullptr;
    Rml::Element* m_previewCanvas = nullptr;
    Rml::Element* m_previewWindow = nullptr;
    Rml::Element* m_previewHost = nullptr;
    Rml::Element* m_previewZoomLabel = nullptr;
    Rml::Element* m_previewModeButton = nullptr;
    Rml::Element* m_horizontalSplitter = nullptr;
    Rml::Element* m_bottomPanel = nullptr;

    int m_windowWidth = 1;
    int m_windowHeight = 1;

    bool m_uiBuilderEnabled = false;
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

    float m_leftRatio = 0.22f;
    float m_leftTopRatio = 0.62f;
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

    static constexpr int SplitterThickness = 8;
    static constexpr int BuilderHeaderHeight = 36;
    static constexpr int MinColumnWidth = 120;
    static constexpr int MinCenterWidth = 180;
    static constexpr int MinLeftSectionHeight = 96;
    static constexpr int MinPreviewDocumentWidth = 220;
    static constexpr int MinPreviewDocumentHeight = 140;
    static constexpr int MinViewportHeight = 120;
    static constexpr int MinBottomHeight = 64;
};