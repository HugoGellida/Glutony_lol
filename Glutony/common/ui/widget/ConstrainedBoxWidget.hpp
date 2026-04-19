#pragma once

#include "Widget.hpp"

namespace ui::widget
{

class ConstrainedBoxWidget : public Widget
{
public:
    ConstrainedBoxWidget();

    std::string tagName() const override;
    bool canHaveEditorChildren() const override;
    bool matchesElement(const Rml::Element& element) const override;
    WidgetPropertyMap capturePropertiesFromElement(const Rml::Element& element) const override;
    std::string buildMarkup(
        const std::string& label,
        const WidgetPropertyMap& properties,
        const std::vector<std::string>& childMarkup,
        int depth,
        DocumentBuildMode mode) const override;
    std::string buildStyleRules() const override;
    InspectorModel buildInspectorModel(const std::string& label, const WidgetPropertyMap& properties) const override;
    std::string buildInspectorMarkup(const std::string& label) const override;

protected:
    ConstrainedBoxWidget(std::string key, std::string name, std::string description);
};

} // namespace ui::widget