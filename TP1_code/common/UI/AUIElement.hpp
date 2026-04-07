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
        std::string segmentId;
        std::string domIdOverride;
        std::vector<std::string> classNames;
        std::vector<AUIElement*> children;
        int width;
        int height;
        Direction direction = Direction::HORIZONTAL;

        virtual const char* getElementType() const = 0;

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

        std::string getResolvedSegmentId() const
        {
            return segmentId.empty() ? std::string(getElementType()) : segmentId;
        }

        std::string buildPathSegment(std::size_t siblingIndex) const
        {
            std::ostringstream stream;
            stream << getResolvedSegmentId() << siblingIndex;
            return stream.str();
        }

        std::size_t getTypedSiblingIndex(std::size_t childIndex) const
        {
            if (childIndex >= children.size() || children[childIndex] == nullptr)
                return 0;

            std::size_t typedIndex = 0;
            const std::string childToken = children[childIndex]->getResolvedSegmentId();
            for (std::size_t index = 0; index < childIndex; ++index)
            {
                const AUIElement* sibling = children[index];
                if (sibling != nullptr && sibling->getResolvedSegmentId() == childToken)
                    ++typedIndex;
            }

            return typedIndex;
        }

        std::string buildChildPath(std::size_t childIndex, const std::string& parentPath) const
        {
            if (childIndex >= children.size() || children[childIndex] == nullptr)
                return parentPath;

            const std::string segment = children[childIndex]->buildPathSegment(getTypedSiblingIndex(childIndex));
            return parentPath.empty() ? segment : parentPath + "." + segment;
        }

        std::string renderChildAt(std::size_t childIndex, const std::string& parentPath) const
        {
            if (childIndex >= children.size() || children[childIndex] == nullptr)
                return "";

            return children[childIndex]->buildRML(buildChildPath(childIndex, parentPath));
        }

        std::string renderChildren(const std::string& parentPath) const
        {
            std::ostringstream stream;

            for (std::size_t index = 0; index < children.size(); ++index)
            {
                if (children[index] != nullptr)
                    stream << renderChildAt(index, parentPath);
            }

            return stream.str();
        }

        std::string buildCommonAttributes(const std::string& hierarchicalId) const
        {
            std::ostringstream stream;

            const std::string& domId = domIdOverride.empty() ? hierarchicalId : domIdOverride;

            stream << " id='" << escapeRML(domId) << "'";
            stream << " data-ui-type='" << escapeRML(getElementType()) << "'";
            stream << " data-ui-segment='" << escapeRML(getResolvedSegmentId()) << "'";
            stream << " data-ui-path='" << escapeRML(hierarchicalId) << "'";
            if (!classNames.empty())
            {
                stream << " class='";
                for (std::size_t index = 0; index < classNames.size(); ++index)
                {
                    if (index > 0)
                        stream << ' ';
                    stream << escapeRML(classNames[index]);
                }
                stream << "'";
            }
            stream << " data-ui-width='" << width << "'";
            stream << " data-ui-height='" << height << "'";
            stream << " data-ui-direction='" << (direction == Direction::HORIZONTAL ? "horizontal" : "vertical") << "'";
            stream << " data-ui-child-count='" << children.size() << "'";
            return stream.str();
        }

        virtual std::string buildRML(const std::string& hierarchicalId) const = 0;

    public:
        explicit AUIElement(int width, int height, Direction direction = Direction::HORIZONTAL)
            : width(width),
              height(height),
              direction(direction)
        {
        }

        virtual ~AUIElement() = default;

        void setSegmentId(const std::string& value)
        {
            segmentId = value;
        }

        const std::string& getSegmentId() const
        {
            return segmentId;
        }

        void setDomIdOverride(const std::string& value)
        {
            domIdOverride = value;
        }

        const std::string& getDomIdOverride() const
        {
            return domIdOverride;
        }

        void addClassName(const std::string& value)
        {
            if (!value.empty())
                classNames.push_back(value);
        }

        void clearClassNames()
        {
            classNames.clear();
        }

        const std::vector<std::string>& getClassNames() const
        {
            return classNames;
        }

        std::string getHierarchicalId() const
        {
            return buildPathSegment(0);
        }

        std::string getDomIdForPath(const std::string& hierarchicalId) const
        {
            return domIdOverride.empty() ? hierarchicalId : domIdOverride;
        }

        std::string getHierarchicalIdForChild(std::size_t childIndex) const
        {
            return buildChildPath(childIndex, getHierarchicalId());
        }

        std::string getHierarchicalIdForChild(std::size_t childIndex, const std::string& parentPath) const
        {
            return buildChildPath(childIndex, parentPath);
        }

        std::string getRMLForPath(const std::string& hierarchicalId) const
        {
            return buildRML(hierarchicalId);
        }

        std::string getChildrenRMLForPath(const std::string& hierarchicalId) const
        {
            return renderChildren(hierarchicalId);
        }

        void addChild(AUIElement* child)
        {
            if (child != nullptr)
                children.push_back(child);
        }

        void clearChildren()
        {
            children.clear();
        }

        const std::vector<AUIElement*>& getChildren() const
        {
            return children;
        }

        std::size_t getChildCount() const
        {
            return children.size();
        }

        bool requiresRebuildComparedTo(const AUIElement& other) const
        {
            return getChildCount() != other.getChildCount();
        }

        bool supportsTargetedUpdateComparedTo(const AUIElement& other) const
        {
            return !requiresRebuildComparedTo(other);
        }

        AUIElement* getChild(std::size_t index) const
        {
            return index < children.size() ? children[index] : nullptr;
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

        std::string getRML() const
        {
            return buildRML(buildPathSegment(0));
        }
    };
}