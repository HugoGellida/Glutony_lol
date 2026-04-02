#pragma once

#include <glm/glm.hpp>

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>

namespace asset
{
enum class MaterialAssetKind
{
    Lit,
    Unlit,
};

struct MaterialAssetDefinition
{
    MaterialAssetKind kind = MaterialAssetKind::Lit;
    std::string shaderPath;
    glm::vec3 mainColor{0.5f, 0.5f, 0.5f};
};

class MaterialAssetIO
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

    static bool extractVec3(const std::string& content, const std::string& key, glm::vec3& value)
    {
        size_t valueStart = 0;
        if (!findValueStart(content, key, valueStart) || content[valueStart] != '[')
            return false;

        const size_t valueEnd = content.find(']', valueStart + 1);
        if (valueEnd == std::string::npos)
            return false;

        std::string arrayContent = content.substr(valueStart + 1, valueEnd - valueStart - 1);
        for (char& character : arrayContent)
        {
            if (character == ',')
                character = ' ';
        }

        std::stringstream stream(arrayContent);
        return static_cast<bool>(stream >> value.x >> value.y >> value.z);
    }

public:
    static bool loadDefinition(const std::string& path, MaterialAssetDefinition& definition)
    {
        std::string content;
        if (!readFile(path, content))
            return false;

        std::string typeValue;
        std::string shaderValue;

        if (!extractString(content, "type", typeValue) || !extractString(content, "shader", shaderValue))
            return false;

        definition.kind = (typeValue == "unlit") ? MaterialAssetKind::Unlit : MaterialAssetKind::Lit;
        definition.shaderPath = shaderValue;

        glm::vec3 mainColor;
        if (extractVec3(content, "mainColor", mainColor))
            definition.mainColor = mainColor;

        return true;
    }
};
}