#pragma once

#include <common/UI/UiBuilderHierarchyModel.hpp>
#include <common/ui/EditorUiCommon.hpp>

#include <string>

namespace UI
{
class UiBuilderHierarchyPresenter
{
public:
    struct ContextMenuState
    {
        bool open = false;
        int nodeId = 0;
        int x = 0;
        int y = 0;
    };

    std::string buildHierarchyMarkup(
        const UiBuilderHierarchyModel& hierarchyModel,
        DragPayloadKind dragPayloadKind,
        int draggedHierarchyNodeId,
        int dropTargetNodeId,
        HierarchyDropMode dropMode,
        const ContextMenuState& contextMenuState) const;

private:
    std::string buildHierarchyNodeMarkup(
        const UiBuilderHierarchyModel::Node& node,
        const UiBuilderHierarchyModel& hierarchyModel,
        DragPayloadKind dragPayloadKind,
        int draggedHierarchyNodeId,
        int dropTargetNodeId,
        HierarchyDropMode dropMode,
        int depth) const;

    std::string buildHierarchyContextMenuMarkup(
        const UiBuilderHierarchyModel& hierarchyModel,
        const ContextMenuState& contextMenuState) const;
};
}