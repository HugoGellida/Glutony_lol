#include <common/UI/UiBuilderWidgetCatalogPresenter.hpp>

#include <common/UI/UiBuilderPanelMarkupFactory.hpp>

#include <common/ui/widget/WidgetRegistry.hpp>

#include <sstream>

namespace UI
{
std::string UiBuilderWidgetCatalogPresenter::buildWidgetCatalogMarkup() const
{
    std::ostringstream stream;
    for (const ui::widget::Widget* widget : ui::widget::getWidgets())
    {
        stream << "<div id='widget_catalog_item_" << widget->key() << "' class='widget_catalog_item'>";
        stream << "<div class='widget_catalog_label'>" << widget->name() << "</div>";
        stream << "<div class='widget_catalog_text'>" << widget->description() << "</div>";
        stream << "</div>";
    }

    return UiBuilderPanelMarkupFactory::buildPanelShellMarkup("Elements &amp; Classes", stream.str(), "widget_catalog_body");
}
}