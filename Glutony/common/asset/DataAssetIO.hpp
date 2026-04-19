#pragma once

#include <glm/glm.hpp>

#include <cctype>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace asset
{
enum class DataAssetValueKind
{
    Group,
    Bool,
    Int,
    Float,
    Vec3,
    String,
};

struct DataAssetNodeDefinition
{
    std::string name;
    DataAssetValueKind kind = DataAssetValueKind::Group;
    bool boolValue = false;
    int intValue = 0;
    float floatValue = 0.0f;
    glm::vec3 vec3Value{0.0f, 0.0f, 0.0f};
    std::string stringValue;
    std::vector<DataAssetNodeDefinition> children;
};

struct DataAssetDefinition
{
    std::string rootName = "root";
    std::vector<DataAssetNodeDefinition> children;
};

class DataAssetIO
{
private:
    class Parser
    {
    private:
        const std::string& m_content;
        size_t m_index = 0;

        void skipWhitespace()
        {
            while (m_index < m_content.size() && std::isspace(static_cast<unsigned char>(m_content[m_index])) != 0)
                ++m_index;
        }

        bool consume(char expected)
        {
            skipWhitespace();
            if (m_index >= m_content.size() || m_content[m_index] != expected)
                return false;
            ++m_index;
            return true;
        }

        bool parseString(std::string& value)
        {
            skipWhitespace();
            if (m_index >= m_content.size() || m_content[m_index] != '"')
                return false;

            ++m_index;
            std::ostringstream stream;
            while (m_index < m_content.size())
            {
                const char character = m_content[m_index++];
                if (character == '"')
                {
                    value = stream.str();
                    return true;
                }

                if (character == '\\')
                {
                    if (m_index >= m_content.size())
                        return false;
                    const char escaped = m_content[m_index++];
                    switch (escaped)
                    {
                    case '"':
                    case '\\':
                    case '/':
                        stream << escaped;
                        break;
                    case 'n':
                        stream << '\n';
                        break;
                    case 't':
                        stream << '\t';
                        break;
                    default:
                        return false;
                    }
                    continue;
                }

                stream << character;
            }

            return false;
        }

        bool parseBool(bool& value)
        {
            skipWhitespace();
            if (m_content.compare(m_index, 4, "true") == 0)
            {
                value = true;
                m_index += 4;
                return true;
            }
            if (m_content.compare(m_index, 5, "false") == 0)
            {
                value = false;
                m_index += 5;
                return true;
            }
            return false;
        }

        bool parseNumberToken(std::string& token)
        {
            skipWhitespace();
            const size_t start = m_index;
            if (m_index < m_content.size() && (m_content[m_index] == '-' || m_content[m_index] == '+'))
                ++m_index;

            bool hasDigit = false;
            while (m_index < m_content.size() && std::isdigit(static_cast<unsigned char>(m_content[m_index])) != 0)
            {
                hasDigit = true;
                ++m_index;
            }

            if (m_index < m_content.size() && m_content[m_index] == '.')
            {
                ++m_index;
                while (m_index < m_content.size() && std::isdigit(static_cast<unsigned char>(m_content[m_index])) != 0)
                {
                    hasDigit = true;
                    ++m_index;
                }
            }

            if (!hasDigit)
                return false;

            token = m_content.substr(start, m_index - start);
            return true;
        }

        bool parseInt(int& value)
        {
            std::string token;
            if (!parseNumberToken(token))
                return false;
            std::stringstream stream(token);
            return static_cast<bool>(stream >> value);
        }

        bool parseFloat(float& value)
        {
            std::string token;
            if (!parseNumberToken(token))
                return false;
            std::stringstream stream(token);
            return static_cast<bool>(stream >> value);
        }

        bool parseVec3(glm::vec3& value)
        {
            if (!consume('['))
                return false;
            if (!parseFloat(value.x))
                return false;
            if (!consume(','))
                return false;
            if (!parseFloat(value.y))
                return false;
            if (!consume(','))
                return false;
            if (!parseFloat(value.z))
                return false;
            return consume(']');
        }

        static bool parseKindValue(const std::string& rawValue, DataAssetValueKind& kind)
        {
            if (rawValue == "group")
                return kind = DataAssetValueKind::Group, true;
            if (rawValue == "bool")
                return kind = DataAssetValueKind::Bool, true;
            if (rawValue == "int")
                return kind = DataAssetValueKind::Int, true;
            if (rawValue == "float")
                return kind = DataAssetValueKind::Float, true;
            if (rawValue == "vec3")
                return kind = DataAssetValueKind::Vec3, true;
            if (rawValue == "string")
                return kind = DataAssetValueKind::String, true;
            return false;
        }

        bool parseNode(DataAssetNodeDefinition& node)
        {
            if (!consume('{'))
                return false;

            bool hasName = false;
            bool hasType = false;
            while (true)
            {
                skipWhitespace();
                if (consume('}'))
                    break;

                std::string key;
                if (!parseString(key) || !consume(':'))
                    return false;

                if (key == "name")
                {
                    if (!parseString(node.name))
                        return false;
                    hasName = true;
                }
                else if (key == "type")
                {
                    std::string typeValue;
                    if (!parseString(typeValue) || !parseKindValue(typeValue, node.kind))
                        return false;
                    hasType = true;
                }
                else if (key == "value")
                {
                    switch (node.kind)
                    {
                    case DataAssetValueKind::Bool:
                        if (!parseBool(node.boolValue))
                            return false;
                        break;
                    case DataAssetValueKind::Int:
                        if (!parseInt(node.intValue))
                            return false;
                        break;
                    case DataAssetValueKind::Float:
                        if (!parseFloat(node.floatValue))
                            return false;
                        break;
                    case DataAssetValueKind::Vec3:
                        if (!parseVec3(node.vec3Value))
                            return false;
                        break;
                    case DataAssetValueKind::String:
                        if (!parseString(node.stringValue))
                            return false;
                        break;
                    case DataAssetValueKind::Group:
                        return false;
                    }
                }
                else if (key == "children")
                {
                    if (!consume('['))
                        return false;

                    node.children.clear();
                    skipWhitespace();
                    while (!consume(']'))
                    {
                        DataAssetNodeDefinition child;
                        if (!parseNode(child))
                            return false;
                        node.children.push_back(child);
                        skipWhitespace();
                        if (consume(']'))
                            break;
                        if (!consume(','))
                            return false;
                    }
                }
                else
                {
                    return false;
                }

                skipWhitespace();
                if (consume('}'))
                    break;
                if (!consume(','))
                    return false;
            }

            if (!hasName || !hasType)
                return false;

            if (node.kind != DataAssetValueKind::Group)
                node.children.clear();

            return true;
        }

    public:
        explicit Parser(const std::string& content)
            : m_content(content)
        {
        }

        bool parseRoot(DataAssetDefinition& definition)
        {
            definition = DataAssetDefinition();
            if (!consume('{'))
                return false;

            bool hasRootName = false;
            bool hasChildren = false;
            while (true)
            {
                skipWhitespace();
                if (consume('}'))
                    break;

                std::string key;
                if (!parseString(key) || !consume(':'))
                    return false;

                if (key == "rootName")
                {
                    if (!parseString(definition.rootName))
                        return false;
                    hasRootName = true;
                }
                else if (key == "children")
                {
                    if (!consume('['))
                        return false;

                    definition.children.clear();
                    skipWhitespace();
                    while (!consume(']'))
                    {
                        DataAssetNodeDefinition child;
                        if (!parseNode(child))
                            return false;
                        definition.children.push_back(child);
                        skipWhitespace();
                        if (consume(']'))
                            break;
                        if (!consume(','))
                            return false;
                    }
                    hasChildren = true;
                }
                else
                {
                    return false;
                }

                skipWhitespace();
                if (consume('}'))
                    break;
                if (!consume(','))
                    return false;
            }

            return hasRootName && hasChildren;
        }
    };

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

    static const char* kindValue(DataAssetValueKind kind)
    {
        switch (kind)
        {
        case DataAssetValueKind::Group:
            return "group";
        case DataAssetValueKind::Bool:
            return "bool";
        case DataAssetValueKind::Int:
            return "int";
        case DataAssetValueKind::Float:
            return "float";
        case DataAssetValueKind::Vec3:
            return "vec3";
        case DataAssetValueKind::String:
            return "string";
        }

        return "group";
    }

    static bool writeNode(std::ostream& output, const DataAssetNodeDefinition& node, int indent)
    {
        const std::string indentation(static_cast<size_t>(indent), ' ');
        output << indentation << "{\n";
        output << indentation << "  \"name\": \"" << node.name << "\",\n";
        output << indentation << "  \"type\": \"" << kindValue(node.kind) << "\"";

        switch (node.kind)
        {
        case DataAssetValueKind::Bool:
            output << ",\n" << indentation << "  \"value\": " << (node.boolValue ? "true" : "false");
            break;
        case DataAssetValueKind::Int:
            output << ",\n" << indentation << "  \"value\": " << node.intValue;
            break;
        case DataAssetValueKind::Float:
            output << ",\n" << indentation << "  \"value\": " << std::fixed << std::setprecision(3) << node.floatValue;
            break;
        case DataAssetValueKind::Vec3:
            output << ",\n" << indentation << "  \"value\": [" << std::fixed << std::setprecision(3)
                   << node.vec3Value.x << ", " << node.vec3Value.y << ", " << node.vec3Value.z << "]";
            break;
        case DataAssetValueKind::String:
            output << ",\n" << indentation << "  \"value\": \"" << node.stringValue << "\"";
            break;
        case DataAssetValueKind::Group:
            break;
        }

        if (node.kind == DataAssetValueKind::Group)
        {
            output << ",\n" << indentation << "  \"children\": [\n";
            for (size_t index = 0; index < node.children.size(); ++index)
            {
                if (!writeNode(output, node.children[index], indent + 4))
                    return false;
                if (index + 1 < node.children.size())
                    output << ",";
                output << "\n";
            }
            output << indentation << "  ]\n";
        }
        else
        {
            output << "\n";
        }

        output << indentation << "}";
        return static_cast<bool>(output);
    }

public:
    static bool loadDefinitionFromContent(const std::string& content, DataAssetDefinition& definition)
    {
        Parser parser(content);
        return parser.parseRoot(definition);
    }

    static bool loadDefinition(const std::string& path, DataAssetDefinition& definition)
    {
        std::string content;
        if (!readFile(path, content))
            return false;
        return loadDefinitionFromContent(content, definition);
    }

    static bool writeDefinition(std::ostream& output, const DataAssetDefinition& definition)
    {
        output << "{\n";
        output << "  \"rootName\": \"" << definition.rootName << "\",\n";
        output << "  \"children\": [\n";
        for (size_t index = 0; index < definition.children.size(); ++index)
        {
            if (!writeNode(output, definition.children[index], 4))
                return false;
            if (index + 1 < definition.children.size())
                output << ",";
            output << "\n";
        }
        output << "  ]\n";
        output << "}\n";
        return static_cast<bool>(output);
    }

    static bool saveDefinition(const std::string& path, const DataAssetDefinition& definition)
    {
        std::ofstream output(path.c_str(), std::ios::trunc);
        if (!output.is_open())
            return false;
        return writeDefinition(output, definition);
    }
};
}