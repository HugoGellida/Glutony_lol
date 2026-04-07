#pragma once

#include <sstream>
#include <string>

#include "common/UI/AUIElement.hpp"

namespace UI
{
    class Container : public AUIElement
    {
    protected:
        int borderWidth = 0;
        std::string tagName = "div";

        const char* getElementType() const override
        {
            return "container";
        }

        std::string buildRML(const std::string& hierarchicalId) const override
        {
            std::ostringstream stream;
            stream << "<" << tagName << buildCommonAttributes(hierarchicalId);
            stream << " data-ui-kind='container'";
            stream << " data-ui-border-width='" << borderWidth << "'";
            stream << ">";
            stream << renderChildren(hierarchicalId);
            stream << "</" << tagName << ">";
            return stream.str();
        }

    public:
        explicit Container(int width, int height, Direction direction = Direction::HORIZONTAL)
            : AUIElement(width, height, direction)
        {
            addClassName("ui_container");
        }

        void setBorderWidth(int value)
        {
            borderWidth = value;
        }

        int getBorderWidth() const
        {
            return borderWidth;
        }

        void setTagName(const std::string& value)
        {
            if (!value.empty())
                tagName = value;
        }

        const std::string& getTagName() const
        {
            return tagName;
        }
    };
}