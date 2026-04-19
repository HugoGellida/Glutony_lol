#include "StackLayoutWidget.hpp"

namespace ui::widget
{

StackLayoutWidget::StackLayoutWidget()
    : Widget(
        "stack",
        "Stack Layout",
        "Flex layout container with real direction, gap, alignment and justification properties.")
{
}

std::string StackLayoutWidget::tagName() const
{
    return "div";
}

bool StackLayoutWidget::canHaveEditorChildren() const
{
    return true;
}

bool StackLayoutWidget::matchesElement(const Rml::Element& element) const
{
    if (element.GetTagName() != "div")
        return false;

    if (hasClassName(element, "widget_stack"))
        return true;

    const WidgetPropertyMap declarations = parseInlineStyle(element.GetAttribute<Rml::String>("style", "").c_str());
    if (const auto displayIt = declarations.find("display"); displayIt != declarations.end() && displayIt->second == "flex")
        return true;

    return declarations.find("flex-direction") != declarations.end();
}

WidgetPropertyMap StackLayoutWidget::capturePropertiesFromElement(const Rml::Element& element) const
{
    WidgetPropertyMap properties;
    const WidgetPropertyMap declarations = parseInlineStyle(element.GetAttribute<Rml::String>("style", "").c_str());
    if (const auto widthIt = declarations.find("width"); widthIt != declarations.end())
        properties["width"] = widthIt->second;
    if (const auto heightIt = declarations.find("height"); heightIt != declarations.end())
        properties["height"] = heightIt->second;
    if (const auto directionIt = declarations.find("flex-direction"); directionIt != declarations.end())
        properties["flex-direction"] = directionIt->second;
    if (const auto gapIt = declarations.find("gap"); gapIt != declarations.end())
        properties["gap"] = gapIt->second;
    if (const auto alignIt = declarations.find("align-items"); alignIt != declarations.end())
        properties["align-items"] = alignIt->second;
    if (const auto justifyIt = declarations.find("justify-content"); justifyIt != declarations.end())
        properties["justify-content"] = justifyIt->second;
    if (const auto backgroundIt = declarations.find("background-color"); backgroundIt != declarations.end())
        properties["background-color"] = backgroundIt->second;

    captureFlexItemProperties(element, properties);

    return properties;
}

std::string StackLayoutWidget::buildMarkup(
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
    const std::string direction = propertyValue(properties, "flex-direction", "column");
    const std::string gap = propertyValue(properties, "gap", "12px");
    const std::string alignment = propertyValue(properties, "align-items", "stretch");
    const std::string justification = propertyValue(properties, "justify-content", "flex-start");
    const std::string backgroundColor = propertyValue(properties, "background-color", "#f2eadc");

    std::vector<std::pair<std::string, std::string>> styleOverrides;
    if (width != "100%")
        styleOverrides.emplace_back("width", width);
    if (height != "auto")
        styleOverrides.emplace_back("height", height);
    if (direction != "column")
        styleOverrides.emplace_back("flex-direction", direction);
    if (gap != "12px")
        styleOverrides.emplace_back("gap", ensureUnit(gap));
    if (alignment != "stretch")
        styleOverrides.emplace_back("align-items", alignment);
    if (justification != "flex-start")
        styleOverrides.emplace_back("justify-content", justification);
    if (backgroundColor != "#f2eadc")
        styleOverrides.emplace_back("background-color", backgroundColor);
    appendFlexItemStyleOverrides(properties, styleOverrides);

    const std::string styleAttribute = buildStyleAttribute(styleOverrides);

    (void)mode;
    return indentation + "<div class='widget_stack'" + styleAttribute + ">\n" +
        children +
        indentation + "</div>\n";
}

std::string StackLayoutWidget::buildStyleRules() const
{
    return ".widget_stack { display: flex; flex-direction: column; gap: 12px; align-items: stretch; justify-content: flex-start; width: 100%; min-width: 0px; min-height: 0px; padding: 12px; box-sizing: border-box; border: 1px #ccbfa9; background-color: #f2eadc; }\n";
}

InspectorModel StackLayoutWidget::buildInspectorModel(const std::string&, const WidgetPropertyMap& properties) const
{
    return {
        {
            InspectorField("width", "Width", "100%", propertyValue(properties, "width", "100%")),
            InspectorField("height", "Height", "auto", propertyValue(properties, "height", "auto")),
            InspectorField("flex-direction", "Flex Direction", "column", propertyValue(properties, "flex-direction", "column"), InspectorInputKind::Enum, {{"column", "Column"}, {"row", "Row"}}),
            InspectorField("gap", "Gap", "12px", propertyValue(properties, "gap", "12px")),
            InspectorField("align-items", "Align Items", "stretch", propertyValue(properties, "align-items", "stretch"), InspectorInputKind::Enum, {{"stretch", "Stretch"}, {"flex-start", "Flex Start"}, {"center", "Center"}, {"flex-end", "Flex End"}}),
            InspectorField("justify-content", "Justify Content", "flex-start", propertyValue(properties, "justify-content", "flex-start"), InspectorInputKind::Enum, {{"flex-start", "Flex Start"}, {"center", "Center"}, {"flex-end", "Flex End"}, {"space-between", "Space Between"}, {"space-around", "Space Around"}, {"space-evenly", "Space Evenly"}}),
            InspectorField("background-color", "Background Color", "#f2eadc", propertyValue(properties, "background-color", "#f2eadc"))
        },
        {
            buildFlexItemInspectorGroup(properties)
        }
    };
}

std::string StackLayoutWidget::buildInspectorMarkup(const std::string& label) const
{
    return std::string("<div class='placeholder_block'><div class='placeholder_title'>") +
        escapeText(label) +
        "</div><div class='placeholder_text'>Auto-layout group. Child order matters and is intended to be edited directly in the hierarchy.</div></div>";
}

} // namespace ui::widget