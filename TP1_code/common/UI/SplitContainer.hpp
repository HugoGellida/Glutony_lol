#pragma once

#include <sstream>

#include "common/UI/AUIElement.hpp"

namespace UI
{
    class SplitContainer : public AUIElement
    {
    public:
        enum class SplitterTarget
        {
            NONE,
            MAIN
        };

    protected:
        float splitRatio = 0.5f;
        int splitterThickness = 8;

        int minStartSize = 120;
        int minEndSize = 120;

        bool splitterEnabled = true;
        bool draggable = true;

        bool startCollapsed = false;
        bool endCollapsed = false;

        bool isDraggingSplitter = false;
        SplitterTarget activeSplitter = SplitterTarget::NONE;

    public:
        explicit SplitContainer(
            int width,
            int height,
            Direction direction = Direction::HORIZONTAL)
            : AUIElement(width, height, direction)
        {
        }

        void setSplitRatio(float value)
        {
            if (value < 0.0f)
                splitRatio = 0.0f;
            else if (value > 1.0f)
                splitRatio = 1.0f;
            else
                splitRatio = value;
        }

        float getSplitRatio() const
        {
            return splitRatio;
        }

        void setSplitterThickness(int value)
        {
            if (value >= 0)
                splitterThickness = value;
        }

        int getSplitterThickness() const
        {
            return splitterThickness;
        }

        void setMinStartSize(int value)
        {
            if (value >= 0)
                minStartSize = value;
        }

        int getMinStartSize() const
        {
            return minStartSize;
        }

        void setMinEndSize(int value)
        {
            if (value >= 0)
                minEndSize = value;
        }

        int getMinEndSize() const
        {
            return minEndSize;
        }

        void setSplitterEnabled(bool value)
        {
            splitterEnabled = value;
        }

        bool isSplitterEnabled() const
        {
            return splitterEnabled;
        }

        void setDraggable(bool value)
        {
            draggable = value;
        }

        bool isDraggable() const
        {
            return draggable;
        }

        void setStartCollapsed(bool value)
        {
            startCollapsed = value;
        }

        bool isStartCollapsed() const
        {
            return startCollapsed;
        }

        void setEndCollapsed(bool value)
        {
            endCollapsed = value;
        }

        bool isEndCollapsed() const
        {
            return endCollapsed;
        }

        void setDraggingSplitter(bool value)
        {
            isDraggingSplitter = value;
            activeSplitter = value ? SplitterTarget::MAIN : SplitterTarget::NONE;
        }

        bool isDragging() const
        {
            return isDraggingSplitter;
        }

        bool hasActiveSplit() const
        {
            return getChildCount() == 2 && splitterEnabled && !startCollapsed && !endCollapsed;
        }

        bool isStructureValid() const
        {
            return getChildCount() <= 2;
        }

        AUIElement* getStartChild() const
        {
            return getChild(0);
        }

        AUIElement* getEndChild() const
        {
            return getChild(1);
        }

        std::string getRML() const override
        {
            std::ostringstream stream;
            stream << "<div" << buildCommonAttributes();
            stream << " data-ui-kind='split-container'";
            stream << " data-ui-split-ratio='" << splitRatio << "'";
            stream << " data-ui-splitter-thickness='" << splitterThickness << "'";
            stream << " data-ui-min-start='" << minStartSize << "'";
            stream << " data-ui-min-end='" << minEndSize << "'";
            stream << " data-ui-splitter-enabled='" << (splitterEnabled ? "true" : "false") << "'";
            stream << " data-ui-draggable='" << (draggable ? "true" : "false") << "'";
            stream << " data-ui-start-collapsed='" << (startCollapsed ? "true" : "false") << "'";
            stream << " data-ui-end-collapsed='" << (endCollapsed ? "true" : "false") << "'";
            stream << ">";

            if (AUIElement* startChild = getStartChild())
            {
                stream << "<div data-ui-slot='start'>";
                stream << startChild->getRML();
                stream << "</div>";
            }

            if (hasActiveSplit())
                stream << "<div data-ui-slot='splitter'></div>";

            if (AUIElement* endChild = getEndChild())
            {
                stream << "<div data-ui-slot='end'>";
                stream << endChild->getRML();
                stream << "</div>";
            }

            stream << "</div>";
            return stream.str();
        }
    };
}