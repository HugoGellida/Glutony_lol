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

    public:
        explicit Container(int width, int height, Direction direction = Direction::HORIZONTAL)
            : AUIElement(width, height, direction)
        {
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

        std::string getRML() const override
        {
            std::ostringstream stream;
            stream << "<" << tagName << buildCommonAttributes();
            stream << " data-ui-border-width='" << borderWidth << "'";
            stream << ">";
            stream << getChildrenRML();
            stream << "</" << tagName << ">";
            return stream.str();
        }
    };
}