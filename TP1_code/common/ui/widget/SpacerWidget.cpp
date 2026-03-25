#include "SpacerWidget.hpp"

namespace ui::widget
{

SpacerWidget::SpacerWidget()
    : Widget(
        "spacer",
        "Spacer",
        "Invisible layout primitive used to reserve space or distribute flex-like gaps.")
{
}

std::string SpacerWidget::tagName() const
{
    return "div";
}

bool SpacerWidget::matchesElement(const Rml::Element& element) const
{
    return element.GetTagName() == "div" && hasClassName(element, "widget_spacer");
}

WidgetPropertyMap SpacerWidget::capturePropertiesFromElement(const Rml::Element& element) const
{
    WidgetPropertyMap properties;
    const WidgetPropertyMap declarations = parseInlineStyle(element.GetAttribute<Rml::String>("style", "").c_str());
    if (const auto heightIt = declarations.find("height"); heightIt != declarations.end())
        properties["height"] = heightIt->second;
    captureFlexItemProperties(element, properties);
    return properties;
}

std::string SpacerWidget::buildMarkup(
    const std::string&,
    const WidgetPropertyMap& properties,
    const std::vector<std::string>&,
    int depth,
    DocumentBuildMode mode) const
{
    const std::string indentation = indent(depth);
    const std::string size = propertyValue(properties, "height", "24px");
    std::vector<std::pair<std::string, std::string>> styleOverrides;
    if (size != "24px")
        styleOverrides.emplace_back("height", ensureUnit(size));
    appendFlexItemStyleOverrides(properties, styleOverrides);
    const std::string styleAttribute = buildStyleAttribute(styleOverrides);
    (void)mode;
    return indentation + "<div class='widget_spacer'" + styleAttribute + "></div>\n";
}

std::string SpacerWidget::buildStyleRules() const
{
    return ".widget_spacer { display: block; width: 100%; height: 24px; min-width: 0px; min-height: 0px; }\n";
}

InspectorModel SpacerWidget::buildInspectorModel(const std::string&, const WidgetPropertyMap& properties) const
{
    return {
        {
            InspectorField("height", "Height", "24px", propertyValue(properties, "height", "24px"))
        },
        {
            buildFlexItemInspectorGroup(properties)
        }
    };
}

std::string SpacerWidget::buildInspectorMarkup(const std::string&) const
{
    return "<div class='placeholder_block'><div class='placeholder_title'>Spacer</div><div class='placeholder_text'>Invisible element used to reserve room in layouts.</div></div>";
}

} // namespace ui::widget