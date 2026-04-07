#pragma once

#include <sstream>
#include <string>

#include "common/UI/AUIElement.hpp"

namespace UI
{
    class MenuEntry : public AUIElement
    {
    protected:
        std::string title;
        bool expanded = false;

    public:
        explicit MenuEntry(int width, int height, const std::string& title = "")
            : AUIElement(width, height, Direction::VERTICAL),
              title(title)
        {
        }

        void setTitle(const std::string& value)
        {
            title = value;
        }

        const std::string& getTitle() const
        {
            return title;
        }

        void setExpanded(bool value)
        {
            expanded = value;
        }

        bool isExpanded() const
        {
            return expanded;
        }

        std::string getRML() const override
        {
            std::ostringstream stream;
            stream << "<div" << buildCommonAttributes() << " data-ui-kind='menu-entry'>";
            stream << "<div data-ui-slot='menu-button'>" << escapeRML(title) << "</div>";
            stream << "<div data-ui-slot='menu-dropdown' data-ui-expanded='" << (expanded ? "true" : "false") << "'>";
            stream << getChildrenRML();
            stream << "</div>";
            stream << "</div>";
            return stream.str();
        }
    };
}