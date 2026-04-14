#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <common/UI/SceneEditorAssetBrowserModel.hpp>
#include <common/UI/SceneEditorHierarchyModel.hpp>
#include <common/UI/SceneEditorLayoutManager.hpp>
#include <common/UI/SceneEditorDomIdCodec.hpp>
#include <common/gameobject/component/ComponentSerialization.hpp>
#include <common/scene/SceneSerialization.hpp>

#include "EditorUiModeController.hpp"

class GameObject;
class Scene;

class SceneEditorController : public EditorUiModeController
{
public:
    using UiGOHierarchyNode = UI::SceneEditorHierarchyModel::Node;
    using AssetBrowserRootKind = UI::SceneEditorAssetBrowserModel::RootKind;
    using AssetBrowserFileKind = UI::SceneEditorAssetBrowserModel::FileKind;
    using AssetBrowserFileEntry = UI::SceneEditorAssetBrowserModel::FileEntry;
    using AssetBrowserDirectoryNode = UI::SceneEditorAssetBrowserModel::DirectoryNode;

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
    bool isExternalPreviewActive() const override;
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

    enum class BottomPanelTab
    {
        AssetBrowser,
        Console,
    };

    enum class ActiveProcessKind
    {
        None,
        Build,
        Player,
    };

    enum class PendingLaunchAction
    {
        None,
        PlayPreview,
        RunDetached,
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

private:

    struct InspectorFieldBinding
    {
        enum class Target
        {
            Scene,
            Transform,
            Component,
        };

        Target target = Target::Component;
        int nodeId = 0;
        size_t componentIndex = 0;
        std::string fieldKey;
    };

    struct InspectorGroupBinding
    {
        int nodeId = 0;
        size_t componentIndex = 0;
    };

    struct MaterialAssetEditorBinding
    {
        InspectorFieldBinding parentField;
        std::string propertyKey;
    };

    struct DataAssetEditorBinding
    {
        InspectorFieldBinding parentField;
        std::string nodePath;
    };

    void attachListeners();
    void detachListeners();
    void applyLayout();
    void refreshHierarchyPresentation();
    void refreshViewportPresentation();
    void refreshPresentation();
    void refreshInspectorPresentation(bool preserveScroll = false);
    void refreshInspectorValuesPresentation();
    void refreshInspectorOverlayPresentation();
    void refreshBottomPanelPresentation(bool preserveScroll = false);
    void refreshAssetBrowserWorkspacePresentation(bool preserveScroll = false);
    void refreshAssetBrowserTreePresentation(bool preserveScroll = false);
    void refreshAssetBrowserFilesPresentation(bool preserveScroll = false);
    void refreshAssetBrowserOverlayPresentation();
    void refreshAssetBrowserDirectorySelectionPresentation(const std::string& previousDirectoryId);
    void refreshAssetBrowserFileSelectionPresentation(const std::string& previousFileId);
    void refreshCachedRects();
    std::string buildHierarchyMarkup() const;
    std::string buildHierarchyNodeMarkup(const UiGOHierarchyNode& node, int depth) const;
    std::string buildHierarchyContextMenuMarkup() const;
    std::string buildInspectorMarkup() const;
    std::string buildInspectorOverlayMarkup() const;
    std::string buildInspectorAddComponentMenuMarkup() const;
    std::string buildInspectorComponentContextMenuMarkup() const;
    std::string buildAssetBrowserMarkup() const;
    std::string buildAssetBrowserWorkspaceMarkup() const;
    std::string buildAssetBrowserTreePaneMarkup() const;
    std::string buildAssetBrowserFilesPaneMarkup() const;
    std::string buildConsoleMarkup() const;
    std::string buildAssetBrowserOverlayMarkup() const;
    std::string buildAssetBrowserDirectoryMarkup(const AssetBrowserDirectoryNode& node, int depth) const;
    std::string buildAssetBrowserFileGridMarkup(const AssetBrowserDirectoryNode* directory) const;
    std::string buildAssetBrowserContextMenuMarkup() const;
    std::string buildViewportMarkup() const;
    std::string buildSceneDirtyPromptMarkup() const;
    void requestHierarchyRefresh();
    void requestSelectionRefresh();
    static std::string makeTransformFieldElementId(int nodeId, const std::string& fieldKey);
    static std::string makeSceneFieldElementId(const std::string& fieldKey);
    static std::string makeInspectorFieldElementId(int nodeId, size_t componentIndex, const std::string& fieldKey);
    static std::optional<InspectorFieldBinding> parseInspectorFieldElementId(const Rml::String& elementId);
    static std::string makeInspectorGroupElementId(int nodeId, size_t componentIndex);
    static std::optional<InspectorGroupBinding> parseInspectorGroupElementId(const Rml::String& elementId);
    static std::string makeMaterialAssetEditorGroupElementId(int nodeId, size_t componentIndex, const std::string& fieldKey);
    static std::string makeMaterialAssetEditorIconElementId(int nodeId, size_t componentIndex, const std::string& fieldKey);
    static std::string makeMaterialAssetEditorBodyElementId(int nodeId, size_t componentIndex, const std::string& fieldKey);
    static std::string makeMaterialAssetEditorFieldElementId(int nodeId, size_t componentIndex, const std::string& fieldKey, const std::string& propertyKey);
    static std::optional<InspectorFieldBinding> parseMaterialAssetEditorGroupElementId(const Rml::String& elementId);
    static std::optional<MaterialAssetEditorBinding> parseMaterialAssetEditorFieldElementId(const Rml::String& elementId);
    static std::string makeDataAssetEditorBindingToken(const InspectorFieldBinding& binding);
    static std::optional<InspectorFieldBinding> parseDataAssetEditorBindingToken(const std::string& token);
    static std::string makeDataAssetEditorGroupElementId(const InspectorFieldBinding& binding);
    static std::string makeDataAssetEditorIconElementId(const InspectorFieldBinding& binding);
    static std::string makeDataAssetEditorBodyElementId(const InspectorFieldBinding& binding);
    static std::string makeDataAssetEditorNodeGroupElementId(const InspectorFieldBinding& binding, const std::string& nodePath);
    static std::string makeDataAssetEditorFieldElementId(const InspectorFieldBinding& binding, const std::string& nodePath);
    static std::optional<InspectorFieldBinding> parseDataAssetEditorGroupElementId(const Rml::String& elementId);
    static std::optional<DataAssetEditorBinding> parseDataAssetEditorNodeGroupElementId(const Rml::String& elementId);
    static std::optional<DataAssetEditorBinding> parseDataAssetEditorFieldElementId(const Rml::String& elementId);
    bool shouldRefreshInspectorPresentation() const;
    const component_meta::ComponentFieldDescriptor* findInspectorFieldDescriptor(const InspectorFieldBinding& binding) const;
    bool isMaterialAssetInspectorField(const InspectorFieldBinding& binding) const;
    bool isDataAssetInspectorField(const InspectorFieldBinding& binding) const;
    std::optional<std::string> getDataAssetInspectorAssetPath(const InspectorFieldBinding& binding) const;
    std::vector<InspectorFieldBinding> collectVisibleDataAssetInspectorBindings() const;
    void refreshVisibleDataAssetEditorsForAsset(const std::string& normalizedAssetPath);
    bool applyInspectorFieldValue(const InspectorFieldBinding& binding, const std::string& value);
    bool applyMaterialAssetEditorFieldValue(const MaterialAssetEditorBinding& binding, const std::string& value);
    bool applyDataAssetEditorFieldValue(const DataAssetEditorBinding& binding, const std::string& value);
    bool canDropDraggedAssetOnInspectorField(const InspectorFieldBinding& binding) const;
    bool canDropDraggedAssetOnMaterialAssetEditorField(const MaterialAssetEditorBinding& binding) const;
    bool applyDraggedAssetToInspectorField(const InspectorFieldBinding& binding);
    bool applyDraggedAssetToMaterialAssetEditorField(const MaterialAssetEditorBinding& binding);
    void toggleInspectorGroup(const std::string& groupId);
    bool isInspectorGroupCollapsed(const std::string& groupId) const;
    void toggleMaterialAssetEditor(const InspectorFieldBinding& binding);
    bool isMaterialAssetEditorCollapsed(const InspectorFieldBinding& binding) const;
    std::string buildMaterialAssetEditorMarkup(const InspectorFieldBinding& binding, const std::string& assetPath) const;
    std::string buildMaterialAssetEditorBodyMarkup(const InspectorFieldBinding& binding, const std::string& assetPath) const;
    void refreshMaterialAssetEditorPresentation(const InspectorFieldBinding& binding);
    void toggleDataAssetEditor(const InspectorFieldBinding& binding);
    bool isDataAssetEditorCollapsed(const InspectorFieldBinding& binding) const;
    void toggleDataAssetEditorNode(const DataAssetEditorBinding& binding);
    bool isDataAssetEditorNodeCollapsed(const DataAssetEditorBinding& binding) const;
    std::string buildDataAssetEditorMarkup(const InspectorFieldBinding& binding, const std::string& assetPath) const;
    std::string buildDataAssetEditorBodyMarkup(const InspectorFieldBinding& binding, const std::string& assetPath) const;
    std::string buildDataAssetEditorNodeMarkup(const InspectorFieldBinding& binding, const asset::DataAssetNodeDefinition& node, const std::string& nodePath, int depth) const;
    void refreshDataAssetEditorPresentation(const InspectorFieldBinding& binding);
    void markSceneDirty(bool requestFullRuntimeSync = true);
    void queueRuntimeGameObjectSync(int nodeId);
    void clearSceneDirty();
    bool saveScene();
    bool saveSceneAs();
    bool loadSceneFromFilePath(const std::string& filePath);
    bool loadSceneFromDialog();
    void beginPendingSceneAction(PendingSceneAction action, const std::string& targetPath = "");
    bool executePendingSceneAction();
    void closePendingSceneActionPrompt();
    void closeHeaderMenus();
    bool prepareRuntimeSceneFile(std::string& outputPath);
    bool prepareGeneratedGameplaySource();
    bool rebuildSceneRenderPipelines();
    bool bakeSceneRenderTargets();
    bool startBuild(PendingLaunchAction launchAction);
    bool startPreviewPlayer(const std::string& scenePath);
    bool startDetachedPlayer(const std::string& scenePath);
    void stopExternalProcess(bool restoreEditorScene = true);
    void pausePreviewPlayer();
    void resumePreviewPlayer();
    bool flushDirtyMaterialAssetsForRuntimeScene();
    void syncRuntimePreviewGameObjectIfNeeded();
    void syncRuntimePreviewSelectionIfNeeded();
    void syncRuntimePreviewSceneIfNeeded();
    void pollRuntimePreviewState();
    void pollRuntimePreviewSceneState();
    void pollRuntimePreviewMaterialState();
    void pollRuntimePreviewDataAssetState();
    void pollDataAssetExternalChanges();
    void applyPendingExternalDataAssetReloads();
    void pollExternalProcess();
    void updatePlaybackStatusPresentation();
    std::string buildPlaybackStatusText() const;
    void appendConsoleOutput(const std::string& text, const std::string& sourceClass = "");
    void appendConsoleLine(const std::string& line, const std::string& sourceClass = "");
    void appendConsoleSystemMessage(const std::string& message, const std::string& sourceClass = "console_line_info");
    void clearConsole();

    Rml::Context* m_context = nullptr;
    Rml::ElementDocument* m_document = nullptr;
    Rml::Element* m_root = nullptr;
    Rml::Element* m_builderHeader = nullptr;
    Rml::Element* m_leftPanel = nullptr;
    Rml::Element* m_leftSplitter = nullptr;
    Rml::Element* m_centerPanel = nullptr;
    Rml::Element* m_viewportPanel = nullptr;
    Rml::Element* m_viewportSurface = nullptr;
    Rml::Element* m_playbackStatusElement = nullptr;
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
    UI::SceneEditorLayoutManager m_layoutManager;
    UI::SceneEditorHierarchyModel m_hierarchyModel;
    UI::SceneEditorAssetBrowserModel m_assetBrowserModel;
    BottomPanelTab m_bottomPanelTab = BottomPanelTab::AssetBrowser;
    bool m_isFileMenuOpen = false;
    bool m_isEditMenuOpen = false;
    bool m_isWindowMenuOpen = false;
    bool m_assetBrowserContextMenuOpen = false;
    bool m_hierarchyContextMenuOpen = false;
    bool m_addComponentMenuOpen = false;
    bool m_inspectorComponentContextMenuOpen = false;
    bool m_sceneDirty = false;
    bool m_sceneSavePromptOpen = false;
    Scene* m_scene = nullptr;
    PlaybackState m_playbackState = PlaybackState::Stopped;
    PendingSceneAction m_pendingSceneAction = PendingSceneAction::None;
    ActiveProcessKind m_activeProcessKind = ActiveProcessKind::None;
    PendingLaunchAction m_pendingLaunchAction = PendingLaunchAction::None;
    std::optional<scene_serialization::SceneSnapshot> m_runtimeSceneSnapshot;
    std::string m_currentSceneFilePath;
    std::string m_pendingSceneTargetPath;
    std::string m_pendingLaunchScenePath;
    std::string m_consolePartialLine;
    DragTarget m_dragTarget = DragTarget::None;
    DragPayloadKind m_dragPayloadKind = DragPayloadKind::None;
    std::string m_draggedAssetFileId;
    std::string m_draggedAssetRuntimePath;
    std::string m_hoveredInspectorFieldId;
    bool m_hierarchyRefreshPending = false;
    bool m_selectionRefreshPending = false;
    int m_assetBrowserContextMenuX = 0;
    int m_assetBrowserContextMenuY = 0;
    int m_hierarchyContextMenuX = 0;
    int m_hierarchyContextMenuY = 0;
    int m_hierarchyContextMenuNodeId = 0;
    int m_addComponentMenuX = 0;
    int m_addComponentMenuY = 0;
    int m_inspectorComponentContextMenuX = 0;
    int m_inspectorComponentContextMenuY = 0;
    int m_inspectorComponentContextMenuNodeId = 0;
    size_t m_inspectorComponentContextMenuIndex = 0;
    std::unordered_set<std::string> m_collapsedInspectorGroups;
    std::vector<std::string> m_consoleLines;
    int m_activeProcessPid = -1;
    int m_activeProcessOutputFd = -1;
    int m_runtimeGameObjectSyncId = -1;
    uint64_t m_runtimePauseSequence = 0;
    uint64_t m_runtimeDataAssetSyncSequence = 0;
    uint64_t m_runtimeDataAssetStateSequence = 0;
    uint64_t m_runtimeGameObjectSyncSequence = 0;
    uint64_t m_runtimeMaterialSyncSequence = 0;
    uint64_t m_runtimeMaterialStateSequence = 0;
    uint64_t m_runtimeSelectionSyncSequence = 0;
    uint64_t m_runtimeSceneStateSequence = 0;
    uint64_t m_runtimeSceneSyncSequence = 0;
    uint64_t m_runtimeStateSequence = 0;
    std::optional<int> m_pendingRuntimeSelectionId;
    std::optional<int> m_lastSentRuntimeSelectionId;
    bool m_runtimeSceneSyncPending = false;
    bool m_consoleRefreshPending = false;
    int m_runtimePreviewFps = -1;
    std::string m_lastPlaybackStatusText;
    std::unordered_map<std::string, std::filesystem::file_time_type> m_observedDataAssetWriteTimes;
    std::unordered_set<std::string> m_pendingExternalDataAssetReloadPaths;
    std::unordered_set<std::string> m_collapsedMaterialAssetEditors;
    std::unordered_set<std::string> m_collapsedDataAssetEditors;
    std::unordered_set<std::string> m_collapsedDataAssetEditorNodes;
};