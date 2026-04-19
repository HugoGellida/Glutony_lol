#pragma once

#include <common/UI/UiBuilderHierarchyModel.hpp>

#include <string>
#include <unordered_set>

namespace UI
{
class UiBuilderInspectorPresenter
{
public:
    std::string buildInspectorPanelMarkup(
        const UiBuilderHierarchyModel& hierarchyModel,
        const std::unordered_set<std::string>& collapsedInspectorGroups) const;
};
}