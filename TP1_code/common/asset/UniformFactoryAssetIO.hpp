#pragma once

#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace asset
{
enum class UniformFactoryOutputKind
{
    Bool,
    Int,
    Float,
    Vec3,
    Mat4,
};

struct UniformFactoryOutputDefinition
{
    std::string name;
    UniformFactoryOutputKind kind = UniformFactoryOutputKind::Float;
};

struct UniformFactoryAssetDefinition
{
    std::string sourcePath;
    std::string entryName = "buildUniforms";
    std::string iterationEntryName = "iterationCount";
    std::string groupEntryName;
    std::string bakedGroupEntryName;
    bool requiresLight = false;
    std::vector<UniformFactoryOutputDefinition> outputs;
};

class UniformFactoryAssetIO
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

        bool parseBool(bool& value)
        {
            skipWhitespace();
            if (m_content.compare(m_index, 4, "true") == 0)
            {
                m_index += 4;
                value = true;
                return true;
            }

            if (m_content.compare(m_index, 5, "false") == 0)
            {
                m_index += 5;
                value = false;
                return true;
            }

            return false;
        }

        static bool parseKindValue(const std::string& rawValue, UniformFactoryOutputKind& kind)
        {
            if (rawValue == "bool")
                return kind = UniformFactoryOutputKind::Bool, true;
            if (rawValue == "int")
                return kind = UniformFactoryOutputKind::Int, true;
            if (rawValue == "float")
                return kind = UniformFactoryOutputKind::Float, true;
            if (rawValue == "vec3")
                return kind = UniformFactoryOutputKind::Vec3, true;
            if (rawValue == "mat4")
                return kind = UniformFactoryOutputKind::Mat4, true;
            return false;
        }

        bool parseOutput(UniformFactoryOutputDefinition& output)
        {
            if (!consume('{'))
                return false;

            bool hasName = false;
            bool hasKind = false;
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
                    if (!parseString(output.name))
                        return false;
                    hasName = true;
                }
                else if (key == "type")
                {
                    std::string rawValue;
                    if (!parseString(rawValue) || !parseKindValue(rawValue, output.kind))
                        return false;
                    hasKind = true;
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

            return hasName && hasKind;
        }

    public:
        explicit Parser(const std::string& content)
            : m_content(content)
        {
        }

        bool parseRoot(UniformFactoryAssetDefinition& definition)
        {
            definition = UniformFactoryAssetDefinition();
            if (!consume('{'))
                return false;

            bool hasSourcePath = false;
            bool hasOutputs = false;
            while (true)
            {
                skipWhitespace();
                if (consume('}'))
                    break;

                std::string key;
                if (!parseString(key) || !consume(':'))
                    return false;

                if (key == "source")
                {
                    if (!parseString(definition.sourcePath))
                        return false;
                    hasSourcePath = true;
                }
                else if (key == "entry")
                {
                    if (!parseString(definition.entryName))
                        return false;
                }
                else if (key == "iterationEntry")
                {
                    if (!parseString(definition.iterationEntryName))
                        return false;
                }
                else if (key == "groupEntry")
                {
                    if (!parseString(definition.groupEntryName))
                        return false;
                }
                else if (key == "bakeGroupEntry" || key == "bakedGroupEntry")
                {
                    if (!parseString(definition.bakedGroupEntryName))
                        return false;
                }
                else if (key == "requiresLight")
                {
                    if (!parseBool(definition.requiresLight))
                        return false;
                }
                else if (key == "outputs")
                {
                    if (!consume('['))
                        return false;

                    definition.outputs.clear();
                    skipWhitespace();
                    while (!consume(']'))
                    {
                        UniformFactoryOutputDefinition output;
                        if (!parseOutput(output))
                            return false;
                        definition.outputs.push_back(output);

                        skipWhitespace();
                        if (consume(']'))
                            break;
                        if (!consume(','))
                            return false;
                    }

                    hasOutputs = true;
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

            if (definition.entryName.empty())
                definition.entryName = "buildUniforms";
            if (definition.iterationEntryName.empty())
                definition.iterationEntryName = "iterationCount";

            return hasSourcePath && hasOutputs;
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

    static const char* outputKindValue(UniformFactoryOutputKind kind)
    {
        switch (kind)
        {
        case UniformFactoryOutputKind::Bool:
            return "bool";
        case UniformFactoryOutputKind::Int:
            return "int";
        case UniformFactoryOutputKind::Float:
            return "float";
        case UniformFactoryOutputKind::Vec3:
            return "vec3";
        case UniformFactoryOutputKind::Mat4:
            return "mat4";
        }

        return "float";
    }

public:
    static bool loadDefinitionFromContent(const std::string& content, UniformFactoryAssetDefinition& definition)
    {
        Parser parser(content);
        return parser.parseRoot(definition);
    }

    static bool loadDefinition(const std::string& path, UniformFactoryAssetDefinition& definition)
    {
        std::string content;
        if (!readFile(path, content))
            return false;

        return loadDefinitionFromContent(content, definition);
    }

    static bool writeDefinition(std::ostream& output, const UniformFactoryAssetDefinition& definition)
    {
        output << "{\n";
        output << "  \"source\": \"" << definition.sourcePath << "\",\n";
        output << "  \"entry\": \"" << definition.entryName << "\",\n";
        output << "  \"iterationEntry\": \"" << definition.iterationEntryName << "\",\n";
        if (!definition.groupEntryName.empty())
            output << "  \"groupEntry\": \"" << definition.groupEntryName << "\",\n";
        if (!definition.bakedGroupEntryName.empty())
            output << "  \"bakeGroupEntry\": \"" << definition.bakedGroupEntryName << "\",\n";
        output << "  \"requiresLight\": " << (definition.requiresLight ? "true" : "false") << ",\n";
        output << "  \"outputs\": [\n";
        for (size_t outputIndex = 0; outputIndex < definition.outputs.size(); ++outputIndex)
        {
            const UniformFactoryOutputDefinition& entry = definition.outputs[outputIndex];
            output << "    { \"name\": \"" << entry.name
                   << "\", \"type\": \"" << outputKindValue(entry.kind)
                   << "\" }";
            if (outputIndex + 1 < definition.outputs.size())
                output << ',';
            output << '\n';
        }
        output << "  ]\n}" << '\n';
        return static_cast<bool>(output);
    }

    static bool saveDefinition(const std::string& path, const UniformFactoryAssetDefinition& definition)
    {
        std::ofstream output(path.c_str(), std::ios::trunc);
        if (!output.is_open())
            return false;

        return writeDefinition(output, definition);
    }

    static std::string saveDefinitionToString(const UniformFactoryAssetDefinition& definition)
    {
        std::ostringstream output;
        if (!writeDefinition(output, definition))
            return "";
        return output.str();
    }
};
}