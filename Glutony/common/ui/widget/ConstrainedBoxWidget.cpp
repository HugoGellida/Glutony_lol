#include "ConstrainedBoxWidget.hpp"

namespace ui::widget
{
ConstrainedBoxWidget::ConstrainedBoxWidget()
    : ConstrainedBoxWidget(
        "constrained-box",
        "Container",
        "Generic div container with standard box-model properties and nested children.")
{
}

ConstrainedBoxWidget::ConstrainedBoxWidget(std::string key, std::string name, std::string description)
    : Widget(
        std::move(key),
        std::move(name),
        std::move(description))
{
}

std::string ConstrainedBoxWidget::tagName() const
{
    return "div";
}

bool ConstrainedBoxWidget::canHaveEditorChildren() const
{
    return true;
}

bool ConstrainedBoxWidget::matchesElement(const Rml::Element& element) const
{
    if (element.GetTagName() != "div")
        return false;

    if (hasClassName(element, "widget_stack") || hasClassName(element, "widget_scroll_region") || hasClassName(element, "widget_spacer"))
        return false;

    const WidgetPropertyMap declarations = parseInlineStyle(element.GetAttribute<Rml::String>("style", "").c_str());
    if (const auto displayIt = declarations.find("display"); displayIt != declarations.end() && displayIt->second == "flex")
        return false;
    if (declarations.find("overflow-y") != declarations.end() || declarations.find("overflow-x") != declarations.end())
        return false;

    return true;
}

WidgetPropertyMap ConstrainedBoxWidget::capturePropertiesFromElement(const Rml::Element& element) const
{
    WidgetPropertyMap properties;
    const WidgetPropertyMap declarations = parseInlineStyle(element.GetAttribute<Rml::String>("style", "").c_str());

    if (const auto widthIt = declarations.find("width"); widthIt != declarations.end())
        properties["width"] = widthIt->second;
    if (const auto heightIt = declarations.find("height"); heightIt != declarations.end())
        properties["height"] = heightIt->second;
    if (const auto paddingIt = declarations.find("padding"); paddingIt != declarations.end())
        properties["padding"] = paddingIt->second;
    if (const auto marginLeftIt = declarations.find("margin-left"); marginLeftIt != declarations.end())
        properties["margin-left"] = marginLeftIt->second;
    if (const auto marginRightIt = declarations.find("margin-right"); marginRightIt != declarations.end())
        properties["margin-right"] = marginRightIt->second;
    if (const auto borderWidthIt = declarations.find("border-width"); borderWidthIt != declarations.end())
        properties["border-width"] = borderWidthIt->second;
    if (const auto borderColorIt = declarations.find("border-color"); borderColorIt != declarations.end())
        properties["border-color"] = borderColorIt->second;
    if (const auto backgroundColorIt = declarations.find("background-color"); backgroundColorIt != declarations.end())
        properties["background-color"] = backgroundColorIt->second;
    if (const auto borderIt = declarations.find("border"); borderIt != declarations.end())
    {
        if (borderIt->second.find("0px") != std::string::npos && properties.find("border-width") == properties.end())
            properties["border-width"] = "0px";
        if (borderIt->second.find('#') != std::string::npos && properties.find("border-color") == properties.end())
            properties["border-color"] = borderIt->second.substr(borderIt->second.find('#'));
    }

    captureFlexItemProperties(element, properties);

    return properties;
}

std::string ConstrainedBoxWidget::buildMarkup(
    const std::string&,
    const WidgetPropertyMap& properties,
    const std::vector<std::string>& childMarkup,
    int depth,
    DocumentBuildMode mode) const
{
    const std::string indentation = indent(depth);
    const std::string children = joinChildren(childMarkup);
    const std::string width = propertyValue(properties, "width", "100%");
    const std::string height = propertyValue(properties, "height", "auto");
    const std::string padding = propertyValue(properties, "padding", "12px");
    const std::string marginLeft = propertyValue(properties, "margin-left", "0px");
    const std::string marginRight = propertyValue(properties, "margin-right", "0px");
    const std::string borderWidth = propertyValue(properties, "border-width", "1px");
    const std::string borderColor = propertyValue(properties, "border-color", "#ccbfa9");
    const std::string backgroundColor = propertyValue(properties, "background-color", "#fff9ef");

    std::vector<std::pair<std::string, std::string>> styleOverrides;
    if (width != "100%")
        styleOverrides.emplace_back("width", width);
    if (height != "auto")
        styleOverrides.emplace_back("height", height);
    if (padding != "12px")
        styleOverrides.emplace_back("padding", ensureUnit(padding));
    if (marginLeft != "0px")
        styleOverrides.emplace_back("margin-left", ensureUnit(marginLeft));
    if (marginRight != "0px")
        styleOverrides.emplace_back("margin-right", ensureUnit(marginRight));
    if (borderWidth != "1px")
        styleOverrides.emplace_back("border-width", ensureUnit(borderWidth));
    if (borderColor != "#ccbfa9")
        styleOverrides.emplace_back("border-color", borderColor);
    if (backgroundColor != "#fff9ef")
        styleOverrides.emplace_back("background-color", backgroundColor);
    appendFlexItemStyleOverrides(properties, styleOverrides);

    const std::string styleAttribute = buildStyleAttribute(styleOverrides);

    (void)mode;
    return indentation + "<div class='widget_container'" + styleAttribute + ">\n" +
        children +
        indentation + "</div>\n";
}

std::string ConstrainedBoxWidget::buildStyleRules() const
{
    return ".widget_container { display: block; width: 100%; min-width: 0px; min-height: 0px; padding: 12px; box-sizing: border-box; border: 1px #ccbfa9; background-color: #fff9ef; }\n";
}

InspectorModel ConstrainedBoxWidget::buildInspectorModel(const std::string&, const WidgetPropertyMap& properties) const
{
    return {
        {
            InspectorField("width", "Width", "100%", propertyValue(properties, "width", "100%")),
            InspectorField("height", "Height", "auto", propertyValue(properties, "height", "auto")),
            InspectorField("padding", "Padding", "12px", propertyValue(properties, "padding", "12px")),
            InspectorField("margin-left", "Margin Left", "0px", propertyValue(properties, "margin-left", "0px")),
            InspectorField("margin-right", "Margin Right", "0px", propertyValue(properties, "margin-right", "0px")),
            InspectorField("border-width", "Border Width", "1px", propertyValue(properties, "border-width", "1px")),
            InspectorField("border-color", "Border Color", "#ccbfa9", propertyValue(properties, "border-color", "#ccbfa9")),
            InspectorField("background-color", "Background Color", "#fff9ef", propertyValue(properties, "background-color", "#fff9ef"))
        },
        {
            buildFlexItemInspectorGroup(properties)
        }
    };
}

std::string ConstrainedBoxWidget::buildInspectorMarkup(const std::string& label) const
{
    return std::string("<div class='placeholder_block'><div class='placeholder_title'>") +
        escapeText(label) +
        "</div><div class='placeholder_text'>Container widget with editable constraints and child content. Children remain visible in the hierarchy.</div></div>";
}

} // namespace ui::widget