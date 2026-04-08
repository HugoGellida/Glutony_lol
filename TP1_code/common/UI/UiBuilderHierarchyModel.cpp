#include <common/UI/UiBuilderHierarchyModel.hpp>

#include <common/ui/EditorUiCommon.hpp>
#include <common/ui/widget/WidgetRegistry.hpp>

#include <functional>
#include <utility>

namespace UI
{
UiBuilderHierarchyModel::UiBuilderHierarchyModel() = default;

void UiBuilderHierarchyModel::reset()
{
    m_root.children.clear();
    m_selectedNodeId = m_root.id;
    m_nextNodeId = m_root.id + 1;
}

UiBuilderHierarchyModel::Node& UiBuilderHierarchyModel::root()
{
    return m_root;
}

const UiBuilderHierarchyModel::Node& UiBuilderHierarchyModel::root() const
{
    return m_root;
}

int UiBuilderHierarchyModel::selectedNodeId() const
{
    return m_selectedNodeId;
}

void UiBuilderHierarchyModel::setSelectedNodeId(int nodeId)
{
    m_selectedNodeId = nodeId;
}

UiBuilderHierarchyModel::Node* UiBuilderHierarchyModel::findNodeById(int nodeId)
{
    if (m_root.id == nodeId)
        return &m_root;

    std::function<Node*(Node&)> findInChildren = [&](Node& node) -> Node*
    {
        for (Node& child : node.children)
        {
            if (child.id == nodeId)
                return &child;
            if (Node* foundChild = findInChildren(child))
                return foundChild;
        }
        return nullptr;
    };

    return findInChildren(m_root);
}

const UiBuilderHierarchyModel::Node* UiBuilderHierarchyModel::findNodeById(int nodeId) const
{
    return const_cast<UiBuilderHierarchyModel*>(this)->findNodeById(nodeId);
}

UiBuilderHierarchyModel::Node* UiBuilderHierarchyModel::findParentNodeOf(int nodeId)
{
    std::function<Node*(Node&)> findParent = [&](Node& node) -> Node*
    {
        for (Node& child : node.children)
        {
            if (child.id == nodeId)
                return &node;
            if (Node* parent = findParent(child))
                return parent;
        }
        return nullptr;
    };

    return findParent(m_root);
}

const UiBuilderHierarchyModel::Node* UiBuilderHierarchyModel::findSelectedNode() const
{
    return findNodeById(m_selectedNodeId);
}

std::optional<ui::widget::InspectorField> UiBuilderHierarchyModel::findInspectorFieldDefinition(const Node& node, const std::string& fieldKey) const
{
    const ui::widget::Widget* widget = ui::widget::findWidgetByKey(node.elementKey);
    if (widget == nullptr)
        return std::nullopt;

    const ui::widget::InspectorModel model = widget->buildInspectorModel(node.label, node.properties);
    for (const ui::widget::InspectorField& field : model.fields)
    {
        if (field.key == fieldKey)
            return field;
    }

    for (const ui::widget::InspectorGroup& group : model.childGroups)
    {
        for (const ui::widget::InspectorField& field : group.fields)
        {
            if (field.key == fieldKey)
                return field;
        }
    }

    return std::nullopt;
}

bool UiBuilderHierarchyModel::applyInspectorFieldValue(int nodeId, const std::string& fieldKey, const std::string& value)
{
    Node* node = findNodeById(nodeId);
    if (node == nullptr)
        return false;

    const ui::widget::Widget* widget = ui::widget::findWidgetByKey(node->elementKey);
    if (widget == nullptr)
        return false;

    const std::optional<ui::widget::InspectorField> field = findInspectorFieldDefinition(*node, fieldKey);
    if (!field.has_value())
        return false;

    if (const std::optional<std::string> primaryTextField = widget->primaryTextPropertyKey())
    {
        if (*primaryTextField == fieldKey)
        {
            if (node->label == value)
                return false;
            node->label = value;
            if (value == field->defaultValue)
                node->properties.erase(fieldKey);
            else
                node->properties[fieldKey] = value;
            return true;
        }
    }

    if (value == field->defaultValue)
    {
        const auto propertyIt = node->properties.find(fieldKey);
        if (propertyIt == node->properties.end())
            return false;
        node->properties.erase(propertyIt);
        return true;
    }

    const auto propertyIt = node->properties.find(fieldKey);
    if (propertyIt != node->properties.end() && propertyIt->second == value)
        return false;

    node->properties[fieldKey] = value;
    return true;
}

bool UiBuilderHierarchyModel::removeNodeById(int nodeId, Node* removedNode)
{
    Node* parent = findParentNodeOf(nodeId);
    if (parent == nullptr)
        return false;

    for (auto childIt = parent->children.begin(); childIt != parent->children.end(); ++childIt)
    {
        if (childIt->id == nodeId)
        {
            if (removedNode != nullptr)
                *removedNode = std::move(*childIt);
            parent->children.erase(childIt);
            return true;
        }
    }

    return false;
}

bool UiBuilderHierarchyModel::insertNodeBefore(int targetNodeId, Node node)
{
    Node* parent = findParentNodeOf(targetNodeId);
    if (parent == nullptr)
        return false;

    for (auto childIt = parent->children.begin(); childIt != parent->children.end(); ++childIt)
    {
        if (childIt->id == targetNodeId)
        {
            parent->children.insert(childIt, std::move(node));
            return true;
        }
    }

    return false;
}

bool UiBuilderHierarchyModel::insertNodeAfter(int targetNodeId, Node node)
{
    Node* parent = findParentNodeOf(targetNodeId);
    if (parent == nullptr)
        return false;

    for (auto childIt = parent->children.begin(); childIt != parent->children.end(); ++childIt)
    {
        if (childIt->id == targetNodeId)
        {
            parent->children.insert(std::next(childIt), std::move(node));
            return true;
        }
    }

    return false;
}

bool UiBuilderHierarchyModel::insertNodeInside(int targetNodeId, Node node)
{
    Node* targetNode = findNodeById(targetNodeId);
    if (targetNode == nullptr)
        return false;

    if (targetNode->id != m_root.id)
    {
        const ui::widget::Widget* widget = ui::widget::findWidgetByKey(targetNode->elementKey);
        if (widget == nullptr || !widget->canHaveEditorChildren())
            return false;
    }

    targetNode->children.push_back(std::move(node));
    return true;
}

bool UiBuilderHierarchyModel::isNodeDescendantOf(int nodeId, int ancestorNodeId) const
{
    const Node* ancestorNode = findNodeById(ancestorNodeId);
    if (ancestorNode == nullptr)
        return false;

    std::function<bool(const Node&)> search = [&](const Node& node) -> bool
    {
        if (node.id == nodeId)
            return true;
        for (const Node& child : node.children)
        {
            if (search(child))
                return true;
        }
        return false;
    };

    for (const Node& child : ancestorNode->children)
    {
        if (search(child))
            return true;
    }
    return false;
}

bool UiBuilderHierarchyModel::moveNodeUp(int nodeId)
{
    Node* parent = findParentNodeOf(nodeId);
    if (parent == nullptr || parent->children.size() <= 1)
        return false;

    for (std::size_t childIndex = 1; childIndex < parent->children.size(); ++childIndex)
    {
        if (parent->children[childIndex].id == nodeId)
        {
            std::swap(parent->children[childIndex - 1], parent->children[childIndex]);
            return true;
        }
    }

    return false;
}

bool UiBuilderHierarchyModel::moveNodeDown(int nodeId)
{
    Node* parent = findParentNodeOf(nodeId);
    if (parent == nullptr || parent->children.size() <= 1)
        return false;

    for (std::size_t childIndex = 0; childIndex + 1 < parent->children.size(); ++childIndex)
    {
        if (parent->children[childIndex].id == nodeId)
        {
            std::swap(parent->children[childIndex], parent->children[childIndex + 1]);
            return true;
        }
    }

    return false;
}

bool UiBuilderHierarchyModel::canMoveNodeUp(int nodeId) const
{
    Node* parent = const_cast<UiBuilderHierarchyModel*>(this)->findParentNodeOf(nodeId);
    if (parent == nullptr || parent->children.size() <= 1)
        return false;

    for (std::size_t childIndex = 1; childIndex < parent->children.size(); ++childIndex)
    {
        if (parent->children[childIndex].id == nodeId)
            return true;
    }

    return false;
}

bool UiBuilderHierarchyModel::canMoveNodeDown(int nodeId) const
{
    Node* parent = const_cast<UiBuilderHierarchyModel*>(this)->findParentNodeOf(nodeId);
    if (parent == nullptr || parent->children.size() <= 1)
        return false;

    for (std::size_t childIndex = 0; childIndex + 1 < parent->children.size(); ++childIndex)
    {
        if (parent->children[childIndex].id == nodeId)
            return true;
    }

    return false;
}

UiBuilderHierarchyModel::Node UiBuilderHierarchyModel::makeNodeForElementKey(const std::string& elementKey)
{
    Node node;
    node.id = m_nextNodeId++;
    node.elementKey = elementKey;
    if (const ui::widget::Widget* widget = ui::widget::findWidgetByKey(elementKey))
    {
        node.tagName = widget->tagName();
        node.label = widget->defaultLabel();
    }
    else
    {
        node.tagName = "div";
        node.label = elementKey;
    }
    return node;
}

bool UiBuilderHierarchyModel::rebuildFromPreviewDocument(const Rml::Element* documentRoot)
{
    const Rml::Element* bodyElement = editor_ui::findFirstElementByTagName(documentRoot, "body");
    if (bodyElement == nullptr)
        return false;

    reset();

    for (int childIndex = 0; childIndex < bodyElement->GetNumChildren(); ++childIndex)
    {
        const Rml::Element* child = bodyElement->GetChild(childIndex);
        if (child != nullptr)
            appendNodesFromElement(m_root, child);
    }

    m_selectedNodeId = m_root.id;
    return true;
}

bool UiBuilderHierarchyModel::appendNodesFromElement(Node& parentNode, const Rml::Element* element)
{
    if (element == nullptr)
        return false;

    if (const ui::widget::Widget* widget = ui::widget::findWidgetByElement(*element))
    {
        Node node = makeNodeForElementKey(widget->key());
        node.label = widget->labelFromElement(*element);
        node.tagName = widget->tagName();
        node.properties = widget->capturePropertiesFromElement(*element);

        if (widget->canHaveEditorChildren())
        {
            for (int childIndex = 0; childIndex < element->GetNumChildren(); ++childIndex)
            {
                const Rml::Element* child = element->GetChild(childIndex);
                if (child != nullptr)
                    appendNodesFromElement(node, child);
            }
        }

        parentNode.children.push_back(std::move(node));
        return true;
    }

    for (int childIndex = 0; childIndex < element->GetNumChildren(); ++childIndex)
    {
        const Rml::Element* child = element->GetChild(childIndex);
        if (child != nullptr)
            appendNodesFromElement(parentNode, child);
    }
    return false;
}
}