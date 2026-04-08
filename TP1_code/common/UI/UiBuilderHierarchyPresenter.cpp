#include <common/UI/UiBuilderHierarchyPresenter.hpp>

#include <common/UI/MarkupBlock.hpp>
#include <common/UI/Panel.hpp>
#include <common/UI/PanelHeader.hpp>
#include <common/UI/UiBuilderIdCodec.hpp>

#include <sstream>

namespace UI
{
std::string UiBuilderHierarchyPresenter::buildHierarchyMarkup(
    const UiBuilderHierarchyModel& hierarchyModel,
    DragPayloadKind dragPayloadKind,
    int draggedHierarchyNodeId,
    int dropTargetNodeId,
    HierarchyDropMode dropMode,
    const ContextMenuState& contextMenuState) const
{
    Panel shell(0, 0);
    shell.addClassName("hierarchy_shell");
    shell.addContentClassName("hierarchy_body");

    PanelHeader header(0, 0, "Hierarchy");
    MarkupBlock hierarchyBody(0, 0, buildHierarchyNodeMarkup(
        hierarchyModel.root(),
        hierarchyModel,
        dragPayloadKind,
        draggedHierarchyNodeId,
        dropTargetNodeId,
        dropMode,
        0));
    MarkupBlock contextMenu(0, 0, buildHierarchyContextMenuMarkup(hierarchyModel, contextMenuState));

    shell.addChild(&header);
    shell.addChild(&hierarchyBody);
    shell.addChild(&contextMenu);
    return shell.getRML();
}

std::string UiBuilderHierarchyPresenter::buildHierarchyNodeMarkup(
    const UiBuilderHierarchyModel::Node& node,
    const UiBuilderHierarchyModel& hierarchyModel,
    DragPayloadKind dragPayloadKind,
    int draggedHierarchyNodeId,
    int dropTargetNodeId,
    HierarchyDropMode dropMode,
    int depth) const
{
    const bool isSelected = node.id == hierarchyModel.selectedNodeId();
    const bool dropBefore = dropTargetNodeId == node.id && dropMode == HierarchyDropMode::Before;
    const bool dropInside = dropTargetNodeId == node.id && dropMode == HierarchyDropMode::Inside;
    const bool dropAfter = dropTargetNodeId == node.id && dropMode == HierarchyDropMode::After;

    std::ostringstream stream;
    stream << "<div class='hierarchy_node depth_" << depth << "'>";
    stream << "<div id='" << UiBuilderIdCodec::makeHierarchyDropElementId(node.id, HierarchyDropMode::Before) << "' class='hierarchy_drop_zone";
    if (dropBefore)
        stream << " drop_active";
    stream << "'></div>";
    stream << "<div id='" << UiBuilderIdCodec::makeHierarchyNodeElementId(node.id) << "' class='hierarchy_row";
    if (isSelected)
        stream << " selected";
    if (dropInside)
        stream << " drop_active";
    if (dragPayloadKind == DragPayloadKind::Node && draggedHierarchyNodeId == node.id)
        stream << " dragging";
    stream << "'>";
    stream << "<div class='hierarchy_label'>" << node.label << "</div>";
    stream << "<div class='hierarchy_meta'>&lt;" << node.tagName << "&gt;</div>";
    stream << "</div>";

    if (!node.children.empty())
    {
        stream << "<div class='hierarchy_children'>";
        for (const UiBuilderHierarchyModel::Node& child : node.children)
        {
            stream << buildHierarchyNodeMarkup(child, hierarchyModel, dragPayloadKind, draggedHierarchyNodeId, dropTargetNodeId, dropMode, depth + 1);
        }
        stream << "</div>";
    }

    stream << "<div id='" << UiBuilderIdCodec::makeHierarchyDropElementId(node.id, HierarchyDropMode::After) << "' class='hierarchy_drop_zone";
    if (dropAfter)
        stream << " drop_active";
    stream << "'></div>";
    stream << "</div>";
    return stream.str();
}

std::string UiBuilderHierarchyPresenter::buildHierarchyContextMenuMarkup(
    const UiBuilderHierarchyModel& hierarchyModel,
    const ContextMenuState& contextMenuState) const
{
    if (!contextMenuState.open || contextMenuState.nodeId == 0 || contextMenuState.nodeId == hierarchyModel.root().id)
        return "";

    const bool canMoveUpValue = hierarchyModel.canMoveNodeUp(contextMenuState.nodeId);
    const bool canMoveDownValue = hierarchyModel.canMoveNodeDown(contextMenuState.nodeId);

    std::ostringstream stream;
    stream << "<div class='hierarchy_context_menu' style='left: " << contextMenuState.x << "px; top: " << contextMenuState.y << "px;'>";
    stream << "<div id='hierarchy_context_move_up' class='hierarchy_context_item";
    if (!canMoveUpValue)
        stream << " disabled";
    stream << "'>Move up</div>";
    stream << "<div id='hierarchy_context_move_down' class='hierarchy_context_item";
    if (!canMoveDownValue)
        stream << " disabled";
    stream << "'>Move down</div>";
    stream << "<div id='hierarchy_context_delete' class='hierarchy_context_item danger'>Delete</div>";
    stream << "</div>";
    return stream.str();
}
}