#pragma once

#include <sstream>

#include "common/UI/AUIElement.hpp"

namespace UI
{
    class MenuBar : public AUIElement
    {
    protected:
        const char* getElementType() const override
        {
            return "menuBar";
        }

        std::string buildRML(const std::string& hierarchicalId) const override
        {
            std::ostringstream stream;
            stream << "<div" << buildCommonAttributes(hierarchicalId) << " data-ui-kind='menu-bar'>";
            stream << renderChildren(hierarchicalId);
            stream << "</div>";
            return stream.str();
        }

    public:
        explicit MenuBar(int width, int height)
            : AUIElement(width, height, Direction::HORIZONTAL)
        {
            addClassName("builder_menu_bar");
        }
    };
}