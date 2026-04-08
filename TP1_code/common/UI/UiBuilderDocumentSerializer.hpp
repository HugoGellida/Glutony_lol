#pragma once

#include <common/UI/UiBuilderHierarchyModel.hpp>

#include <string>

namespace UI
{
class UiBuilderDocumentSerializer
{
public:
    std::string buildPreviewDocumentSourceFromHierarchy(const UiBuilderHierarchyModel& hierarchyModel) const;
    std::string buildRenderDocumentSourceFromHierarchy(const UiBuilderHierarchyModel& hierarchyModel) const;
    std::string buildExportDocumentSourceFromHierarchy(const UiBuilderHierarchyModel& hierarchyModel) const;

private:
    std::string buildExportNodeMarkup(const UiBuilderHierarchyModel::Node& node, int depth) const;
};
}