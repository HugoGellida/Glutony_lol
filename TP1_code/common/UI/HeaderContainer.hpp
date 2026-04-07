#pragma once

#include <sstream>
#include <string>

#include "common/UI/AUIElement.hpp"

namespace UI
{
    class HeaderContainer : public AUIElement
    {
    protected:
        std::string headerTitle;
        bool headerVisible = true;

    public:
        explicit HeaderContainer(int width, int height, Direction direction = Direction::VERTICAL)
            : AUIElement(width, height, direction)
        {
        }

        void setHeaderTitle(const std::string& value)
        {
            headerTitle = value;
        }

        const std::string& getHeaderTitle() const
        {
            return headerTitle;
        }

        void setHeaderVisible(bool value)
        {
            headerVisible = value;
        }

        bool isHeaderVisible() const
        {
            return headerVisible;
        }

        std::string getRML() const override
        {
            std::ostringstream stream;
            stream << "<div" << buildCommonAttributes() << " data-ui-kind='header-container'>";

            if (headerVisible)
            {
                stream << "<div data-ui-slot='header'>";
                stream << escapeRML(headerTitle);
                stream << "</div>";
            }

            stream << "<div data-ui-slot='content'>";
            stream << getChildrenRML();
            stream << "</div>";
            stream << "</div>";
            return stream.str();
        }
    };
}