#pragma once

#include <RmlUi/Core.h>

#include <common/ui/widget/Widget.hpp>

#include <optional>
#include <string>
#include <vector>

namespace UI
{
class UiBuilderHierarchyModel
{
public:
    struct Node
    {
        int id = 0;
        std::string elementKey;
        std::string tagName;
        std::string label;
        ui::widget::WidgetPropertyMap properties;
        std::vector<Node> children;
    };

    UiBuilderHierarchyModel();

    void reset();

    Node& root();
    const Node& root() const;

    int selectedNodeId() const;
    void setSelectedNodeId(int nodeId);

    Node* findNodeById(int nodeId);
    const Node* findNodeById(int nodeId) const;
    Node* findParentNodeOf(int nodeId);
    const Node* findSelectedNode() const;

    std::optional<ui::widget::InspectorField> findInspectorFieldDefinition(const Node& node, const std::string& fieldKey) const;
    bool applyInspectorFieldValue(int nodeId, const std::string& fieldKey, const std::string& value);

    bool removeNodeById(int nodeId, Node* removedNode);
    bool insertNodeBefore(int targetNodeId, Node node);
    bool insertNodeAfter(int targetNodeId, Node node);
    bool insertNodeInside(int targetNodeId, Node node);
    bool isNodeDescendantOf(int nodeId, int ancestorNodeId) const;
    bool moveNodeUp(int nodeId);
    bool moveNodeDown(int nodeId);
    bool canMoveNodeUp(int nodeId) const;
    bool canMoveNodeDown(int nodeId) const;

    Node makeNodeForElementKey(const std::string& elementKey);
    bool rebuildFromPreviewDocument(const Rml::Element* documentRoot);
    bool appendNodesFromElement(Node& parentNode, const Rml::Element* element);

private:
    int m_selectedNodeId = 1;
    int m_nextNodeId = 2;
    Node m_root = {1, "root", "root", "Root", {}, {}};
};
}