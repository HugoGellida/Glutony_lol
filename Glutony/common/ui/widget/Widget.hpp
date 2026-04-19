#pragma once

#include <RmlUi/Core.h>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ui::widget
{

using WidgetPropertyMap = std::unordered_map<std::string, std::string>;

enum class InspectorInputKind
{
    Text,
    Number,
    Enum,
};

struct InspectorOption
{
    InspectorOption() = default;
    InspectorOption(std::string value, std::string label);

    std::string value;
    std::string label;
};

struct InspectorField
{
    InspectorField() = default;
    InspectorField(
        std::string key,
        std::string label,
        std::string defaultValue,
        std::string value,
        InspectorInputKind inputKind = InspectorInputKind::Text,
        std::vector<InspectorOption> options = {});

    std::string key;
    std::string label;
    std::string defaultValue;
    std::string value;
    InspectorInputKind inputKind = InspectorInputKind::Text;
    std::vector<InspectorOption> options;
};

struct InspectorGroup
{
    InspectorGroup() = default;
    InspectorGroup(std::string key, std::string title, std::vector<InspectorField> fields);

    std::string key;
    std::string title;
    std::vector<InspectorField> fields;
};

struct InspectorModel
{
    std::vector<InspectorField> fields;
    std::vector<InspectorGroup> childGroups;
};

enum class DocumentBuildMode
{
    Preview,
    Render,
    Export,
};

class Widget
{
public:
    Widget(std::string key, std::string name, std::string description);
    virtual ~Widget() = default;

    const std::string& key() const;
    const std::string& name() const;
    const std::string& description() const;
    virtual std::string tagName() const = 0;

    virtual std::string defaultLabel() const;
    virtual bool canHaveEditorChildren() const;
    virtual bool isCompositeTemplate() const;
    virtual std::optional<std::string> primaryTextPropertyKey() const;

    virtual bool matchesElement(const Rml::Element& element) const = 0;
    virtual std::string labelFromElement(const Rml::Element& element) const;
    virtual WidgetPropertyMap capturePropertiesFromElement(const Rml::Element& element) const;
    virtual std::string buildMarkup(
        const std::string& label,
        const WidgetPropertyMap& properties,
        const std::vector<std::string>& childMarkup,
        int depth,
        DocumentBuildMode mode) const = 0;
    virtual std::string buildStyleRules() const = 0;
    virtual InspectorModel buildInspectorModel(const std::string& label, const WidgetPropertyMap& properties) const = 0;
    virtual std::string buildInspectorMarkup(const std::string& label) const = 0;

protected:
    static std::string indent(int depth);
    static std::string joinChildren(const std::vector<std::string>& childMarkup);
    static std::string escapeText(const std::string& value);
    static std::string propertyValue(const WidgetPropertyMap& properties, const std::string& key, const std::string& defaultValue);
    static std::string ensureUnit(const std::string& value, const std::string& unit = "px");
    static bool hasClassName(const Rml::Element& element, const std::string& className);
    static WidgetPropertyMap parseInlineStyle(const std::string& styleValue);
    static std::string buildStyleAttribute(const std::vector<std::pair<std::string, std::string>>& declarations);
    static std::string buildDataAttribute(const std::string& name, const std::string& value);
    static void captureFlexItemProperties(const Rml::Element& element, WidgetPropertyMap& properties);
    static void appendFlexItemStyleOverrides(
        const WidgetPropertyMap& properties,
        std::vector<std::pair<std::string, std::string>>& declarations,
        const std::string& defaultBasis = "auto",
        const std::string& defaultGrow = "0",
        const std::string& defaultShrink = "1");
    static InspectorGroup buildFlexItemInspectorGroup(const WidgetPropertyMap& properties);

private:
    std::string m_key;
    std::string m_name;
    std::string m_description;
};

} // namespace ui::widget