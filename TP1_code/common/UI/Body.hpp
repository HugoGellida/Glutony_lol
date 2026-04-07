#pragma once

#include <sstream>

#include "common/UI/AUIElement.hpp"

namespace UI
{
    class Body : public AUIElement
    {
    public:
        explicit Body(int width, int height, Direction direction = Direction::HORIZONTAL)
            : AUIElement(width, height, direction)
        {
        }

        std::string getRML() const override
        {
            std::ostringstream stream;
            stream << "<body" << buildCommonAttributes() << ">";
            stream << getChildrenRML();
            stream << "</body>";
            return stream.str();
        }
    };
}