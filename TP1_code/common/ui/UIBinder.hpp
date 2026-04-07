#pragma once

#include <RmlUi/Core.h>

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include <common/UI/AUIElement.hpp>

class UIBinder
{
public:
    struct ApplySummary
    {
        bool rebuiltRoot = false;
        bool appliedTargetedUpdates = false;
        bool hasUnsupportedChanges = false;
        std::size_t targetedUpdateCount = 0;
        std::vector<std::string> unsupportedPaths;
    };

    UIBinder() = default;
    ~UIBinder() = default;

    ApplySummary apply(Rml::Element* mountPoint, const UI::AUIElement& root);
    void clear();
    bool hasSnapshot() const;

private:
    struct SnapshotNode
    {
        std::string path;
        std::string domId;
        std::string markup;
        std::string childrenMarkup;
        std::string innerMarkup;
        std::size_t childCount = 0;
        bool canPatchElement = false;
        std::unordered_map<std::string, std::string> attributes;
        std::vector<SnapshotNode> children;
    };

    SnapshotNode buildSnapshot(const UI::AUIElement& node, const std::string& path) const;
    bool applyTargetedChanges(
        Rml::ElementDocument* document,
        const SnapshotNode& previous,
        const SnapshotNode& current,
        ApplySummary& summary) const;
    bool applyNodeUpdate(
        Rml::ElementDocument* document,
        const SnapshotNode& current,
        ApplySummary& summary) const;
    bool applyElementPatch(
        Rml::ElementDocument* document,
        const SnapshotNode& previous,
        const SnapshotNode& current,
        ApplySummary& summary) const;

    SnapshotNode m_previousRoot;
    bool m_hasSnapshot = false;
};