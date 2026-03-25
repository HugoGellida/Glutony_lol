#include "ImageWidget.hpp"

namespace ui::widget
{

ImageWidget::ImageWidget()
    : Widget(
        "image",
        "Image",
        "Static image element with source and real size properties.")
{
}

std::string ImageWidget::tagName() const
{
    return "img";
}

bool ImageWidget::matchesElement(const Rml::Element& element) const
{
    return element.GetTagName() == "img";
}

WidgetPropertyMap ImageWidget::capturePropertiesFromElement(const Rml::Element& element) const
{
    WidgetPropertyMap properties;
    const std::string source = element.GetAttribute<Rml::String>("src", "").c_str();
    const WidgetPropertyMap declarations = parseInlineStyle(element.GetAttribute<Rml::String>("style", "").c_str());
    if (!source.empty())
        properties["src"] = source;
    if (const auto widthIt = declarations.find("width"); widthIt != declarations.end())
        properties["width"] = widthIt->second;
    if (const auto heightIt = declarations.find("height"); heightIt != declarations.end())
        properties["height"] = heightIt->second;
    captureFlexItemProperties(element, properties);
    return properties;
}

std::string ImageWidget::buildMarkup(
    const std::string&,
    const WidgetPropertyMap& properties,
    const std::vector<std::string>&,
    int depth,
    DocumentBuildMode mode) const
{
    const std::string indentation = indent(depth);
    const std::string source = propertyValue(properties, "src", "img/placeholder.png");
    const std::string width = propertyValue(properties, "width", "100%");
    const std::string height = propertyValue(properties, "height", "120px");

    std::vector<std::pair<std::string, std::string>> styleOverrides;
    if (width != "100%")
        styleOverrides.emplace_back("width", width);
    if (height != "120px")
        styleOverrides.emplace_back("height", height);
    appendFlexItemStyleOverrides(properties, styleOverrides);

    const std::string styleAttribute = buildStyleAttribute(styleOverrides);
    (void)mode;
    return indentation + "<img class='widget_image' src='" + escapeText(source) + "'" + styleAttribute + " />\n";
}

std::string ImageWidget::buildStyleRules() const
{
    return ".widget_image { display: block; width: 100%; height: 120px; min-width: 0px; min-height: 0px; background-color: #ddd4c2; }\n";
}

InspectorModel ImageWidget::buildInspectorModel(const std::string&, const WidgetPropertyMap& properties) const
{
    return {
        {
            InspectorField("src", "Source", "img/placeholder.png", propertyValue(properties, "src", "img/placeholder.png")),
            InspectorField("width", "Width", "100%", propertyValue(properties, "width", "100%")),
            InspectorField("height", "Height", "120px", propertyValue(properties, "height", "120px"))
        },
        {
            buildFlexItemInspectorGroup(properties)
        }
    };
}

std::string ImageWidget::buildInspectorMarkup(const std::string&) const
{
    return "<div class='placeholder_block'><div class='placeholder_title'>Image</div><div class='placeholder_text'>Image source and size are edited as real element attributes and styles.</div></div>";
}

} // namespace ui::widget