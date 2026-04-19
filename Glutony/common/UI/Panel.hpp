#pragma once

#include <sstream>
#include <string>
#include <vector>

#include "common/UI/Container.hpp"
#include "common/UI/PanelHeader.hpp"

namespace UI
{
    class Panel : public Container
    {
    protected:
        std::string contentDomIdOverride;
        std::vector<std::string> contentClassNames;

        const char* getElementType() const override
        {
            return "panel";
        }

        std::string buildRML(const std::string& hierarchicalId) const override
        {
            std::ostringstream stream;
            stream << "<div" << buildCommonAttributes(hierarchicalId) << " data-ui-kind='panel'>";

            std::size_t contentStartIndex = 0;
            if (getChildCount() > 0)
            {
                if (dynamic_cast<const PanelHeader*>(getChild(0)) != nullptr)
                {
                    stream << "<div data-ui-slot='header'>";
                    stream << renderChildAt(0, hierarchicalId);
                    stream << "</div>";
                    contentStartIndex = 1;
                }
            }

            stream << "<div data-ui-slot='content' class='panel_body";
            for (const std::string& className : contentClassNames)
                stream << " " << escapeRML(className);
            stream << "'";
            if (!contentDomIdOverride.empty())
                stream << " id='" << escapeRML(contentDomIdOverride) << "'";
            stream << ">";
            for (std::size_t index = contentStartIndex; index < getChildCount(); ++index)
                stream << renderChildAt(index, hierarchicalId);
            stream << "</div>";
            stream << "</div>";
            return stream.str();
        }

    public:
        explicit Panel(int width, int height, Direction direction = Direction::VERTICAL)
            : Container(width, height, direction)
        {
            addClassName("panel_shell");
        }

        void setContentDomIdOverride(const std::string& value)
        {
            contentDomIdOverride = value;
        }

        const std::string& getContentDomIdOverride() const
        {
            return contentDomIdOverride;
        }

        void addContentClassName(const std::string& value)
        {
            if (!value.empty())
                contentClassNames.push_back(value);
        }

        void clearContentClassNames()
        {
            contentClassNames.clear();
        }

        const std::vector<std::string>& getContentClassNames() const
        {
            return contentClassNames;
        }
    };
}