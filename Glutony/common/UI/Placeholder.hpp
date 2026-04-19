#pragma once

#include <sstream>
#include <string>

#include "common/UI/AUIElement.hpp"

namespace UI
{
    class Placeholder : public AUIElement
    {
    protected:
        std::string title;
        std::string description;

        const char* getElementType() const override
        {
            return "placeholder";
        }

        std::string buildRML(const std::string& hierarchicalId) const override
        {
            std::ostringstream stream;
            stream << "<div" << buildCommonAttributes(hierarchicalId) << " data-ui-kind='placeholder'>";
            stream << "<div data-ui-slot='title' class='placeholder_title'>" << escapeRML(title) << "</div>";
            stream << "<div data-ui-slot='description' class='placeholder_text'>" << escapeRML(description) << "</div>";
            stream << "</div>";
            return stream.str();
        }

    public:
        explicit Placeholder(int width, int height)
            : AUIElement(width, height, Direction::VERTICAL)
        {
            addClassName("placeholder_block");
        }

        void setTitle(const std::string& value)
        {
            title = value;
        }

        const std::string& getTitle() const
        {
            return title;
        }

        void setDescription(const std::string& value)
        {
            description = value;
        }

        const std::string& getDescription() const
        {
            return description;
        }
    };
}