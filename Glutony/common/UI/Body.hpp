#pragma once

#include <sstream>

#include "common/UI/AUIElement.hpp"

namespace UI
{
    class Body : public AUIElement
    {
    protected:
        const char* getElementType() const override
        {
            return "body";
        }

        std::string buildRML(const std::string& hierarchicalId) const override
        {
            std::ostringstream stream;
            stream << "<body" << buildCommonAttributes(hierarchicalId) << ">";
            stream << renderChildren(hierarchicalId);
            stream << "</body>";
            return stream.str();
        }

    public:
        explicit Body(int width, int height, Direction direction = Direction::HORIZONTAL)
            : AUIElement(width, height, direction)
        {
        }
    };
}