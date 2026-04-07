#pragma once

#include <sstream>
#include <string>
#include <vector>

#include "common/UI/AUIElement.hpp"

namespace UI
{
    class TabPanel : public AUIElement
    {
    protected:
        std::vector<std::string> tabTitles;
        std::size_t activeTabIndex = 0;

    public:
        explicit TabPanel(int width, int height, Direction direction = Direction::VERTICAL)
            : AUIElement(width, height, direction)
        {
        }

        void addTab(const std::string& title, AUIElement* page)
        {
            tabTitles.push_back(title);
            addChild(page);
        }

        const std::vector<std::string>& getTabTitles() const
        {
            return tabTitles;
        }

        void setActiveTabIndex(std::size_t index)
        {
            if (index < tabTitles.size())
                activeTabIndex = index;
        }

        std::size_t getActiveTabIndex() const
        {
            return activeTabIndex;
        }

        std::string getRML() const override
        {
            std::ostringstream stream;
            stream << "<div" << buildCommonAttributes() << " data-ui-kind='tab-panel'>";
            stream << "<div data-ui-slot='tab-header'>";

            for (std::size_t index = 0; index < tabTitles.size(); ++index)
            {
                stream << "<div data-ui-tab-index='" << index << "'";
                if (index == activeTabIndex)
                    stream << " data-ui-active='true'";
                stream << ">" << escapeRML(tabTitles[index]) << "</div>";
            }

            stream << "</div>";
            stream << "<div data-ui-slot='tab-content'>";

            if (AUIElement* activeChild = getChild(activeTabIndex))
                stream << activeChild->getRML();

            stream << "</div>";
            stream << "</div>";
            return stream.str();
        }
    };
}