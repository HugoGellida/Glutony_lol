#include "Widget.hpp"

#include <cctype>

namespace ui::widget
{
namespace
{

std::string trim(const std::string& value)
{
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0)
        ++begin;

    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
        --end;

    return value.substr(begin, end - begin);
}

} // namespace

InspectorOption::InspectorOption(std::string value, std::string label)
    : value(std::move(value))
    , label(std::move(label))
{
}

InspectorField::InspectorField(
    std::string key,
    std::string label,
    std::string defaultValue,
    std::string value,
    InspectorInputKind inputKind,
    std::vector<InspectorOption> options)
    : key(std::move(key))
    , label(std::move(label))
    , defaultValue(std::move(defaultValue))
    , value(std::move(value))
    , inputKind(inputKind)
    , options(std::move(options))
{
}

InspectorGroup::InspectorGroup(std::string key, std::string title, std::vector<InspectorField> fields)
    : key(std::move(key))
    , title(std::move(title))
    , fields(std::move(fields))
{
}

Widget::Widget(std::string key, std::string name, std::string description)
    : m_key(std::move(key))
    , m_name(std::move(name))
    , m_description(std::move(description))
{
}

const std::string& Widget::key() const
{
    return m_key;
}

const std::string& Widget::name() const
{
    return m_name;
}

const std::string& Widget::description() const
{
    return m_description;
}

std::string Widget::defaultLabel() const
{
    return m_name;
}

bool Widget::canHaveEditorChildren() const
{
    return false;
}

bool Widget::isCompositeTemplate() const
{
    return false;
}

std::optional<std::string> Widget::primaryTextPropertyKey() const
{
    return std::nullopt;
}

std::string Widget::labelFromElement(const Rml::Element&) const
{
    return defaultLabel();
}

WidgetPropertyMap Widget::capturePropertiesFromElement(const Rml::Element&) const
{
    return {};
}

std::string Widget::indent(int depth)
{
    return std::string(static_cast<std::size_t>(depth * 4), ' ');
}

std::string Widget::joinChildren(const std::vector<std::string>& childMarkup)
{
    std::string joined;
    for (const std::string& child : childMarkup)
        joined += child;
    return joined;
}

std::string Widget::escapeText(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char character : value)
    {
        switch (character)
        {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        case '\'': escaped += "&apos;"; break;
        default: escaped.push_back(character); break;
        }
    }
    return escaped;
}

std::string Widget::propertyValue(const WidgetPropertyMap& properties, const std::string& key, const std::string& defaultValue)
{
    const auto propertyIt = properties.find(key);
    return propertyIt != properties.end() ? propertyIt->second : defaultValue;
}

std::string Widget::ensureUnit(const std::string& value, const std::string& unit)
{
    const std::string trimmedValue = trim(value);
    if (trimmedValue.empty())
        return "";

    if (trimmedValue.find_first_not_of("0123456789.-") == std::string::npos)
        return trimmedValue + unit;

    return trimmedValue;
}

bool Widget::hasClassName(const Rml::Element& element, const std::string& className)
{
    const std::string classes = element.GetAttribute<Rml::String>("class", "").c_str();
    if (classes.empty())
        return false;

    std::size_t start = 0;
    while (start < classes.size())
    {
        const std::size_t end = classes.find(' ', start);
        const std::string token = classes.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (token == className)
            return true;
        if (end == std::string::npos)
            break;
        start = end + 1;
    }

    return false;
}

WidgetPropertyMap Widget::parseInlineStyle(const std::string& styleValue)
{
    WidgetPropertyMap declarations;
    std::size_t start = 0;
    while (start < styleValue.size())
    {
        const std::size_t separator = styleValue.find(';', start);
        const std::string declaration = trim(styleValue.substr(start, separator == std::string::npos ? std::string::npos : separator - start));
        if (!declaration.empty())
        {
            const std::size_t colon = declaration.find(':');
            if (colon != std::string::npos)
            {
                const std::string name = trim(declaration.substr(0, colon));
                const std::string value = trim(declaration.substr(colon + 1));
                if (!name.empty() && !value.empty())
                    declarations[name] = value;
            }
        }

        if (separator == std::string::npos)
            break;
        start = separator + 1;
    }

    return declarations;
}

std::string Widget::buildStyleAttribute(const std::vector<std::pair<std::string, std::string>>& declarations)
{
    std::string style;
    for (const auto& declaration : declarations)
    {
        if (declaration.second.empty())
            continue;
        style += declaration.first + ": " + declaration.second + "; ";
    }

    if (style.empty())
        return "";

    return " style='" + escapeText(style) + "'";
}

std::string Widget::buildDataAttribute(const std::string& name, const std::string& value)
{
    if (value.empty())
        return "";
    return " data-" + name + "='" + escapeText(value) + "'";
}

void Widget::captureFlexItemProperties(const Rml::Element& element, WidgetPropertyMap& properties)
{
    const WidgetPropertyMap declarations = parseInlineStyle(element.GetAttribute<Rml::String>("style", "").c_str());
    if (const auto flexBasisIt = declarations.find("flex-basis"); flexBasisIt != declarations.end())
        properties["flex-basis"] = flexBasisIt->second;
    if (const auto flexGrowIt = declarations.find("flex-grow"); flexGrowIt != declarations.end())
        properties["flex-grow"] = flexGrowIt->second;
    if (const auto flexShrinkIt = declarations.find("flex-shrink"); flexShrinkIt != declarations.end())
        properties["flex-shrink"] = flexShrinkIt->second;
}

void Widget::appendFlexItemStyleOverrides(
    const WidgetPropertyMap& properties,
    std::vector<std::pair<std::string, std::string>>& declarations,
    const std::string& defaultBasis,
    const std::string& defaultGrow,
    const std::string& defaultShrink)
{
    const std::string flexBasis = propertyValue(properties, "flex-basis", defaultBasis);
    const std::string flexGrow = propertyValue(properties, "flex-grow", defaultGrow);
    const std::string flexShrink = propertyValue(properties, "flex-shrink", defaultShrink);

    if (flexBasis != defaultBasis)
        declarations.emplace_back("flex-basis", flexBasis == "auto" ? flexBasis : ensureUnit(flexBasis));
    if (flexGrow != defaultGrow)
        declarations.emplace_back("flex-grow", flexGrow);
    if (flexShrink != defaultShrink)
        declarations.emplace_back("flex-shrink", flexShrink);
}

InspectorGroup Widget::buildFlexItemInspectorGroup(const WidgetPropertyMap& properties)
{
    return InspectorGroup(
        "flex-item",
        "Flex Item",
        {
            InspectorField("flex-basis", "Flex Basis", "auto", propertyValue(properties, "flex-basis", "auto")),
            InspectorField("flex-grow", "Flex Grow", "0", propertyValue(properties, "flex-grow", "0"), InspectorInputKind::Number),
            InspectorField("flex-shrink", "Flex Shrink", "1", propertyValue(properties, "flex-shrink", "1"), InspectorInputKind::Number)
        });
}

} // namespace ui::widget