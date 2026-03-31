#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <common/gameobject/component/ComponentSerialization.hpp>

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

    enum class DragTarget
    {
        None,
        LeftSplitter,
        RightSplitter,
        HorizontalSplitter,
    };

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
    void rebuildHierarchyFromScene(const Scene& scene);
    void appendHierarchyNodeFromGameObject(UiGOHierarchyNode& parentNode, const GameObject& gameObject);
    static bool hierarchyNodesEqual(const UiGOHierarchyNode& lhs, const UiGOHierarchyNode& rhs);
    std::string buildHierarchyMarkup() const;
    std::string buildHierarchyNodeMarkup(const UiGOHierarchyNode& node, int depth) const;
    std::string buildInspectorMarkup() const;
    std::string buildViewportMarkup() const;
    void requestHierarchyRefresh();
    static std::string makeHierarchyNodeElementId(int nodeId);
    static std::optional<int> parseHierarchyNodeId(const Rml::String& elementId);
    static std::string makeTransformFieldElementId(int nodeId, const std::string& fieldKey);
    static std::string makeInspectorFieldElementId(int nodeId, size_t componentIndex, const std::string& fieldKey);
    static std::optional<InspectorFieldBinding> parseInspectorFieldElementId(const Rml::String& elementId);
    static std::string makeInspectorGroupElementId(int nodeId, size_t componentIndex);
    UiGOHierarchyNode* findHierarchyNodeById(int nodeId);
    const UiGOHierarchyNode* findHierarchyNodeById(int nodeId) const;
    UiGOHierarchyNode* findHierarchyNodeByGameObject(const GameObject* gameObject);
    const UiGOHierarchyNode* findHierarchyNodeByGameObject(const GameObject* gameObject) const;
    const UiGOHierarchyNode* findSelectedHierarchyNode() const;
    bool shouldRefreshInspectorPresentation() const;
    bool applyInspectorFieldValue(const InspectorFieldBinding& binding, const std::string& value);
    void toggleInspectorGroup(const std::string& groupId);
    bool isInspectorGroupCollapsed(const std::string& groupId) const;

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
    Rml::Element* m_rightSplitter = nullptr;
    Rml::Element* m_rightPanel = nullptr;
    std::function<void(editor_ui::EditorMode)> m_modeChangeCallback;
    int m_windowWidth = 1;
    int m_windowHeight = 1;
    float m_leftRatio = 0.22f;
    float m_rightRatio = 0.22f;
    float m_viewportRatio = 0.78f;
    bool m_isWindowMenuOpen = false;
    Scene* m_scene = nullptr;
    PlaybackState m_playbackState = PlaybackState::Stopped;
    DragTarget m_dragTarget = DragTarget::None;
    UiRect m_viewportRect;
    UiRect m_centerRect;
    int m_selectedHierarchyNodeId = 1;
    int m_nextHierarchyNodeId = 2;
    bool m_hierarchyRefreshPending = false;
    UiGOHierarchyNode m_hierarchyRoot = {1, "Root", "scene", nullptr, {}};
    std::unordered_set<std::string> m_collapsedInspectorGroups;
};