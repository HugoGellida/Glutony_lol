#pragma once

#include "common/UI/Container.hpp"

namespace UI
{
    class ToolbarGroup : public Container
    {
    protected:
        const char* getElementType() const override
        {
            return "toolbarGroup";
        }

    public:
        explicit ToolbarGroup(int width, int height)
            : Container(width, height, Direction::HORIZONTAL)
        {
            addClassName("preview_toolbar_group");
        }
    };
}