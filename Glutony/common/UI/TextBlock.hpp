#pragma once

#include <sstream>
#include <string>

#include "common/UI/AUIElement.hpp"

namespace UI
{
    class TextBlock : public AUIElement
    {
    protected:
        std::string text;

        const char* getElementType() const override
        {
            return "text";
        }

        std::string buildRML(const std::string& hierarchicalId) const override
        {
            std::ostringstream stream;
            stream << "<div" << buildCommonAttributes(hierarchicalId) << " data-ui-kind='text-block'>";
            stream << escapeRML(text);
            stream << "</div>";
            return stream.str();
        }

    public:
        explicit TextBlock(int width, int height, const std::string& text = "")
            : AUIElement(width, height, Direction::HORIZONTAL),
              text(text)
        {
        }

        void setText(const std::string& value)
        {
            text = value;
        }

        const std::string& getText() const
        {
            return text;
        }
    };
}