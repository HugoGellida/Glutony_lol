#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <common/gameobject/component/ComponentSerialization.hpp>
#include <common/scene/SceneSerialization.hpp>

#include "EditorUiModeController.hpp"

class GameObject;
class Scene;

class SceneEditorController : public EditorUiModeController
{
public:
    bool initialize(Rml::Context* context) override;
    void shutdown() override;
    void activate() override;
    void deactivate() override;
    void setModeChangeCallback(const std::function<void(editor_ui::EditorMode)>& callback) override;
    void syncToWindow(int width, int height) override;
    void sync(Scene& scene);
    void setShowStylePanel(bool showStylePanel) override;
    void update() override;
    void render() override;
    UiRect getViewportRect() const override;
    bool isViewportHovered(double mouseX, double mouseY) const override;
    bool isDragging() const override;
    void ProcessEvent(Rml::Event& event) override;

private:
    enum class PlaybackState
    {
        Stopped,
        Playing,
        Paused,
    };

    enum class PendingSceneAction
    {
        None,
        LoadFromDialog,
        OpenFile,
    };

public:

    enum class DragTarget
    {
        None,
        LeftSplitter,
        RightSplitter,
        HorizontalSplitter,
        BottomBrowserSplitter,
    };

    enum class AssetBrowserRootKind
    {
        Assets,
        BuiltIn,
    };

    enum class AssetBrowserFileKind
    {
        Generic,
        Material,
        Mesh,
        Shader,
        Texture,
        Scene,
    };

    struct AssetBrowserFileEntry
    {
        std::string id;
        std::string label;
        std::string runtimePath;
        std::string diskPath;
        std::string extension;
        AssetBrowserRootKind rootKind = AssetBrowserRootKind::Assets;
        AssetBrowserFileKind fileKind = AssetBrowserFileKind::Generic;
        DragPayloadKind dragPayloadKind = DragPayloadKind::AssetFile;
    };

    struct AssetBrowserDirectoryNode
    {
        std::string id;
        std::string label;
        std::string runtimePath;
        std::string diskPath;
        AssetBrowserRootKind rootKind = AssetBrowserRootKind::Assets;
        std::vector<AssetBrowserDirectoryNode> children;
        std::vector<AssetBrowserFileEntry> files;
    };

private:

    struct UiGOHierarchyNode
    {
        int id = 0;
        std::string label;
        std::string tagName;
        const GameObject* gameObject = nullptr;
        std::vector<UiGOHierarchyNode> children;
    };

    struct InspectorFieldBinding
    {
        enum class Target
        {
            Transform,
            Component,
        };

        Target target = Target::Component;
        int nodeId = 0;
        size_t componentIndex = 0;
        std::string fieldKey;
    };

    void attachListeners();
    void detachListeners();
    void applyLayout();
    void refreshPresentation();
    void refreshInspectorPresentation();
    void refreshInspectorValuesPresentation();
    void refreshCachedRects();
    void rescanAssetBrowser();
    void rebuildHierarchyFromScene(const Scene& scene);
    void appendHierarchyNodeFromGameObject(UiGOHierarchyNode& parentNode, const GameObject& gameObject);
    static bool hierarchyNodesEqual(const UiGOHierarchyNode& lhs, const UiGOHierarchyNode& rhs);
    std::string buildHierarchyMarkup() const;
    std::string buildHierarchyNodeMarkup(const UiGOHierarchyNode& node, int depth) const;
    std::string buildHierarchyContextMenuMarkup() const;
    std::string buildInspectorMarkup() const;
    std::string buildInspectorAddComponentMenuMarkup() const;
    std::string buildAssetBrowserMarkup() const;
    std::string buildAssetBrowserDirectoryMarkup(const AssetBrowserDirectoryNode& node, int depth) const;
    std::string buildAssetBrowserFileGridMarkup(const AssetBrowserDirectoryNode* directory) const;
    std::string buildAssetBrowserContextMenuMarkup() const;
    std::string buildViewportMarkup() const;
    std::string buildSceneDirtyPromptMarkup() const;
    void requestHierarchyRefresh();
    static std::string makeHierarchyNodeElementId(int nodeId);
    static std::string makeAssetDirectoryElementId(const std::string& directoryId);
    static std::string makeAssetFileElementId(const std::string& fileId);
    static std::optional<int> parseHierarchyNodeId(const Rml::String& elementId);
    static std::optional<std::string> parseAssetDirectoryElementId(const Rml::String& elementId);
    static std::optional<std::string> parseAssetFileElementId(const Rml::String& elementId);
    static std::string makeTransformFieldElementId(int nodeId, const std::string& fieldKey);
    static std::string makeInspectorFieldElementId(int nodeId, size_t componentIndex, const std::string& fieldKey);
    static std::optional<InspectorFieldBinding> parseInspectorFieldElementId(const Rml::String& elementId);
    static std::string makeInspectorGroupElementId(int nodeId, size_t componentIndex);
    UiGOHierarchyNode* findHierarchyNodeById(int nodeId);
    const UiGOHierarchyNode* findHierarchyNodeById(int nodeId) const;
    UiGOHierarchyNode* findHierarchyNodeByGameObject(const GameObject* gameObject);
    const UiGOHierarchyNode* findHierarchyNodeByGameObject(const GameObject* gameObject) const;
    const UiGOHierarchyNode* findSelectedHierarchyNode() const;
    AssetBrowserDirectoryNode* findAssetDirectoryById(const std::string& directoryId);
    const AssetBrowserDirectoryNode* findAssetDirectoryById(const std::string& directoryId) const;
    const AssetBrowserFileEntry* findAssetFileById(const std::string& fileId) const;
    const AssetBrowserDirectoryNode* findSelectedAssetDirectory() const;
    void selectAssetDirectory(const std::string& directoryId);
    void toggleAssetDirectoryExpansion(const std::string& directoryId);
    bool isAssetDirectoryExpanded(const AssetBrowserDirectoryNode& node) const;
    bool shouldRefreshInspectorPresentation() const;
    const component_meta::ComponentFieldDescriptor* findInspectorFieldDescriptor(const InspectorFieldBinding& binding) const;
    bool applyInspectorFieldValue(const InspectorFieldBinding& binding, const std::string& value);
    bool canDropDraggedAssetOnInspectorField(const InspectorFieldBinding& binding) const;
    bool applyDraggedAssetToInspectorField(const InspectorFieldBinding& binding);
    void toggleInspectorGroup(const std::string& groupId);
    bool isInspectorGroupCollapsed(const std::string& groupId) const;
    void markSceneDirty();
    void clearSceneDirty();
    bool saveScene();
    bool saveSceneAs();
    bool loadSceneFromFilePath(const std::string& filePath);
    bool loadSceneFromDialog();
    void beginPendingSceneAction(PendingSceneAction action, const std::string& targetPath = "");
    bool executePendingSceneAction();
    void closePendingSceneActionPrompt();
    void closeHeaderMenus();

    Rml::Context* m_context = nullptr;
    Rml::ElementDocument* m_document = nullptr;
    Rml::Element* m_root = nullptr;
    Rml::Element* m_builderHeader = nullptr;
    Rml::Element* m_leftPanel = nullptr;
    Rml::Element* m_leftSplitter = nullptr;
    Rml::Element* m_centerPanel = nullptr;
    Rml::Element* m_viewportPanel = nullptr;
    Rml::Element* m_viewportSurface = nullptr;
    Rml::Element* m_horizontalSplitter = nullptr;
    Rml::Element* m_bottomPanel = nullptr;
    Rml::Element* m_bottomBrowserFilesPane = nullptr;
    Rml::Element* m_bottomBrowserSplitter = nullptr;
    Rml::Element* m_bottomBrowserTreePane = nullptr;
    Rml::Element* m_rightSplitter = nullptr;
    Rml::Element* m_rightPanel = nullptr;
    std::function<void(editor_ui::EditorMode)> m_modeChangeCallback;
    int m_windowWidth = 1;
    int m_windowHeight = 1;
    float m_leftRatio = 0.22f;
    float m_rightRatio = 0.22f;
    float m_viewportRatio = 0.78f;
    float m_bottomBrowserTreeRatio = 0.34f;
    bool m_isFileMenuOpen = false;
    bool m_isWindowMenuOpen = false;
    bool m_assetBrowserContextMenuOpen = false;
    bool m_hierarchyContextMenuOpen = false;
    bool m_addComponentMenuOpen = false;
    bool m_sceneDirty = false;
    bool m_sceneSavePromptOpen = false;
    Scene* m_scene = nullptr;
    PlaybackState m_playbackState = PlaybackState::Stopped;
    PendingSceneAction m_pendingSceneAction = PendingSceneAction::None;
    std::optional<scene_serialization::SceneSnapshot> m_runtimeSceneSnapshot;
    std::string m_currentSceneFilePath;
    std::string m_pendingSceneTargetPath;
    DragTarget m_dragTarget = DragTarget::None;
    DragPayloadKind m_dragPayloadKind = DragPayloadKind::None;
    std::string m_draggedAssetFileId;
    std::string m_draggedAssetRuntimePath;
    std::string m_hoveredInspectorFieldId;
    UiRect m_viewportRect;
    UiRect m_centerRect;
    int m_selectedHierarchyNodeId = 1;
    int m_nextHierarchyNodeId = 2;
    bool m_hierarchyRefreshPending = false;
    UiGOHierarchyNode m_hierarchyRoot = {1, "Root", "scene", nullptr, {}};
    std::vector<AssetBrowserDirectoryNode> m_assetBrowserRoots;
    std::unordered_set<std::string> m_expandedAssetDirectoryIds;
    std::string m_selectedAssetDirectoryId;
    std::string m_selectedAssetFileId;
    int m_assetBrowserContextMenuX = 0;
    int m_assetBrowserContextMenuY = 0;
    int m_hierarchyContextMenuX = 0;
    int m_hierarchyContextMenuY = 0;
    int m_hierarchyContextMenuNodeId = 0;
    int m_addComponentMenuX = 0;
    int m_addComponentMenuY = 0;
    std::unordered_set<std::string> m_collapsedInspectorGroups;
};