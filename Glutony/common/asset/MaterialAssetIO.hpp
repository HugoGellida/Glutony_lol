#pragma once

#include <glm/glm.hpp>

#include <cctype>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace asset
{
enum class MaterialAssetKind
{
    Lit,
    Unlit,
};

enum class MaterialUniformKind
{
    Bool,
    Int,
    Float,
    Vec3,
    Texture,
};

struct MaterialUniformDefinition
{
    std::string name;
    MaterialUniformKind kind = MaterialUniformKind::Float;
    bool boolValue = false;
    int intValue = 0;
    float floatValue = 0.0f;
    glm::vec3 vec3Value{0.0f, 0.0f, 0.0f};
    std::string textureAssetPath;
};

struct MaterialAssetDefinition
{
    MaterialAssetKind kind = MaterialAssetKind::Lit;
    std::string shaderPath;
    std::string renderPassPath;
    std::vector<MaterialUniformDefinition> uniforms;
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

    static size_t skipWhitespaceAndSeparators(const std::string& content, size_t index)
    {
        while (index < content.size())
        {
            const char character = content[index];
            if (std::isspace(static_cast<unsigned char>(character)) != 0 || character == ',')
            {
                ++index;
                continue;
            }
            break;
        }

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

    static size_t findMatchingDelimiter(const std::string& content, size_t startIndex, char openChar, char closeChar)
    {
        if (startIndex >= content.size() || content[startIndex] != openChar)
            return std::string::npos;

        int depth = 0;
        for (size_t index = startIndex; index < content.size(); ++index)
        {
            if (content[index] == openChar)
                ++depth;
            else if (content[index] == closeChar)
            {
                --depth;
                if (depth == 0)
                    return index;
            }
        }

        return std::string::npos;
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

    static bool extractScalarToken(const std::string& content, const std::string& key, std::string& value)
    {
        size_t valueStart = 0;
        if (!findValueStart(content, key, valueStart))
            return false;

        size_t valueEnd = valueStart;
        while (valueEnd < content.size())
        {
            const char character = content[valueEnd];
            if (character == ',' || character == '}' || character == ']' || std::isspace(static_cast<unsigned char>(character)) != 0)
                break;
            ++valueEnd;
        }

        if (valueEnd <= valueStart)
            return false;

        value = content.substr(valueStart, valueEnd - valueStart);
        return true;
    }

    static bool extractBool(const std::string& content, const std::string& key, bool& value)
    {
        std::string token;
        if (!extractScalarToken(content, key, token))
            return false;
        if (token == "true")
            return value = true, true;
        if (token == "false")
            return value = false, true;
        return false;
    }

    static bool extractInt(const std::string& content, const std::string& key, int& value)
    {
        std::string token;
        if (!extractScalarToken(content, key, token))
            return false;
        std::stringstream stream(token);
        return static_cast<bool>(stream >> value);
    }

    static bool extractFloat(const std::string& content, const std::string& key, float& value)
    {
        std::string token;
        if (!extractScalarToken(content, key, token))
            return false;
        std::stringstream stream(token);
        return static_cast<bool>(stream >> value);
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

    static const char* uniformKindValue(MaterialUniformKind kind)
    {
        switch (kind)
        {
        case MaterialUniformKind::Bool:
            return "bool";
        case MaterialUniformKind::Int:
            return "int";
        case MaterialUniformKind::Float:
            return "float";
        case MaterialUniformKind::Vec3:
            return "vec3";
        case MaterialUniformKind::Texture:
            return "texture";
        }

        return "float";
    }

    static bool parseUniformKind(const std::string& rawValue, MaterialUniformKind& kind)
    {
        if (rawValue == "bool")
            return kind = MaterialUniformKind::Bool, true;
        if (rawValue == "int")
            return kind = MaterialUniformKind::Int, true;
        if (rawValue == "float")
            return kind = MaterialUniformKind::Float, true;
        if (rawValue == "vec3")
            return kind = MaterialUniformKind::Vec3, true;
        if (rawValue == "texture")
            return kind = MaterialUniformKind::Texture, true;
        return false;
    }

    static bool extractUniformDefinitions(const std::string& content, std::vector<MaterialUniformDefinition>& uniforms)
    {
        size_t valueStart = 0;
        if (!findValueStart(content, "uniforms", valueStart))
            return false;
        if (content[valueStart] != '[')
            return false;

        const size_t valueEnd = findMatchingDelimiter(content, valueStart, '[', ']');
        if (valueEnd == std::string::npos)
            return false;

        size_t cursor = valueStart + 1;
        while (cursor < valueEnd)
        {
            cursor = skipWhitespaceAndSeparators(content, cursor);
            if (cursor >= valueEnd)
                break;
            if (content[cursor] != '{')
                return false;

            const size_t objectEnd = findMatchingDelimiter(content, cursor, '{', '}');
            if (objectEnd == std::string::npos || objectEnd > valueEnd)
                return false;

            const std::string objectContent = content.substr(cursor, objectEnd - cursor + 1);
            MaterialUniformDefinition uniform;
            std::string typeValue;
            if (!extractString(objectContent, "name", uniform.name) || !extractString(objectContent, "type", typeValue))
                return false;
            if (!parseUniformKind(typeValue, uniform.kind))
            {
                cursor = objectEnd + 1;
                continue;
            }

            switch (uniform.kind)
            {
            case MaterialUniformKind::Bool:
                if (!extractBool(objectContent, "value", uniform.boolValue))
                    return false;
                break;
            case MaterialUniformKind::Int:
                if (!extractInt(objectContent, "value", uniform.intValue))
                    return false;
                break;
            case MaterialUniformKind::Float:
                if (!extractFloat(objectContent, "value", uniform.floatValue))
                    return false;
                break;
            case MaterialUniformKind::Vec3:
                if (!extractVec3(objectContent, "value", uniform.vec3Value))
                    return false;
                break;
            case MaterialUniformKind::Texture:
                if (!extractString(objectContent, "value", uniform.textureAssetPath))
                    return false;
                break;
            }

            uniforms.push_back(uniform);
            cursor = objectEnd + 1;
        }

        return true;
    }

public:
    static bool loadDefinitionFromContent(const std::string& content, MaterialAssetDefinition& definition)
    {
        definition = MaterialAssetDefinition();

        std::string typeValue;
        std::string shaderValue;
        if (!extractString(content, "type", typeValue) || !extractString(content, "shader", shaderValue))
            return false;

        if (typeValue == "unlit")
            definition.kind = MaterialAssetKind::Unlit;
        else if (typeValue == "lit")
            definition.kind = MaterialAssetKind::Lit;
        else
            return false;

        definition.shaderPath = shaderValue;
    extractString(content, "renderPass", definition.renderPassPath);

        size_t uniformsValueStart = 0;
        if (findValueStart(content, "uniforms", uniformsValueStart))
            return extractUniformDefinitions(content, definition.uniforms);

        glm::vec3 legacyMainColor(0.0f, 0.0f, 0.0f);
        if (extractVec3(content, "mainColor", legacyMainColor))
        {
            MaterialUniformDefinition uniform;
            uniform.name = "_mainCol";
            uniform.kind = MaterialUniformKind::Vec3;
            uniform.vec3Value = legacyMainColor;
            definition.uniforms.push_back(uniform);
        }

        return true;
    }

    static bool loadDefinition(const std::string& path, MaterialAssetDefinition& definition)
    {
        std::string content;
        if (!readFile(path, content))
            return false;

        return loadDefinitionFromContent(content, definition);
    }

    static bool writeDefinition(std::ostream& output, const MaterialAssetDefinition& definition)
    {
        output << std::fixed << std::setprecision(3);
        output << "{\n";
        output << "  \"type\": \"" << (definition.kind == MaterialAssetKind::Unlit ? "unlit" : "lit") << "\",\n";
        output << "  \"shader\": \"" << definition.shaderPath << "\",\n";
        output << "  \"renderPass\": \"" << definition.renderPassPath << "\",\n";
        output << "  \"uniforms\": [\n";

        for (size_t index = 0; index < definition.uniforms.size(); ++index)
        {
            const MaterialUniformDefinition& uniform = definition.uniforms[index];
            output << "    { \"name\": \"" << uniform.name << "\", \"type\": \"" << uniformKindValue(uniform.kind) << "\", \"value\": ";
            switch (uniform.kind)
            {
            case MaterialUniformKind::Bool:
                output << (uniform.boolValue ? "true" : "false");
                break;
            case MaterialUniformKind::Int:
                output << uniform.intValue;
                break;
            case MaterialUniformKind::Float:
                output << uniform.floatValue;
                break;
            case MaterialUniformKind::Vec3:
                output << "[" << uniform.vec3Value.x << ", " << uniform.vec3Value.y << ", " << uniform.vec3Value.z << "]";
                break;
            case MaterialUniformKind::Texture:
                output << "\"" << uniform.textureAssetPath << "\"";
                break;
            }

            output << " }";
            if (index + 1 < definition.uniforms.size())
                output << ",";
            output << "\n";
        }

        output << "  ]\n";
        output << "}\n";
        return static_cast<bool>(output);
    }

    static std::string saveDefinitionToString(const MaterialAssetDefinition& definition)
    {
        std::ostringstream output;
        if (!writeDefinition(output, definition))
            return "";
        return output.str();
    }

    static bool saveDefinition(const std::string& path, const MaterialAssetDefinition& definition)
    {
        std::ofstream output(path.c_str(), std::ios::trunc);
        if (!output.is_open())
            return false;

        return writeDefinition(output, definition);
    }
};
}