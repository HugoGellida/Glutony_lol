#pragma once

#include <sstream>
#include <string>

#include "common/UI/AUIElement.hpp"

namespace UI
{
    class MenuItem : public AUIElement
    {
    protected:
        std::string label;
        bool enabled = true;
        bool separatorAbove = false;

        const char* getElementType() const override
        {
            return "menuItem";
        }

        std::string buildRML(const std::string& hierarchicalId) const override
        {
            std::ostringstream stream;
            stream << "<div" << buildCommonAttributes(hierarchicalId);
            stream << " data-ui-kind='menu-item'";
            stream << " data-ui-enabled='" << (enabled ? "true" : "false") << "'";
            stream << " data-ui-separator-above='" << (separatorAbove ? "true" : "false") << "'";
            stream << ">";
            stream << escapeRML(label);
            stream << "</div>";
            return stream.str();
        }

    public:
        explicit MenuItem(int width, int height, const std::string& label = "")
            : AUIElement(width, height, Direction::HORIZONTAL),
              label(label)
        {
                        addClassName("builder_menu_item");
        }

        void setLabel(const std::string& value)
        {
            label = value;
        }

        const std::string& getLabel() const
        {
            return label;
        }

        void setEnabled(bool value)
        {
            enabled = value;
        }

        bool isEnabled() const
        {
            return enabled;
        }

        void setSeparatorAbove(bool value)
        {
            separatorAbove = value;
        }

        bool hasSeparatorAbove() const
        {
            return separatorAbove;
        }
    };
}