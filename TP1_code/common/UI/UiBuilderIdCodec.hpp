#pragma once

#include <RmlUi/Core.h>

#include <common/ui/EditorUiCommon.hpp>

#include <optional>
#include <string>
#include <utility>

namespace UI
{
class UiBuilderIdCodec
{
public:
    static std::string makeHierarchyNodeElementId(int nodeId)
    {
        return "hierarchy_node_" + std::to_string(nodeId);
    }

    static std::string makeHierarchyDropElementId(int nodeId, HierarchyDropMode dropMode)
    {
        const char* suffix = "inside";
        if (dropMode == HierarchyDropMode::Before)
            suffix = "before";
        else if (dropMode == HierarchyDropMode::After)
            suffix = "after";

        return "hierarchy_drop_" + std::to_string(nodeId) + "_" + suffix;
    }

    static std::string makeInspectorFieldElementId(int nodeId, const std::string& fieldKey)
    {
        return "inspector_field_" + std::to_string(nodeId) + "__" + fieldKey;
    }

    static std::optional<int> parseHierarchyNodeId(const Rml::String& elementId)
    {
        const std::string value = elementId;
        const std::string prefix = "hierarchy_node_";
        if (!editor_ui::startsWith(value, prefix))
            return std::nullopt;

        return std::stoi(value.substr(prefix.size()));
    }

    static std::optional<std::pair<int, HierarchyDropMode>> parseHierarchyDropId(const Rml::String& elementId)
    {
        const std::string value = elementId;
        const std::string prefix = "hierarchy_drop_";
        if (!editor_ui::startsWith(value, prefix))
            return std::nullopt;

        const std::size_t separator = value.find('_', prefix.size());
        if (separator == std::string::npos)
            return std::nullopt;

        const int nodeId = std::stoi(value.substr(prefix.size(), separator - prefix.size()));
        const std::string suffix = value.substr(separator + 1);
        if (suffix == "before")
            return std::make_pair(nodeId, HierarchyDropMode::Before);
        if (suffix == "after")
            return std::make_pair(nodeId, HierarchyDropMode::After);
        if (suffix == "inside")
            return std::make_pair(nodeId, HierarchyDropMode::Inside);
        return std::nullopt;
    }

    static std::optional<std::pair<int, std::string>> parseInspectorFieldElementId(const Rml::String& elementId)
    {
        const std::string value = elementId;
        const std::string prefix = "inspector_field_";
        if (!editor_ui::startsWith(value, prefix))
            return std::nullopt;

        const std::size_t separator = value.find("__", prefix.size());
        if (separator == std::string::npos)
            return std::nullopt;

        const int nodeId = std::stoi(value.substr(prefix.size(), separator - prefix.size()));
        return std::make_pair(nodeId, value.substr(separator + 2));
    }
};
}