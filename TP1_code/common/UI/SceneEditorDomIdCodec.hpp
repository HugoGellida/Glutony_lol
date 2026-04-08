#pragma once

#include <RmlUi/Core.h>

#include <common/ui/EditorUiCommon.hpp>

#include <optional>
#include <iomanip>
#include <sstream>
#include <string>

namespace UI
{
class SceneEditorDomIdCodec
{
public:
    static std::string makeHierarchyNodeElementId(int nodeId)
    {
        return "hierarchy_node_" + std::to_string(nodeId);
    }

    static std::string makeAssetDirectoryElementId(const std::string& directoryId)
    {
        return "asset_directory_" + encodeElementToken(directoryId);
    }

    static std::string makeAssetDirectoryToggleElementId(const std::string& directoryId)
    {
        return "asset_directory_toggle_" + encodeElementToken(directoryId);
    }

    static std::string makeAssetFileElementId(const std::string& fileId)
    {
        return "asset_file_" + encodeElementToken(fileId);
    }

    static std::optional<int> parseHierarchyNodeId(const Rml::String& elementId)
    {
        const std::string value = elementId;
        const std::string prefix = "hierarchy_node_";
        if (!editor_ui::startsWith(value, prefix))
            return std::nullopt;

        return std::stoi(value.substr(prefix.size()));
    }

    static std::optional<std::string> parseAssetDirectoryElementId(const Rml::String& elementId)
    {
        return parseEncodedElementId(elementId, "asset_directory_");
    }

    static std::optional<std::string> parseAssetDirectoryToggleElementId(const Rml::String& elementId)
    {
        return parseEncodedElementId(elementId, "asset_directory_toggle_");
    }

    static std::optional<std::string> parseAssetFileElementId(const Rml::String& elementId)
    {
        return parseEncodedElementId(elementId, "asset_file_");
    }

private:
    static std::string encodeElementToken(const std::string& value)
    {
        std::ostringstream stream;
        stream << std::hex << std::setfill('0');
        for (unsigned char character : value)
            stream << std::setw(2) << static_cast<int>(character);
        return stream.str();
    }

    static int decodeHexDigit(char value)
    {
        if (value >= '0' && value <= '9')
            return value - '0';
        if (value >= 'a' && value <= 'f')
            return 10 + (value - 'a');
        if (value >= 'A' && value <= 'F')
            return 10 + (value - 'A');
        return -1;
    }

    static std::optional<std::string> decodeElementToken(const std::string& token)
    {
        if ((token.size() % 2) != 0)
            return std::nullopt;

        std::string decoded;
        decoded.reserve(token.size() / 2);

        for (size_t index = 0; index < token.size(); index += 2)
        {
            const int high = decodeHexDigit(token[index]);
            const int low = decodeHexDigit(token[index + 1]);
            if (high < 0 || low < 0)
                return std::nullopt;

            decoded.push_back(static_cast<char>((high << 4) | low));
        }

        return decoded;
    }

    static std::optional<std::string> parseEncodedElementId(const Rml::String& elementId, const std::string& prefix)
    {
        const std::string value = elementId;
        if (!editor_ui::startsWith(value, prefix))
            return std::nullopt;

        const std::optional<std::string> decoded = decodeElementToken(value.substr(prefix.size()));
        if (!decoded.has_value() || decoded->empty())
            return std::nullopt;

        return decoded;
    }
};
}