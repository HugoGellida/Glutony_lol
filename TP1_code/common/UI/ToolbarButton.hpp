#pragma once

#include <sstream>
#include <string>

#include "common/UI/AUIElement.hpp"

namespace UI
{
    class ToolbarButton : public AUIElement
    {
    protected:
        std::string label;

        const char* getElementType() const override
        {
            return "toolbarButton";
        }

        std::string buildRML(const std::string& hierarchicalId) const override
        {
            std::ostringstream stream;
            stream << "<div" << buildCommonAttributes(hierarchicalId) << " data-ui-kind='toolbar-button'>";
            stream << escapeRML(label);
            stream << "</div>";
            return stream.str();
        }

    public:
        explicit ToolbarButton(int width, int height, const std::string& label = "")
            : AUIElement(width, height, Direction::HORIZONTAL),
              label(label)
        {
            addClassName("preview_toolbar_button");
        }

        void setLabel(const std::string& value)
        {
            label = value;
        }

        const std::string& getLabel() const
        {
            return label;
        }
    };
}