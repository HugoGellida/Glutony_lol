#pragma once

#include <sstream>

#include "common/UI/AUIElement.hpp"
#include "common/UI/TabHeader.hpp"
#include "common/UI/TabItem.hpp"

namespace UI
{
    class TabPanel : public AUIElement
    {
    protected:
        TabHeader* getTabHeader() const
        {
            return dynamic_cast<TabHeader*>(getChild(0));
        }

        std::size_t getTabContentStartIndex() const
        {
            return getTabHeader() != nullptr ? 1U : 0U;
        }

        std::size_t getTabItemCount() const
        {
            const std::size_t startIndex = getTabContentStartIndex();
            return getChildCount() > startIndex ? getChildCount() - startIndex : 0U;
        }

        TabItem* getTabItem(std::size_t tabIndex) const
        {
            return dynamic_cast<TabItem*>(getChild(getTabContentStartIndex() + tabIndex));
        }

        const char* getElementType() const override
        {
            return "tabPanel";
        }

        std::string buildRML(const std::string& hierarchicalId) const override
        {
            std::ostringstream stream;
            stream << "<div" << buildCommonAttributes(hierarchicalId) << " data-ui-kind='tab-panel'>";
            stream << "<div data-ui-slot='tab-header'>";

            if (TabHeader* header = getTabHeader())
                stream << renderChildAt(0, hierarchicalId);
            else
            {
                for (std::size_t index = 0; index < getTabItemCount(); ++index)
                {
                    if (const TabItem* item = getTabItem(index))
                    {
                        stream << "<div class='panel_tab_button";
                        if (index == activeTabIndex)
                            stream << " active";
                        stream << "' data-ui-tab-index='" << index << "'";
                        if (index == activeTabIndex)
                            stream << " data-ui-active='true'";
                        stream << ">" << escapeRML(item->getTitle()) << "</div>";
                    }
                }
            }

            stream << "</div>";
            stream << "<div data-ui-slot='tab-content' class='panel_body";
            if (contentWithoutPadding)
                stream << " panel_body_no_padding";
            stream << "'>";

            if (activeTabIndex < getTabItemCount())
                stream << renderChildAt(getTabContentStartIndex() + activeTabIndex, hierarchicalId);

            stream << "</div>";
            stream << "</div>";
            return stream.str();
        }

        std::size_t activeTabIndex = 0;
    bool contentWithoutPadding = false;

    public:
        explicit TabPanel(int width, int height, Direction direction = Direction::VERTICAL)
            : AUIElement(width, height, direction)
        {
            addClassName("panel_shell");
        }

        void setTabHeader(TabHeader* header)
        {
            if (header == nullptr)
                return;

            if (getTabHeader() != nullptr)
                children[0] = header;
            else
                children.insert(children.begin(), header);
        }

        void addTab(TabItem* item)
        {
            if (item != nullptr)
                addChild(item);
        }

        void setActiveTabIndex(std::size_t index)
        {
            if (index < getTabItemCount())
                activeTabIndex = index;

            for (std::size_t itemIndex = 0; itemIndex < getTabItemCount(); ++itemIndex)
            {
                if (TabItem* item = getTabItem(itemIndex))
                    item->setActive(itemIndex == activeTabIndex);
            }
        }

        std::size_t getActiveTabIndex() const
        {
            return activeTabIndex;
        }

        void setContentWithoutPadding(bool value)
        {
            contentWithoutPadding = value;
        }
    };
}