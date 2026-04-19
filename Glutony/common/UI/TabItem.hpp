#pragma once

#include <sstream>
#include <string>

#include "common/UI/Container.hpp"

namespace UI
{
    class TabItem : public Container
    {
    protected:
        std::string title;
        bool active = false;

        const char* getElementType() const override
        {
            return "tab";
        }

        std::string buildRML(const std::string& hierarchicalId) const override
        {
            std::ostringstream stream;
            stream << "<div" << buildCommonAttributes(hierarchicalId);
            stream << " data-ui-kind='tab-item'";
            stream << " data-ui-title='" << escapeRML(title) << "'";
            stream << " data-ui-active='" << (active ? "true" : "false") << "'";
            stream << ">";
            stream << renderChildren(hierarchicalId);
            stream << "</div>";
            return stream.str();
        }

    public:
        explicit TabItem(int width, int height, const std::string& title = "")
            : Container(width, height, Direction::VERTICAL),
              title(title)
        {
                        addClassName("panel_tab_content");
        }

        void setTitle(const std::string& value)
        {
            title = value;
        }

        const std::string& getTitle() const
        {
            return title;
        }

        void setActive(bool value)
        {
            active = value;
        }

        bool isActive() const
        {
            return active;
        }
    };
}