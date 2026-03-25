#include "ButtonWidget.hpp"

namespace ui::widget
{
ButtonWidget::ButtonWidget()
    : ConstrainedBoxWidget(
        "button",
        "Button",
        "Interactive button element backed by a real RmlUi button tag.")
{
}

std::string ButtonWidget::tagName() const
{
    return "button";
}

std::string ButtonWidget::defaultLabel() const
{
    return "Button";
}

bool ButtonWidget::canHaveEditorChildren() const
{
    return false;
}

bool ButtonWidget::isCompositeTemplate() const
{
    return false;
}

std::optional<std::string> ButtonWidget::primaryTextPropertyKey() const
{
    return std::string("content");
}

bool ButtonWidget::matchesElement(const Rml::Element& element) const
{
    return element.GetTagName() == "button";
}

std::string ButtonWidget::labelFromElement(const Rml::Element& element) const
{
    return element.GetInnerRML().empty() ? defaultLabel() : std::string(element.GetInnerRML().c_str());
}

WidgetPropertyMap ButtonWidget::capturePropertiesFromElement(const Rml::Element& element) const
{
    WidgetPropertyMap properties;
    const WidgetPropertyMap declarations = parseInlineStyle(element.GetAttribute<Rml::String>("style", "").c_str());
    if (const auto widthIt = declarations.find("width"); widthIt != declarations.end())
        properties["width"] = widthIt->second;
    if (const auto heightIt = declarations.find("height"); heightIt != declarations.end())
        properties["height"] = heightIt->second;
    if (const auto paddingIt = declarations.find("padding"); paddingIt != declarations.end())
        properties["padding"] = paddingIt->second;
    if (const auto borderWidthIt = declarations.find("border-width"); borderWidthIt != declarations.end())
        properties["border-width"] = borderWidthIt->second;
    if (const auto borderColorIt = declarations.find("border-color"); borderColorIt != declarations.end())
        properties["border-color"] = borderColorIt->second;
    if (const auto backgroundIt = declarations.find("background-color"); backgroundIt != declarations.end())
        properties["background-color"] = backgroundIt->second;
    if (const auto colorIt = declarations.find("color"); colorIt != declarations.end())
        properties["color"] = colorIt->second;
    if (const auto fontIt = declarations.find("font-family"); fontIt != declarations.end())
        properties["font-family"] = fontIt->second;
    captureFlexItemProperties(element, properties);

    return properties;
}

std::string ButtonWidget::buildMarkup(
    const std::string& label,
    const WidgetPropertyMap& properties,
    const std::vector<std::string>&,
    int depth,
    DocumentBuildMode mode) const
{
    const std::string indentation = indent(depth);
    const std::string escapedLabel = escapeText(label);
    const std::string width = propertyValue(properties, "width", "auto");
    const std::string height = propertyValue(properties, "height", "auto");
    const std::string padding = propertyValue(properties, "padding", "10px 14px");
    const std::string borderWidth = propertyValue(properties, "border-width", "0px");
    const std::string borderColor = propertyValue(properties, "border-color", "transparent");
    const std::string background = propertyValue(properties, "background-color", "#172028");
    const std::string fontFamily = propertyValue(properties, "font-family", "LatoLatin");
    const std::string color = propertyValue(properties, "color", "#f8f4ea");

    std::vector<std::pair<std::string, std::string>> buttonStyle;
    if (width != "auto")
        buttonStyle.emplace_back("width", width);
    if (height != "auto")
        buttonStyle.emplace_back("height", height);
    if (padding != "10px 14px")
        buttonStyle.emplace_back("padding", padding);
    if (borderWidth != "0px")
        buttonStyle.emplace_back("border-width", ensureUnit(borderWidth));
    if (borderColor != "transparent")
        buttonStyle.emplace_back("border-color", borderColor);
    if (background != "#172028")
        buttonStyle.emplace_back("background-color", background);
    if (fontFamily != "LatoLatin")
        buttonStyle.emplace_back("font-family", fontFamily);
    if (color != "#f8f4ea")
        buttonStyle.emplace_back("color", color);
    appendFlexItemStyleOverrides(properties, buttonStyle);

    const std::string buttonStyleAttribute = buildStyleAttribute(buttonStyle);

    (void)mode;
    return indentation + "<button class='widget_button'" + buttonStyleAttribute + ">" + escapedLabel + "</button>\n";
}

std::string ButtonWidget::buildStyleRules() const
{
    return ".widget_button { display: inline-block; width: auto; height: auto; min-width: 0px; min-height: 0px; padding: 10px 14px; background-color: #172028; border: 0px transparent; color: #f8f4ea; font-family: LatoLatin; }\n"
        ".widget_button:hover { background-color: #22303a; }\n";
}

InspectorModel ButtonWidget::buildInspectorModel(const std::string& label, const WidgetPropertyMap& properties) const
{
    return {
        {
            InspectorField("content", "Content", defaultLabel(), label),
            InspectorField("width", "Width", "auto", propertyValue(properties, "width", "auto")),
            InspectorField("height", "Height", "auto", propertyValue(properties, "height", "auto")),
            InspectorField("padding", "Padding", "10px 14px", propertyValue(properties, "padding", "10px 14px")),
            InspectorField("border-width", "Border Width", "0px", propertyValue(properties, "border-width", "0px")),
            InspectorField("border-color", "Border Color", "transparent", propertyValue(properties, "border-color", "transparent")),
            InspectorField("background-color", "Background Color", "#172028", propertyValue(properties, "background-color", "#172028")),
            InspectorField("color", "Color", "#f8f4ea", propertyValue(properties, "color", "#f8f4ea")),
            InspectorField("font-family", "Font Family", "LatoLatin", propertyValue(properties, "font-family", "LatoLatin"))
        },
        {
            buildFlexItemInspectorGroup(properties)
        }
    };
}

std::string ButtonWidget::buildInspectorMarkup(const std::string& label) const
{
    return std::string("<div class='placeholder_block'><div class='placeholder_title'>Button</div><div class='placeholder_text'>Real button element. Current label: ") +
        escapeText(label) +
        "</div></div>";
}

} // namespace ui::widget