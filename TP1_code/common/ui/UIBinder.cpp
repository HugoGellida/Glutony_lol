#include "UIBinder.hpp"

#include <cctype>

namespace
{
bool parseElementMarkup(
    const std::string& markup,
    std::unordered_map<std::string, std::string>& attributes,
    std::string& innerMarkup)
{
    attributes.clear();
    innerMarkup.clear();

    const std::size_t tagStart = markup.find('<');
    const std::size_t tagEnd = markup.find('>');
    if (tagStart == std::string::npos || tagEnd == std::string::npos || tagEnd <= tagStart + 1)
        return false;

    const std::size_t nameStart = tagStart + 1;
    std::size_t cursor = nameStart;
    while (cursor < tagEnd && !std::isspace(static_cast<unsigned char>(markup[cursor])) && markup[cursor] != '>')
        ++cursor;

    if (cursor == nameStart)
        return false;

    const std::string tagName = markup.substr(nameStart, cursor - nameStart);
    const std::string closingTag = "</" + tagName + ">";
    const std::size_t closeStart = markup.rfind(closingTag);
    if (closeStart == std::string::npos || closeStart < tagEnd)
        return false;

    while (cursor < tagEnd)
    {
        while (cursor < tagEnd && std::isspace(static_cast<unsigned char>(markup[cursor])) != 0)
            ++cursor;
        if (cursor >= tagEnd)
            break;

        const std::size_t keyStart = cursor;
        while (cursor < tagEnd && markup[cursor] != '=' && !std::isspace(static_cast<unsigned char>(markup[cursor])) && markup[cursor] != '>')
            ++cursor;
        if (cursor <= keyStart || cursor >= tagEnd)
            break;

        const std::string key = markup.substr(keyStart, cursor - keyStart);
        while (cursor < tagEnd && std::isspace(static_cast<unsigned char>(markup[cursor])) != 0)
            ++cursor;
        if (cursor >= tagEnd || markup[cursor] != '=')
            break;

        ++cursor;
        while (cursor < tagEnd && std::isspace(static_cast<unsigned char>(markup[cursor])) != 0)
            ++cursor;
        if (cursor >= tagEnd)
            break;

        const char quote = markup[cursor];
        if (quote != '\'' && quote != '"')
            break;

        ++cursor;
        const std::size_t valueStart = cursor;
        while (cursor < tagEnd && markup[cursor] != quote)
            ++cursor;
        if (cursor > tagEnd)
            break;

        attributes[key] = markup.substr(valueStart, cursor - valueStart);
        if (cursor < tagEnd)
            ++cursor;
    }

    innerMarkup = markup.substr(tagEnd + 1, closeStart - (tagEnd + 1));
    return true;
}
}

UIBinder::ApplySummary UIBinder::apply(Rml::Element* mountPoint, const UI::AUIElement& root)
{
    ApplySummary summary;
    if (mountPoint == nullptr)
        return summary;

    const SnapshotNode currentRoot = buildSnapshot(root, root.getHierarchicalId());
    if (!m_hasSnapshot || m_previousRoot.childCount != currentRoot.childCount)
    {
        mountPoint->SetInnerRML(root.getRML());
        m_previousRoot = currentRoot;
        m_hasSnapshot = true;
        summary.rebuiltRoot = true;
        return summary;
    }

    Rml::ElementDocument* document = mountPoint->GetOwnerDocument();
    if (document == nullptr)
    {
        mountPoint->SetInnerRML(root.getRML());
        m_previousRoot = currentRoot;
        m_hasSnapshot = true;
        summary.rebuiltRoot = true;
        return summary;
    }

    if (!applyTargetedChanges(document, m_previousRoot, currentRoot, summary))
    {
        summary.hasUnsupportedChanges = true;
    }

    m_previousRoot = currentRoot;
    m_hasSnapshot = true;
    return summary;
}

void UIBinder::clear()
{
    m_previousRoot = {};
    m_hasSnapshot = false;
}

bool UIBinder::hasSnapshot() const
{
    return m_hasSnapshot;
}

UIBinder::SnapshotNode UIBinder::buildSnapshot(const UI::AUIElement& node, const std::string& path) const
{
    SnapshotNode snapshot;
    snapshot.path = path;
    snapshot.domId = node.getDomIdForPath(path);
    snapshot.markup = node.getRMLForPath(path);
    snapshot.childrenMarkup = node.getChildrenRMLForPath(path);
    snapshot.childCount = node.getChildCount();
    snapshot.canPatchElement = parseElementMarkup(snapshot.markup, snapshot.attributes, snapshot.innerMarkup);
    snapshot.children.reserve(node.getChildCount());

    for (std::size_t index = 0; index < node.getChildCount(); ++index)
    {
        const UI::AUIElement* child = node.getChild(index);
        if (child == nullptr)
            continue;

        snapshot.children.push_back(buildSnapshot(*child, node.getHierarchicalIdForChild(index, path)));
    }

    return snapshot;
}

bool UIBinder::applyTargetedChanges(
    Rml::ElementDocument* document,
    const SnapshotNode& previous,
    const SnapshotNode& current,
    ApplySummary& summary) const
{
    if (previous.childCount != current.childCount)
        return applyNodeUpdate(document, current, summary);

    if (previous.markup == current.markup)
        return true;

    if (previous.children.size() != current.children.size())
        return applyNodeUpdate(document, current, summary);

    bool hadUnsupportedChange = false;
    for (std::size_t index = 0; index < current.children.size(); ++index)
    {
        if (!applyTargetedChanges(document, previous.children[index], current.children[index], summary))
            hadUnsupportedChange = true;
    }

    if (hadUnsupportedChange)
        return false;

    if (previous.markup != current.markup)
        return applyElementPatch(document, previous, current, summary);

    return true;
}

bool UIBinder::applyNodeUpdate(
    Rml::ElementDocument* document,
    const SnapshotNode& current,
    ApplySummary& summary) const
{
    if (document == nullptr)
        return false;

    Rml::Element* element = document->GetElementById(current.domId);
    if (element == nullptr)
        return false;

    element->SetInnerRML(current.childrenMarkup);
    summary.appliedTargetedUpdates = true;
    ++summary.targetedUpdateCount;
    return true;
}

bool UIBinder::applyElementPatch(
    Rml::ElementDocument* document,
    const SnapshotNode& previous,
    const SnapshotNode& current,
    ApplySummary& summary) const
{
    if (document == nullptr || !current.canPatchElement)
    {
        summary.unsupportedPaths.push_back(current.path);
        return false;
    }

    Rml::Element* element = document->GetElementById(current.domId);
    if (element == nullptr)
    {
        summary.unsupportedPaths.push_back(current.path);
        return false;
    }

    for (const auto& [key, value] : current.attributes)
    {
        if (key == "id")
            continue;

        const auto previousIt = previous.attributes.find(key);
        if (previousIt == previous.attributes.end() || previousIt->second != value)
            element->SetAttribute(key, value);
    }

    for (const auto& [key, previousValue] : previous.attributes)
    {
        if (key == "id")
            continue;

        if (current.attributes.find(key) == current.attributes.end())
            element->SetAttribute(key, "");
    }

    if (current.childCount == 0 && previous.innerMarkup != current.innerMarkup)
        element->SetInnerRML(current.innerMarkup);

    summary.appliedTargetedUpdates = true;
    ++summary.targetedUpdateCount;
    return true;
}