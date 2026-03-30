#pragma once

#include <RmlUi/Core.h>

#include <optional>
#include <string>

struct UiRect
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    bool isValid() const
    {
        return width > 0 && height > 0;
    }

    bool contains(double px, double py) const
    {
        return px >= x && py >= y && px < (x + width) && py < (y + height);
    }

    int centerX() const
    {
        return x + width / 2;
    }

    int centerY() const
    {
        return y + height / 2;
    }
};

namespace editor_ui
{
enum class EditorMode
{
    SceneEditor,
    UiBuilder,
};

struct ShellElements
{
    Rml::Context* context = nullptr;
    Rml::ElementDocument* document = nullptr;
    Rml::Element* root = nullptr;
    Rml::Element* header = nullptr;
    Rml::Element* leftPanel = nullptr;
    Rml::Element* centerPanel = nullptr;
    Rml::Element* viewportPanel = nullptr;
    Rml::Element* bottomPanel = nullptr;
    Rml::Element* rightPanel = nullptr;
};

struct ShellLayout
{
    int windowWidth = 1;
    int windowHeight = 1;
    int contentTop = 0;
    int contentHeight = 0;
    UiRect leftPanelRect;
    UiRect centerRect;
    UiRect viewportRect;
    UiRect rightPanelRect;
    UiRect bottomPanelRect;
};

struct EventHandlingResult
{
    bool handled = false;
    std::optional<EditorMode> requestedMode;
};

constexpr int SplitterThickness = 8;
constexpr int BuilderHeaderHeight = 36;
constexpr int MinColumnWidth = 120;
constexpr int MinCenterWidth = 180;
constexpr int MinViewportHeight = 120;
constexpr int MinBottomHeight = 120;
constexpr int MinLeftSectionHeight = 96;
constexpr int MinPreviewDocumentWidth = 220;
constexpr int MinPreviewDocumentHeight = 140;

bool startsWith(const std::string& value, const std::string& prefix);
std::string escapeRmlText(const std::string& value);
const Rml::Element* findFirstElementByTagName(const Rml::Element* root, const Rml::String& tagName);
Rml::String pixels(int value);
int clampInt(int value, int minValue, int maxValue);
}