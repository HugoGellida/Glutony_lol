#pragma once

#include <sstream>
#include <string>

#include "common/UI/Container.hpp"

namespace UI
{
    class PanelHeader : public Container
    {
    protected:
        std::string title;

        const char* getElementType() const override
        {
            return "header";
        }

        std::string buildRML(const std::string& hierarchicalId) const override
        {
            std::ostringstream stream;
            stream << "<div" << buildCommonAttributes(hierarchicalId) << " data-ui-kind='panel-header'>";
            stream << "<div data-ui-slot='title'>" << escapeRML(title) << "</div>";
            stream << "<div data-ui-slot='actions'>" << renderChildren(hierarchicalId) << "</div>";
            stream << "</div>";
            return stream.str();
        }

    public:
        explicit PanelHeader(int width, int height, const std::string& title = "")
            : Container(width, height, Direction::HORIZONTAL),
              title(title)
        {
                        addClassName("panel_header");
        }

        void setTitle(const std::string& value)
        {
            title = value;
        }

        const std::string& getTitle() const
        {
            return title;
        }
    };
}