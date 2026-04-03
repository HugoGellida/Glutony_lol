#pragma once

#include <cctype>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>

namespace asset
{
struct SceneScriptAssetDefinition
{
    std::string sourcePath;
    std::string entryName = "sceneMain";
};

class SceneScriptAssetIO
{
private:
    static bool readFile(const std::string& path, std::string& content)
    {
        std::ifstream input(path.c_str(), std::ios::in);
        if (!input.is_open())
            return false;

        std::ostringstream stream;
        stream << input.rdbuf();
        content = stream.str();
        return true;
    }

    static size_t skipWhitespace(const std::string& content, size_t index)
    {
        while (index < content.size() && std::isspace(static_cast<unsigned char>(content[index])) != 0)
            ++index;
        return index;
    }

    static bool findValueStart(const std::string& content, const std::string& key, size_t& valueStart)
    {
        const std::string pattern = "\"" + key + "\"";
        const size_t keyPosition = content.find(pattern);
        if (keyPosition == std::string::npos)
            return false;

        const size_t colonPosition = content.find(':', keyPosition + pattern.size());
        if (colonPosition == std::string::npos)
            return false;

        valueStart = skipWhitespace(content, colonPosition + 1);
        return valueStart < content.size();
    }

    static bool extractString(const std::string& content, const std::string& key, std::string& value)
    {
        size_t valueStart = 0;
        if (!findValueStart(content, key, valueStart) || content[valueStart] != '"')
            return false;

        const size_t valueEnd = content.find('"', valueStart + 1);
        if (valueEnd == std::string::npos)
            return false;

        value = content.substr(valueStart + 1, valueEnd - valueStart - 1);
        return true;
    }

public:
    static bool loadDefinitionFromContent(const std::string& content, SceneScriptAssetDefinition& definition)
    {
        definition = SceneScriptAssetDefinition();
        if (!extractString(content, "source", definition.sourcePath))
            return false;

        std::string entryName;
        if (extractString(content, "entry", entryName) && !entryName.empty())
            definition.entryName = entryName;

        return !definition.sourcePath.empty();
    }

    static bool loadDefinition(const std::string& path, SceneScriptAssetDefinition& definition)
    {
        std::string content;
        if (!readFile(path, content))
            return false;
        return loadDefinitionFromContent(content, definition);
    }

    static bool writeDefinition(std::ostream& output, const SceneScriptAssetDefinition& definition)
    {
        output << "{\n";
        output << "  \"source\": \"" << definition.sourcePath << "\",\n";
        output << "  \"entry\": \"" << definition.entryName << "\"\n";
        output << "}\n";
        return static_cast<bool>(output);
    }

    static bool saveDefinition(const std::string& path, const SceneScriptAssetDefinition& definition)
    {
        std::ofstream output(path.c_str(), std::ios::trunc);
        if (!output.is_open())
            return false;
        return writeDefinition(output, definition);
    }
};
}