#pragma once

#include <common/UI/MarkupBlock.hpp>
#include <common/UI/Panel.hpp>
#include <common/UI/PanelHeader.hpp>
#include <common/UI/Placeholder.hpp>

#include <string>

namespace UI
{
class UiBuilderPanelMarkupFactory
{
public:
    static std::string buildPlaceholderPanelMarkup(const std::string& panelTitle, const std::string& placeholderTitle, const std::string& placeholderText)
    {
        Panel panel(0, 0);
        PanelHeader header(0, 0, panelTitle);
        Placeholder placeholder(0, 0);
        placeholder.setTitle(placeholderTitle);
        placeholder.setDescription(placeholderText);

        panel.addChild(&header);
        panel.addChild(&placeholder);
        return panel.getRML();
    }

    static std::string buildPanelShellMarkup(
        const std::string& panelTitle,
        const std::string& bodyMarkup,
        const std::string& bodyClassName,
        const std::string& shellClassName = "",
        const std::string& bodyDomId = "")
    {
        Panel shell(0, 0);
        if (!shellClassName.empty())
            shell.addClassName(shellClassName);
        if (!bodyClassName.empty())
            shell.addContentClassName(bodyClassName);
        if (!bodyDomId.empty())
            shell.setContentDomIdOverride(bodyDomId);

        PanelHeader header(0, 0, panelTitle);
        MarkupBlock bodyBlock(0, 0, bodyMarkup);

        shell.addChild(&header);
        shell.addChild(&bodyBlock);
        return shell.getRML();
    }
};
}