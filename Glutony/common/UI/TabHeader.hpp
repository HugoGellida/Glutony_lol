#pragma once

#include <sstream>

#include "common/UI/Container.hpp"

namespace UI
{
    class TabHeader : public Container
    {
    protected:
        const char* getElementType() const override
        {
            return "tabHeader";
        }

        std::string buildRML(const std::string& hierarchicalId) const override
        {
            std::ostringstream stream;
            stream << "<div" << buildCommonAttributes(hierarchicalId) << " data-ui-kind='tab-header'>";
            stream << renderChildren(hierarchicalId);
            stream << "</div>";
            return stream.str();
        }

    public:
        explicit TabHeader(int width, int height)
            : Container(width, height, Direction::HORIZONTAL)
        {
            addClassName("panel_header");
            addClassName("panel_header_tabs");
        }
    };
}