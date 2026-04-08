#pragma once

#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace asset
{
enum class RenderPhaseIterator
{
    None,
    Light,
};

struct RenderPhaseAssetDefinition
{
    std::string name;
    std::vector<std::string> includes;
    std::vector<std::string> before;
    std::vector<std::string> after;
    RenderPhaseIterator additionalIterator = RenderPhaseIterator::None;
};

class RenderPhaseAssetIO
{
private:
    class Parser
    {
    private:
        const std::string& m_content;
        size_t m_index = 0;

        void skipWhitespace()
        {
            while (m_index < m_content.size())
            {
                const unsigned char character = static_cast<unsigned char>(m_content[m_index]);
                if (character == ' ' || character == '\n' || character == '\r' || character == '\t')
                    ++m_index;
                else
                    break;
            }
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

                if (character != '\\')
                {
                    stream << character;
                    continue;
                }

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
            }

            return false;
        }

        bool parseStringArray(std::vector<std::string>& values)
        {
            if (!consume('['))
                return false;

            values.clear();
            skipWhitespace();
            while (!consume(']'))
            {
                std::string value;
                if (!parseString(value))
                    return false;
                values.push_back(value);

                skipWhitespace();
                if (consume(']'))
                    break;
                if (!consume(','))
                    return false;
            }

            return true;
        }

        static bool parseIteratorValue(const std::string& rawValue, RenderPhaseIterator& iterator)
        {
            if (rawValue == "none")
                return iterator = RenderPhaseIterator::None, true;
            if (rawValue == "light")
                return iterator = RenderPhaseIterator::Light, true;
            return false;
        }

    public:
        explicit Parser(const std::string& content)
            : m_content(content)
        {
        }

        bool parseRoot(RenderPhaseAssetDefinition& definition)
        {
            definition = RenderPhaseAssetDefinition();
            if (!consume('{'))
                return false;

            bool hasName = false;
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
                    if (!parseString(definition.name))
                        return false;
                    hasName = true;
                }
                else if (key == "includes")
                {
                    if (!parseStringArray(definition.includes))
                        return false;
                }
                else if (key == "before")
                {
                    if (!parseStringArray(definition.before))
                        return false;
                }
                else if (key == "after")
                {
                    if (!parseStringArray(definition.after))
                        return false;
                }
                else if (key == "additionalIterator")
                {
                    std::string rawValue;
                    if (!parseString(rawValue) || !parseIteratorValue(rawValue, definition.additionalIterator))
                        return false;
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

            return hasName;
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

    static const char* iteratorValue(RenderPhaseIterator iterator)
    {
        return iterator == RenderPhaseIterator::Light ? "light" : "none";
    }

    static bool writeStringArray(std::ostream& output, const char* key, const std::vector<std::string>& values, bool trailingComma)
    {
        output << "  \"" << key << "\": [";
        for (size_t index = 0; index < values.size(); ++index)
        {
            if (index > 0)
                output << ", ";
            output << "\"" << values[index] << "\"";
        }
        output << "]";
        if (trailingComma)
            output << ',';
        output << '\n';
        return static_cast<bool>(output);
    }

public:
    static bool loadDefinitionFromContent(const std::string& content, RenderPhaseAssetDefinition& definition)
    {
        Parser parser(content);
        return parser.parseRoot(definition);
    }

    static bool loadDefinition(const std::string& path, RenderPhaseAssetDefinition& definition)
    {
        std::string content;
        if (!readFile(path, content))
            return false;

        return loadDefinitionFromContent(content, definition);
    }

    static bool writeDefinition(std::ostream& output, const RenderPhaseAssetDefinition& definition)
    {
        output << "{\n";
        output << "  \"name\": \"" << definition.name << "\",\n";
        if (!writeStringArray(output, "includes", definition.includes, true))
            return false;
        if (!writeStringArray(output, "before", definition.before, true))
            return false;
        if (!writeStringArray(output, "after", definition.after, true))
            return false;
        output << "  \"additionalIterator\": \"" << iteratorValue(definition.additionalIterator) << "\"\n";
        output << "}\n";
        return static_cast<bool>(output);
    }

    static std::string saveDefinitionToString(const RenderPhaseAssetDefinition& definition)
    {
        std::ostringstream output;
        if (!writeDefinition(output, definition))
            return "";
        return output.str();
    }

    static bool saveDefinition(const std::string& path, const RenderPhaseAssetDefinition& definition)
    {
        std::ofstream output(path.c_str(), std::ios::trunc);
        if (!output.is_open())
            return false;

        return writeDefinition(output, definition);
    }
};
}