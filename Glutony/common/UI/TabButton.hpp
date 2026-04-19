#pragma once

#include <sstream>
#include <string>

#include "common/UI/AUIElement.hpp"

namespace UI
{
    class TabButton : public AUIElement
    {
    protected:
        std::string label;
        bool active = false;

        const char* getElementType() const override
        {
            return "tabButton";
        }

        std::string buildRML(const std::string& hierarchicalId) const override
        {
            std::ostringstream stream;
            stream << "<div" << buildCommonAttributes(hierarchicalId) << " data-ui-kind='tab-button'";
            stream << " data-ui-active='" << (active ? "true" : "false") << "'>";
            stream << escapeRML(label);
            stream << "</div>";
            return stream.str();
        }

    public:
        explicit TabButton(int width, int height, const std::string& label = "")
            : AUIElement(width, height, Direction::HORIZONTAL),
              label(label)
        {
            addClassName("panel_tab_button");
        }

        void setLabel(const std::string& value)
        {
            label = value;
        }

        const std::string& getLabel() const
        {
            return label;
        }

        void setActive(bool value)
        {
            active = value;
            clearClassNames();
            addClassName("panel_tab_button");
            if (active)
                addClassName("active");
        }

        bool isActive() const
        {
            return active;
        }
    };
}