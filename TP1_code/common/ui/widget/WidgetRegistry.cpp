#include "WidgetRegistry.hpp"

#include "ButtonWidget.hpp"
#include "ConstrainedBoxWidget.hpp"
#include "ImageWidget.hpp"
#include "ScrollRegionWidget.hpp"
#include "SpacerWidget.hpp"
#include "StackLayoutWidget.hpp"
#include "TextWidget.hpp"

#include <memory>

namespace ui::widget
{
namespace
{

const std::vector<std::unique_ptr<Widget>>& getWidgetStorage()
{
    static const std::vector<std::unique_ptr<Widget>> widgets = [] {
        std::vector<std::unique_ptr<Widget>> instances;
        instances.emplace_back(std::make_unique<StackLayoutWidget>());
        instances.emplace_back(std::make_unique<ScrollRegionWidget>());
        instances.emplace_back(std::make_unique<SpacerWidget>());
        instances.emplace_back(std::make_unique<TextWidget>());
        instances.emplace_back(std::make_unique<ImageWidget>());
        instances.emplace_back(std::make_unique<ButtonWidget>());
        instances.emplace_back(std::make_unique<ConstrainedBoxWidget>());
        return instances;
    }();
    return widgets;
}

} // namespace

const std::vector<const Widget*>& getWidgets()
{
    static const std::vector<const Widget*> widgets = [] {
        std::vector<const Widget*> entries;
        for (const std::unique_ptr<Widget>& widget : getWidgetStorage())
            entries.push_back(widget.get());
        return entries;
    }();
    return widgets;
}

const Widget* findWidgetByKey(const std::string& key)
{
    for (const Widget* widget : getWidgets())
    {
        if (widget->key() == key)
            return widget;
    }
    return nullptr;
}

const Widget* findWidgetByElement(const Rml::Element& element)
{
    for (const Widget* widget : getWidgets())
    {
        if (widget->matchesElement(element))
            return widget;
    }
    return nullptr;
}

} // namespace ui::widget