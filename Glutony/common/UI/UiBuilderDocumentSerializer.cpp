#include <common/UI/UiBuilderDocumentSerializer.hpp>

#include <common/ui/widget/WidgetRegistry.hpp>

#include <sstream>
#include <vector>

namespace UI
{
std::string UiBuilderDocumentSerializer::buildPreviewDocumentSourceFromHierarchy(const UiBuilderHierarchyModel& hierarchyModel) const
{
    return buildExportDocumentSourceFromHierarchy(hierarchyModel);
}

std::string UiBuilderDocumentSerializer::buildRenderDocumentSourceFromHierarchy(const UiBuilderHierarchyModel& hierarchyModel) const
{
    return buildExportDocumentSourceFromHierarchy(hierarchyModel);
}

std::string UiBuilderDocumentSerializer::buildExportDocumentSourceFromHierarchy(const UiBuilderHierarchyModel& hierarchyModel) const
{
    std::ostringstream stream;
    stream << "<rml>\n";
    stream << "<head>\n";
    stream << "    <style>\n";
    stream << "        body { margin: 0px; width: 100%; height: 100%; padding: 16px; box-sizing: border-box; font-family: LatoLatin; background-color: #f8f4ea; color: #172028; overflow: auto; }\n";
    stream << "        button { font-family: LatoLatin; }\n";
    for (const ui::widget::Widget* widget : ui::widget::getWidgets())
        stream << "        " << widget->buildStyleRules();
    stream << "    </style>\n";
    stream << "</head>\n";
    stream << "<body>\n";
    for (const UiBuilderHierarchyModel::Node& child : hierarchyModel.root().children)
        stream << buildExportNodeMarkup(child, 1);
    stream << "</body>\n";
    stream << "</rml>\n";
    return stream.str();
}

std::string UiBuilderDocumentSerializer::buildExportNodeMarkup(const UiBuilderHierarchyModel::Node& node, int depth) const
{
    const ui::widget::Widget* widget = ui::widget::findWidgetByKey(node.elementKey);
    if (widget == nullptr)
        return "";

    std::vector<std::string> childMarkup;
    childMarkup.reserve(node.children.size());
    for (const UiBuilderHierarchyModel::Node& child : node.children)
        childMarkup.push_back(buildExportNodeMarkup(child, depth + 1));

    return widget->buildMarkup(node.label, node.properties, childMarkup, depth, ui::widget::DocumentBuildMode::Export);
}
}