#include <common/UI/UiBuilderInspectorPresenter.hpp>

#include <common/UI/UiBuilderIdCodec.hpp>
#include <common/UI/UiBuilderPanelMarkupFactory.hpp>

#include <common/ui/EditorUiCommon.hpp>
#include <common/ui/widget/WidgetRegistry.hpp>

#include <sstream>

namespace UI
{
std::string UiBuilderInspectorPresenter::buildInspectorPanelMarkup(
    const UiBuilderHierarchyModel& hierarchyModel,
    const std::unordered_set<std::string>& collapsedInspectorGroups) const
{
    const UiBuilderHierarchyModel::Node* selectedNode = hierarchyModel.findSelectedNode();
    if (selectedNode == nullptr || selectedNode->id == hierarchyModel.root().id)
    {
        return UiBuilderPanelMarkupFactory::buildPlaceholderPanelMarkup(
            "Inspector",
            "Root",
            "Select a non-root widget to inspect its metadata and widget-specific settings.");
    }

    const ui::widget::Widget* widget = ui::widget::findWidgetByKey(selectedNode->elementKey);
    if (widget == nullptr)
    {
        const std::string fallback = std::string("<div class='placeholder_block'><div class='placeholder_title'>") +
            editor_ui::escapeRmlText(selectedNode->label) +
            "</div><div class='placeholder_text'>Unknown widget type.</div></div>";
        return UiBuilderPanelMarkupFactory::buildPanelShellMarkup("Inspector", fallback, "inspector_panel_body");
    }

    const ui::widget::InspectorModel model = widget->buildInspectorModel(selectedNode->label, selectedNode->properties);
    const auto buildFieldMarkup = [&](const ui::widget::InspectorField& field) {
        std::ostringstream fieldStream;
        fieldStream << "<div class='inspector_field_row'><div class='inspector_field_name'>" << editor_ui::escapeRmlText(field.label) << "</div>";
        const std::string fieldId = UiBuilderIdCodec::makeInspectorFieldElementId(selectedNode->id, field.key);
        if (field.inputKind == ui::widget::InspectorInputKind::Enum)
        {
            fieldStream << "<select id='" << fieldId << "' class='inspector_field_input'>";
            for (const ui::widget::InspectorOption& option : field.options)
            {
                fieldStream << "<option value='" << editor_ui::escapeRmlText(option.value) << "'";
                if (option.value == field.value)
                    fieldStream << " selected='selected'";
                fieldStream << ">" << editor_ui::escapeRmlText(option.label) << "</option>";
            }
            fieldStream << "</select>";
        }
        else
        {
            fieldStream << "<input id='" << fieldId << "' class='inspector_field_input' type='";
            fieldStream << (field.inputKind == ui::widget::InspectorInputKind::Number ? "number" : "text");
            fieldStream << "' value='" << editor_ui::escapeRmlText(field.value) << "' />";
        }
        fieldStream << "</div>";
        return fieldStream.str();
    };

    std::ostringstream bodyStream;
    bodyStream << "<div class='inspector_summary'>";
    bodyStream << "<div class='inspector_summary_title'>" << editor_ui::escapeRmlText(widget->name()) << "</div>";
    bodyStream << "<div class='inspector_summary_text'>" << editor_ui::escapeRmlText(widget->description()) << "</div>";
    bodyStream << "</div>";

    if (!model.fields.empty())
    {
        bodyStream << "<div class='inspector_section'>";
        for (const ui::widget::InspectorField& field : model.fields)
            bodyStream << buildFieldMarkup(field);
        bodyStream << "</div>";
    }

    for (const ui::widget::InspectorGroup& group : model.childGroups)
    {
        const std::string groupId = "node_" + std::to_string(selectedNode->id) + "_" + group.key;
        const bool collapsed = collapsedInspectorGroups.find(groupId) != collapsedInspectorGroups.end();
        bodyStream << "<div class='inspector_foldout'>";
        bodyStream << "<div id='inspector_group_toggle_" << groupId << "' class='inspector_foldout_header'>";
        bodyStream << "<div class='inspector_foldout_icon'>" << (collapsed ? ">" : "v") << "</div>";
        bodyStream << "<div class='inspector_foldout_title'>" << editor_ui::escapeRmlText(group.title) << "</div>";
        bodyStream << "</div>";
        if (!collapsed)
        {
            bodyStream << "<div class='inspector_foldout_body'>";
            for (const ui::widget::InspectorField& field : group.fields)
                bodyStream << buildFieldMarkup(field);
            bodyStream << "</div>";
        }
        bodyStream << "</div>";
    }

    return UiBuilderPanelMarkupFactory::buildPanelShellMarkup("Inspector", bodyStream.str(), "inspector_panel_body");
}
}