#pragma once

#include "Widget.hpp"

#include <vector>

namespace ui::widget
{

const std::vector<const Widget*>& getWidgets();
const Widget* findWidgetByKey(const std::string& key);
const Widget* findWidgetByElement(const Rml::Element& element);

} // namespace ui::widget