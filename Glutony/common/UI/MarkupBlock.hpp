#pragma once

#include <string>

#include "common/UI/AUIElement.hpp"

namespace UI
{
    class MarkupBlock : public AUIElement
    {
    protected:
        std::string markup;

        const char* getElementType() const override
        {
            return "markup";
        }

        std::string buildRML(const std::string& hierarchicalId) const override
        {
            (void)hierarchicalId;
            return markup;
        }

    public:
        explicit MarkupBlock(int width, int height, const std::string& markup = "")
            : AUIElement(width, height, Direction::VERTICAL),
              markup(markup)
        {
        }

        void setMarkup(const std::string& value)
        {
            markup = value;
        }

        const std::string& getMarkup() const
        {
            return markup;
        }
    };
}