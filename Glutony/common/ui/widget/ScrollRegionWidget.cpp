#include "ScrollRegionWidget.hpp"

namespace ui::widget
{

ScrollRegionWidget::ScrollRegionWidget()
    : Widget(
        "scroll-region",
        "Scroll Region",
        "Clipped container that enables scrolling when content exceeds bounds.")
{
}

std::string ScrollRegionWidget::tagName() const
{
    return "div";
}

bool ScrollRegionWidget::canHaveEditorChildren() const
{
    return true;
}

bool ScrollRegionWidget::matchesElement(const Rml::Element& element) const
{
    if (element.GetTagName() != "div")
        return false;

    if (hasClassName(element, "widget_scroll_region"))
        return true;

    const WidgetPropertyMap declarations = parseInlineStyle(element.GetAttribute<Rml::String>("style", "").c_str());
    return declarations.find("overflow-y") != declarations.end() || declarations.find("overflow-x") != declarations.end();
}

WidgetPropertyMap ScrollRegionWidget::capturePropertiesFromElement(const Rml::Element& element) const
{
    WidgetPropertyMap properties;
    const WidgetPropertyMap declarations = parseInlineStyle(element.GetAttribute<Rml::String>("style", "").c_str());
    if (const auto widthIt = declarations.find("width"); widthIt != declarations.end())
        properties["width"] = widthIt->second;
    if (const auto heightIt = declarations.find("height"); heightIt != declarations.end())
        properties["height"] = heightIt->second;
    if (const auto overflowYIt = declarations.find("overflow-y"); overflowYIt != declarations.end())
        properties["overflow-y"] = overflowYIt->second;
    if (const auto overflowXIt = declarations.find("overflow-x"); overflowXIt != declarations.end())
        properties["overflow-x"] = overflowXIt->second;
    if (const auto paddingIt = declarations.find("padding"); paddingIt != declarations.end())
        properties["padding"] = paddingIt->second;
    if (const auto borderWidthIt = declarations.find("border-width"); borderWidthIt != declarations.end())
        properties["border-width"] = borderWidthIt->second;
    if (const auto borderColorIt = declarations.find("border-color"); borderColorIt != declarations.end())
        properties["border-color"] = borderColorIt->second;
    if (const auto backgroundIt = declarations.find("background-color"); backgroundIt != declarations.end())
        properties["background-color"] = backgroundIt->second;
    captureFlexItemProperties(element, properties);
    return properties;
}

std::string ScrollRegionWidget::buildMarkup(
    const std::string&,
    const WidgetPropertyMap& properties,
    const std::vector<std::string>& childMarkup,
    int depth,
    DocumentBuildMode mode) const
{
    const std::string indentation = indent(depth);
    const std::string children = joinChildren(childMarkup);
    const std::string width = propertyValue(properties, "width", "100%");
    const std::string height = propertyValue(properties, "height", "140px");
    const std::string overflowY = propertyValue(properties, "overflow-y", "auto");
    const std::string overflowX = propertyValue(properties, "overflow-x", "hidden");
    const std::string padding = propertyValue(properties, "padding", "12px");
    const std::string borderWidth = propertyValue(properties, "border-width", "1px");
    const std::string borderColor = propertyValue(properties, "border-color", "#ccbfa9");
    const std::string backgroundColor = propertyValue(properties, "background-color", "#fff9ef");
    std::vector<std::pair<std::string, std::string>> styleOverrides = {
        {"width", width == "100%" ? "" : width},
        {"height", height == "140px" ? "" : ensureUnit(height)},
        {"overflow-y", overflowY == "auto" ? "" : overflowY},
        {"overflow-x", overflowX == "hidden" ? "" : overflowX},
        {"padding", padding == "12px" ? "" : ensureUnit(padding)},
        {"border-width", borderWidth == "1px" ? "" : ensureUnit(borderWidth)},
        {"border-color", borderColor == "#ccbfa9" ? "" : borderColor},
        {"background-color", backgroundColor == "#fff9ef" ? "" : backgroundColor}
    };
    appendFlexItemStyleOverrides(properties, styleOverrides);
    const std::string styleAttribute = buildStyleAttribute(styleOverrides);

    (void)mode;
    return indentation + "<div class='widget_scroll_region'" + styleAttribute + ">\n" +
        children +
        indentation + "</div>\n";
}

std::string ScrollRegionWidget::buildStyleRules() const
{
    return ".widget_scroll_region { display: block; width: 100%; height: 140px; min-width: 0px; min-height: 0px; overflow-y: auto; overflow-x: hidden; padding: 12px; box-sizing: border-box; border: 1px #ccbfa9; background-color: #fff9ef; }\n"
        ".widget_scroll_region scrollbarvertical { width: 12px; }\n"
        ".widget_scroll_region scrollbarhorizontal { height: 12px; }\n"
        ".widget_scroll_region scrollbarvertical slidertrack, .widget_scroll_region scrollbarhorizontal slidertrack { background-color: #141b21; }\n"
        ".widget_scroll_region scrollbarvertical sliderbar { width: 12px; min-height: 28px; margin-left: 1px; background-color: #4a6273; }\n"
        ".widget_scroll_region scrollbarhorizontal sliderbar { height: 12px; min-width: 28px; margin-top: 1px; background-color: #4a6273; }\n"
        ".widget_scroll_region scrollbarvertical sliderbar:hover, .widget_scroll_region scrollbarhorizontal sliderbar:hover { background-color: #6f8a9c; }\n";
}

InspectorModel ScrollRegionWidget::buildInspectorModel(const std::string&, const WidgetPropertyMap& properties) const
{
    return {
        {
            InspectorField("width", "Width", "100%", propertyValue(properties, "width", "100%")),
            InspectorField("height", "Height", "140px", propertyValue(properties, "height", "140px")),
            InspectorField("overflow-y", "Overflow Y", "auto", propertyValue(properties, "overflow-y", "auto"), InspectorInputKind::Enum, {{"auto", "Auto"}, {"scroll", "Scroll"}, {"hidden", "Hidden"}}),
            InspectorField("overflow-x", "Overflow X", "hidden", propertyValue(properties, "overflow-x", "hidden"), InspectorInputKind::Enum, {{"hidden", "Hidden"}, {"auto", "Auto"}, {"scroll", "Scroll"}}),
            InspectorField("padding", "Padding", "12px", propertyValue(properties, "padding", "12px")),
            InspectorField("border-width", "Border Width", "1px", propertyValue(properties, "border-width", "1px")),
            InspectorField("border-color", "Border Color", "#ccbfa9", propertyValue(properties, "border-color", "#ccbfa9")),
            InspectorField("background-color", "Background Color", "#fff9ef", propertyValue(properties, "background-color", "#fff9ef"))
        },
        {
            buildFlexItemInspectorGroup(properties)
        }
    };
}

std::string ScrollRegionWidget::buildInspectorMarkup(const std::string& label) const
{
    return std::string("<div class='placeholder_block'><div class='placeholder_title'>") +
        escapeText(label) +
        "</div><div class='placeholder_text'>Scrollable container. Composite content stays grouped under one editor node per child widget.</div></div>";
}

} // namespace ui::widget