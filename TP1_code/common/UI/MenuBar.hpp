#pragma once

#include <sstream>

#include "common/UI/AUIElement.hpp"

namespace UI
{
    class MenuBar : public AUIElement
    {
    public:
        explicit MenuBar(int width, int height)
            : AUIElement(width, height, Direction::HORIZONTAL)
        {
        }

        std::string getRML() const override
        {
            std::ostringstream stream;
            stream << "<div" << buildCommonAttributes() << " data-ui-kind='menu-bar'>";
            stream << getChildrenRML();
            stream << "</div>";
            return stream.str();
        }
    };
}