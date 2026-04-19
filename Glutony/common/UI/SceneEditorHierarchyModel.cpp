#include <common/UI/SceneEditorHierarchyModel.hpp>

#include <common/Scene.hpp>

#include <unordered_set>

namespace
{
template <typename NodeType>
NodeType* findHierarchyNodeByIdRecursive(NodeType& node, int nodeId)
{
    if (node.id == nodeId)
        return &node;

    for (auto& child : node.children)
    {
        if (NodeType* found = findHierarchyNodeByIdRecursive(child, nodeId))
            return found;
    }

    return nullptr;
}

template <typename NodeType>
NodeType* findHierarchyNodeByGameObjectRecursive(NodeType& node, const GameObject* gameObject)
{
    if (node.gameObject == gameObject)
        return &node;

    for (auto& child : node.children)
    {
        if (NodeType* found = findHierarchyNodeByGameObjectRecursive(child, gameObject))
            return found;
    }

    return nullptr;
}
}

namespace UI
{
void SceneEditorHierarchyModel::clear()
{
    m_root.children.clear();
    m_selectedNodeId = m_root.id;
}

void SceneEditorHierarchyModel::rebuildFromScene(const Scene& scene)
{
    m_root.children.clear();

    std::unordered_set<const GameObject*> childObjects;
    for (size_t index = 0; index < scene.getGameObjectCount(); ++index)
    {
        GameObject* gameObject = scene.getGameObject(index);
        if (gameObject == nullptr)
            continue;

        for (size_t childIndex = 0; childIndex < gameObject->transform.getChildCount(); ++childIndex)
        {
            if (GameObject* child = gameObject->transform.getChild(childIndex))
                childObjects.insert(child);
        }
    }

    for (size_t index = 0; index < scene.getGameObjectCount(); ++index)
    {
        GameObject* gameObject = scene.getGameObject(index);
        if (gameObject == nullptr || childObjects.count(gameObject) > 0)
            continue;

        appendNodeFromGameObject(m_root, *gameObject);
    }
}

bool SceneEditorHierarchyModel::nodesEqual(const Node& lhs, const Node& rhs)
{
    if (lhs.id != rhs.id ||
        lhs.label != rhs.label ||
        lhs.tagName != rhs.tagName ||
        lhs.gameObject != rhs.gameObject ||
        lhs.children.size() != rhs.children.size())
    {
        return false;
    }

    for (size_t index = 0; index < lhs.children.size(); ++index)
    {
        if (!nodesEqual(lhs.children[index], rhs.children[index]))
            return false;
    }

    return true;
}

SceneEditorHierarchyModel::Node* SceneEditorHierarchyModel::findNodeById(int nodeId)
{
    return findHierarchyNodeByIdRecursive(m_root, nodeId);
}

const SceneEditorHierarchyModel::Node* SceneEditorHierarchyModel::findNodeById(int nodeId) const
{
    return findHierarchyNodeByIdRecursive(m_root, nodeId);
}

SceneEditorHierarchyModel::Node* SceneEditorHierarchyModel::findNodeByGameObject(const GameObject* gameObject)
{
    if (gameObject == nullptr)
        return nullptr;

    return findHierarchyNodeByGameObjectRecursive(m_root, gameObject);
}

const SceneEditorHierarchyModel::Node* SceneEditorHierarchyModel::findNodeByGameObject(const GameObject* gameObject) const
{
    if (gameObject == nullptr)
        return nullptr;

    return findHierarchyNodeByGameObjectRecursive(m_root, gameObject);
}

const SceneEditorHierarchyModel::Node* SceneEditorHierarchyModel::findSelectedNode() const
{
    return findNodeById(m_selectedNodeId);
}

SceneEditorHierarchyModel::Node& SceneEditorHierarchyModel::root()
{
    return m_root;
}

const SceneEditorHierarchyModel::Node& SceneEditorHierarchyModel::root() const
{
    return m_root;
}

int SceneEditorHierarchyModel::selectedNodeId() const
{
    return m_selectedNodeId;
}

void SceneEditorHierarchyModel::setSelectedNodeId(int nodeId)
{
    m_selectedNodeId = nodeId;
}

void SceneEditorHierarchyModel::appendNodeFromGameObject(Node& parentNode, const GameObject& gameObject)
{
    parentNode.children.push_back({gameObject.getId(), gameObject.getName(), "gameobject", &gameObject, {}});
    Node& newNode = parentNode.children.back();

    for (size_t childIndex = 0; childIndex < gameObject.transform.getChildCount(); ++childIndex)
    {
        const GameObject* child = gameObject.transform.getChild(childIndex);
        if (child != nullptr)
            appendNodeFromGameObject(newNode, *child);
    }
}
}