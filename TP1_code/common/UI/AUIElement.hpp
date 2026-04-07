#pragma once

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

namespace UI
{
    enum Direction
    {
        HORIZONTAL,
        VERTICAL
    };


    class AUIElement
    {
    protected:
        std::string id;
        std::vector<AUIElement*> content;
        int width;
        int height;
        Direction direction = Direction::HORIZONTAL;

        static std::string escapeRML(const std::string& value)
        {
            std::string escaped;
            escaped.reserve(value.size());

            for (const char character : value)
            {
                switch (character)
                {
                case '&':
                    escaped += "&amp;";
                    break;
                case '<':
                    escaped += "&lt;";
                    break;
                case '>':
                    escaped += "&gt;";
                    break;
                case '\"':
                    escaped += "&quot;";
                    break;
                case '\'':
                    escaped += "&apos;";
                    break;
                default:
                    escaped.push_back(character);
                    break;
                }
            }

            return escaped;
        }

        std::string getChildrenRML() const
        {
            std::ostringstream stream;

            for (const AUIElement* child : content)
            {
                if (child != nullptr)
                    stream << child->getRML();
            }

            return stream.str();
        }

        std::string buildCommonAttributes() const
        {
            std::ostringstream stream;

            if (!id.empty())
                stream << " id='" << escapeRML(id) << "'";

            stream << " data-ui-width='" << width << "'";
            stream << " data-ui-height='" << height << "'";
            stream << " data-ui-direction='" << (direction == Direction::HORIZONTAL ? "horizontal" : "vertical") << "'";
            return stream.str();
        }

    public:
        explicit AUIElement(int width, int height, Direction direction = Direction::HORIZONTAL)
            : width(width),
              height(height),
              direction(direction)
        {
        }

        virtual ~AUIElement() = default;

        void setId(const std::string& value)
        {
            id = value;
        }

        const std::string& getId() const
        {
            return id;
        }

        void addChild(AUIElement* child)
        {
            if (child != nullptr)
                content.push_back(child);
        }

        void clearChildren()
        {
            content.clear();
        }

        const std::vector<AUIElement*>& getChildren() const
        {
            return content;
        }

        std::size_t getChildCount() const
        {
            return content.size();
        }

        AUIElement* getChild(std::size_t index) const
        {
            return index < content.size() ? content[index] : nullptr;
        }

        int getWidth() const
        {
            return width;
        }

        int getHeight() const
        {
            return height;
        }

        Direction getDirection() const
        {
            return direction;
        }

        virtual std::string getRML() const = 0;
    };
}