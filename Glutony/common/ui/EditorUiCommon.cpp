#include "EditorUiCommon.hpp"

namespace editor_ui
{
bool startsWith(const std::string& value, const std::string& prefix)
{
    return value.rfind(prefix, 0) == 0;
}

std::string escapeRmlText(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char character : value)
    {
        switch (character)
        {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        case '\'': escaped += "&apos;"; break;
        default: escaped.push_back(character); break;
        }
    }
    return escaped;
}

const Rml::Element* findFirstElementByTagName(const Rml::Element* root, const Rml::String& tagName)
{
    if (root == nullptr)
        return nullptr;

    if (root->GetTagName() == tagName)
        return root;

    for (int childIndex = 0; childIndex < root->GetNumChildren(true); ++childIndex)
    {
        if (const Rml::Element* found = findFirstElementByTagName(root->GetChild(childIndex), tagName))
            return found;
    }

    return nullptr;
}

Rml::String pixels(int value)
{
    return std::to_string(value) + "px";
}

int clampInt(int value, int minValue, int maxValue)
{
    if (maxValue < minValue)
        maxValue = minValue;
    return std::max(minValue, std::min(value, maxValue));
}
}