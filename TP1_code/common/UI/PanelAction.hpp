#pragma once

#include <sstream>
#include <string>

#include "common/UI/AUIElement.hpp"

namespace UI
{
    class PanelAction : public AUIElement
    {
    protected:
        std::string label;
        bool enabled = true;
        bool emphasized = false;

        const char* getElementType() const override
        {
            return "action";
        }

        std::string buildRML(const std::string& hierarchicalId) const override
        {
            std::ostringstream stream;
            stream << "<div" << buildCommonAttributes(hierarchicalId);
            stream << " data-ui-kind='panel-action'";
            stream << " data-ui-enabled='" << (enabled ? "true" : "false") << "'";
            stream << " data-ui-emphasized='" << (emphasized ? "true" : "false") << "'";
            stream << ">";
            stream << escapeRML(label);
            stream << "</div>";
            return stream.str();
        }

    public:
        explicit PanelAction(int width, int height, const std::string& label = "")
            : AUIElement(width, height, Direction::HORIZONTAL),
              label(label)
        {
                        addClassName("panel_header_action");
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

        void setEmphasized(bool value)
        {
            emphasized = value;
        }

        bool isEmphasized() const
        {
            return emphasized;
        }
    };
}