#pragma once

#include <sstream>
#include <string>
#include <vector>

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
        const char* getElementType() const override
        {
            return "split";
        }

        std::string buildRML(const std::string& hierarchicalId) const override
        {
            std::ostringstream stream;
            stream << "<div" << buildCommonAttributes(hierarchicalId);
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

            if (getStartChild() != nullptr)
            {
                stream << "<div data-ui-slot='start'>";
                stream << renderChildAt(0, hierarchicalId);
                stream << "</div>";
            }

            if (hasActiveSplit())
            {
                stream << "<div";
                if (!splitterDomIdOverride.empty())
                    stream << " id='" << escapeRML(splitterDomIdOverride) << "'";
                stream << " data-ui-slot='splitter' class='splitter";
                for (const std::string& className : splitterClassNames)
                    stream << " " << escapeRML(className);
                stream << "'></div>";
            }

            if (getEndChild() != nullptr)
            {
                stream << "<div data-ui-slot='end'>";
                stream << renderChildAt(1, hierarchicalId);
                stream << "</div>";
            }

            stream << "</div>";
            return stream.str();
        }

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
        std::string splitterDomIdOverride;
        std::vector<std::string> splitterClassNames;

    public:
        explicit SplitContainer(
            int width,
            int height,
            Direction direction = Direction::HORIZONTAL)
            : AUIElement(width, height, direction)
        {
            addClassName("split_container");
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

        void setSplitterDomIdOverride(const std::string& value)
        {
            splitterDomIdOverride = value;
        }

        void addSplitterClassName(const std::string& value)
        {
            if (!value.empty())
                splitterClassNames.push_back(value);
        }

        void clearSplitterClassNames()
        {
            splitterClassNames.clear();
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
    };
}