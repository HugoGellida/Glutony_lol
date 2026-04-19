#pragma once

#include <string>
#include <vector>

class GameObject;
class Scene;

namespace UI
{
class SceneEditorHierarchyModel
{
public:
    struct Node
    {
        int id = 0;
        std::string label;
        std::string tagName;
        const GameObject* gameObject = nullptr;
        std::vector<Node> children;
    };

    void clear();
    void rebuildFromScene(const Scene& scene);

    static bool nodesEqual(const Node& lhs, const Node& rhs);

    Node* findNodeById(int nodeId);
    const Node* findNodeById(int nodeId) const;
    Node* findNodeByGameObject(const GameObject* gameObject);
    const Node* findNodeByGameObject(const GameObject* gameObject) const;
    const Node* findSelectedNode() const;

    Node& root();
    const Node& root() const;

    int selectedNodeId() const;
    void setSelectedNodeId(int nodeId);

private:
    void appendNodeFromGameObject(Node& parentNode, const GameObject& gameObject);

    Node m_root = {0, "Scene", "scene", nullptr, {}};
    int m_selectedNodeId = 0;
};
}