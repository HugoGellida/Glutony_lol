#pragma once

#include <sstream>
#include <string>

#include "common/UI/AUIElement.hpp"

namespace UI
{
    class MenuEntry : public AUIElement
    {
    protected:
        std::string buttonDomIdOverride;
        std::string dropdownDomIdOverride;

        const char* getElementType() const override
        {
            return "menu";
        }

        std::string buildRML(const std::string& hierarchicalId) const override
        {
            std::ostringstream stream;
            const std::string buttonId = buttonDomIdOverride.empty() ? hierarchicalId + ".button" : buttonDomIdOverride;
            const std::string dropdownId = dropdownDomIdOverride.empty() ? hierarchicalId + ".dropdown" : dropdownDomIdOverride;

            stream << "<div" << buildCommonAttributes(hierarchicalId) << " data-ui-kind='menu-entry'>";
            stream << "<div id='" << escapeRML(buttonId) << "' data-ui-slot='menu-button' class='builder_menu_button'>" << escapeRML(title) << "</div>";
            stream << "<div id='" << escapeRML(dropdownId) << "' data-ui-slot='menu-dropdown' class='builder_menu_dropdown' data-ui-expanded='" << (expanded ? "true" : "false") << "' style='display: " << (expanded ? "block" : "none") << ";'>";
            stream << renderChildren(hierarchicalId);
            stream << "</div>";
            stream << "</div>";
            return stream.str();
        }

        std::string title;
        bool expanded = false;

    public:
        explicit MenuEntry(int width, int height, const std::string& title = "")
            : AUIElement(width, height, Direction::VERTICAL),
              title(title)
        {
                        addClassName("builder_menu");
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

        void setButtonDomIdOverride(const std::string& value)
        {
            buttonDomIdOverride = value;
        }

        void setDropdownDomIdOverride(const std::string& value)
        {
            dropdownDomIdOverride = value;
        }
    };
}