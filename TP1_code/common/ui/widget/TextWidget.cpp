#include "TextWidget.hpp"

namespace ui::widget
{
TextWidget::TextWidget()
    : Widget(
        "text",
        "Text",
        "Plain text element backed by a real paragraph tag and standard text properties.")
{
}

std::string TextWidget::tagName() const
{
    return "p";
}

std::optional<std::string> TextWidget::primaryTextPropertyKey() const
{
    return std::string("content");
}

bool TextWidget::matchesElement(const Rml::Element& element) const
{
    return element.GetTagName() == "p" || element.GetTagName() == "span";
}

std::string TextWidget::labelFromElement(const Rml::Element& element) const
{
    return element.GetInnerRML().empty() ? defaultLabel() : std::string(element.GetInnerRML().c_str());
}

WidgetPropertyMap TextWidget::capturePropertiesFromElement(const Rml::Element& element) const
{
    WidgetPropertyMap properties;
    const WidgetPropertyMap declarations = parseInlineStyle(element.GetAttribute<Rml::String>("style", "").c_str());
    if (const auto fontIt = declarations.find("font-family"); fontIt != declarations.end())
        properties["font-family"] = fontIt->second;
    if (const auto colorIt = declarations.find("color"); colorIt != declarations.end())
        properties["color"] = colorIt->second;
    if (const auto alignIt = declarations.find("text-align"); alignIt != declarations.end())
        properties["text-align"] = alignIt->second;
    captureFlexItemProperties(element, properties);
    return properties;
}

std::string TextWidget::buildMarkup(
    const std::string& label,
    const WidgetPropertyMap& properties,
    const std::vector<std::string>&,
    int depth,
    DocumentBuildMode mode) const
{
    const std::string indentation = indent(depth);
    const std::string escapedLabel = escapeText(label);
    std::vector<std::pair<std::string, std::string>> styleOverrides;
    const std::string fontFamily = propertyValue(properties, "font-family", "LatoLatin");
    const std::string color = propertyValue(properties, "color", "#172028");
    const std::string textAlign = propertyValue(properties, "text-align", "left");
    if (fontFamily != "LatoLatin")
        styleOverrides.emplace_back("font-family", fontFamily);
    if (color != "#172028")
        styleOverrides.emplace_back("color", color);
    if (textAlign != "left")
        styleOverrides.emplace_back("text-align", textAlign);
    appendFlexItemStyleOverrides(properties, styleOverrides);
    const std::string styleAttribute = buildStyleAttribute(styleOverrides);

    (void)mode;
    return indentation + "<p class='widget_text'" + styleAttribute + ">" + escapedLabel + "</p>\n";
}

std::string TextWidget::buildStyleRules() const
{
    return ".widget_text { display: block; min-width: 0px; min-height: 0px; margin: 0px 0px 12px 0px; color: #172028; font-family: LatoLatin; text-align: left; }\n";
}

InspectorModel TextWidget::buildInspectorModel(const std::string& label, const WidgetPropertyMap& properties) const
{
    return {
        {
            InspectorField("content", "Content", defaultLabel(), label),
            InspectorField("text-align", "Text Align", "left", propertyValue(properties, "text-align", "left"), InspectorInputKind::Enum, {{"left", "Left"}, {"center", "Center"}, {"right", "Right"}}),
            InspectorField("font-family", "Font Family", "LatoLatin", propertyValue(properties, "font-family", "LatoLatin")),
            InspectorField("color", "Color", "#172028", propertyValue(properties, "color", "#172028"))
        },
        {
            buildFlexItemInspectorGroup(properties)
        }
    };
}

std::string TextWidget::buildInspectorMarkup(const std::string& label) const
{
    return std::string("<div class='placeholder_block'><div class='placeholder_title'>Text</div><div class='placeholder_text'>Current text: ") +
        escapeText(label) +
        "</div></div>";
}

} // namespace ui::widget