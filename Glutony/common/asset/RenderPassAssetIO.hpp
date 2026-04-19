#pragma once

#include "../render/SceneRenderTargetSettings.hpp"

#include <glm/glm.hpp>

#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace asset
{
enum class RenderBlendOp
{
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max,
    Mul,
};

enum class RenderBlendFactor
{
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
};

struct RenderBlendChannelDefinition
{
    RenderBlendOp op = RenderBlendOp::Add;
    RenderBlendFactor src = RenderBlendFactor::One;
    RenderBlendFactor dst = RenderBlendFactor::Zero;
};

struct RenderBlendStateDefinition
{
    bool enabled = false;
    bool separateAlpha = false;
    RenderBlendChannelDefinition rgb;
    RenderBlendChannelDefinition a;
};

enum class RenderDepthAction
{
    Preserve,
    Clear,
};

struct RenderTargetAssetReference
{
    std::string name;
    bool shared = false;
    bool grouped = true;
    int width = 0;
    int height = 0;
    render::RenderTargetFormat format = render::RenderTargetFormat::Rgba;
    bool hasFormat = false;
    bool bakeable = false;
    render::RenderTargetBakeCombineOp bakeCombine = render::RenderTargetBakeCombineOp::Multiply;
};

enum class RenderPassIterator
{
    None,
    Light,
};

enum class RenderPassDrawMode
{
    Geometry,
    Fullscreen,
};

enum class RenderPassUniformKind
{
    Bool,
    Int,
    Float,
    Vec3,
    Mat4,
    Texture,
    RenderTarget,
    RenderTargetDepth,
};

struct RenderPassUniformDefinition
{
    std::string name;
    RenderPassUniformKind kind = RenderPassUniformKind::Float;
    bool boolValue = false;
    int intValue = 0;
    float floatValue = 0.0f;
    glm::vec3 vec3Value{0.0f, 0.0f, 0.0f};
    glm::mat4 mat4Value{1.0f};
    std::string assetPath;
    RenderTargetAssetReference renderTargetValue;
};

struct RenderPassStepDefinition
{
    std::string phaseName;
    std::string shaderPath;
    RenderPassDrawMode drawMode = RenderPassDrawMode::Geometry;
    RenderPassIterator iterator = RenderPassIterator::None;
    std::string uniformFactoryPath;
    std::vector<RenderPassUniformDefinition> uniforms;
    RenderTargetAssetReference target;
    bool hasDepthSource = false;
    RenderTargetAssetReference depthSource;
    bool clearColor = false;
    RenderBlendStateDefinition blend;
    RenderDepthAction depthAction = RenderDepthAction::Preserve;
};

struct RenderPassAssetDefinition
{
    std::vector<RenderPassStepDefinition> passes;
};

class RenderPassAssetIO
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
            while (m_index < m_content.size() && m_content[m_index] >= '0' && m_content[m_index] <= '9')
            {
                hasDigit = true;
                ++m_index;
            }

            if (m_index < m_content.size() && m_content[m_index] == '.')
            {
                ++m_index;
                while (m_index < m_content.size() && m_content[m_index] >= '0' && m_content[m_index] <= '9')
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

        bool parseMat4(glm::mat4& value)
        {
            if (!consume('['))
                return false;

            for (int column = 0; column < 4; ++column)
            {
                for (int row = 0; row < 4; ++row)
                {
                    if (!parseFloat(value[column][row]))
                        return false;
                    if (column == 3 && row == 3)
                        break;
                    if (!consume(','))
                        return false;
                }
            }

            return consume(']');
        }

        static bool parseBlendOpValue(const std::string& rawValue, RenderBlendOp& value)
        {
            if (rawValue == "add")
                return value = RenderBlendOp::Add, true;
            if (rawValue == "sub" || rawValue == "subtract")
                return value = RenderBlendOp::Subtract, true;
            if (rawValue == "reverseSubtract" || rawValue == "reverse_subtract")
                return value = RenderBlendOp::ReverseSubtract, true;
            if (rawValue == "min")
                return value = RenderBlendOp::Min, true;
            if (rawValue == "max")
                return value = RenderBlendOp::Max, true;
            if (rawValue == "mul")
                return value = RenderBlendOp::Mul, true;
            return false;
        }

        static bool parseBlendFactorValue(const std::string& rawValue, RenderBlendFactor& value)
        {
            if (rawValue == "zero")
                return value = RenderBlendFactor::Zero, true;
            if (rawValue == "one")
                return value = RenderBlendFactor::One, true;
            if (rawValue == "srcColor" || rawValue == "src_color")
                return value = RenderBlendFactor::SrcColor, true;
            if (rawValue == "oneMinusSrcColor" || rawValue == "one_minus_src_color")
                return value = RenderBlendFactor::OneMinusSrcColor, true;
            if (rawValue == "dstColor" || rawValue == "dst_color")
                return value = RenderBlendFactor::DstColor, true;
            if (rawValue == "oneMinusDstColor" || rawValue == "one_minus_dst_color")
                return value = RenderBlendFactor::OneMinusDstColor, true;
            if (rawValue == "srcAlpha" || rawValue == "src_alpha")
                return value = RenderBlendFactor::SrcAlpha, true;
            if (rawValue == "oneMinusSrcAlpha" || rawValue == "one_minus_src_alpha")
                return value = RenderBlendFactor::OneMinusSrcAlpha, true;
            if (rawValue == "dstAlpha" || rawValue == "dst_alpha")
                return value = RenderBlendFactor::DstAlpha, true;
            if (rawValue == "oneMinusDstAlpha" || rawValue == "one_minus_dst_alpha")
                return value = RenderBlendFactor::OneMinusDstAlpha, true;
            return false;
        }

        static bool parseDepthActionValue(const std::string& rawValue, RenderDepthAction& value)
        {
            if (rawValue == "preserve" || rawValue == "keep" || rawValue == "pass")
                return value = RenderDepthAction::Preserve, true;
            if (rawValue == "clear")
                return value = RenderDepthAction::Clear, true;
            return false;
        }

        static bool parseUniformKindValue(const std::string& rawValue, RenderPassUniformKind& value)
        {
            if (rawValue == "bool")
                return value = RenderPassUniformKind::Bool, true;
            if (rawValue == "int")
                return value = RenderPassUniformKind::Int, true;
            if (rawValue == "float")
                return value = RenderPassUniformKind::Float, true;
            if (rawValue == "vec3")
                return value = RenderPassUniformKind::Vec3, true;
            if (rawValue == "mat4")
                return value = RenderPassUniformKind::Mat4, true;
            if (rawValue == "texture")
                return value = RenderPassUniformKind::Texture, true;
            if (rawValue == "renderTarget" || rawValue == "render_target")
                return value = RenderPassUniformKind::RenderTarget, true;
            if (rawValue == "renderTargetDepth" || rawValue == "render_target_depth")
                return value = RenderPassUniformKind::RenderTargetDepth, true;
            return false;
        }

        static bool parseIteratorValue(const std::string& rawValue, RenderPassIterator& value)
        {
            if (rawValue == "none")
                return value = RenderPassIterator::None, true;
            if (rawValue == "light")
                return value = RenderPassIterator::Light, true;
            return false;
        }

        static bool parseDrawModeValue(const std::string& rawValue, RenderPassDrawMode& value)
        {
            if (rawValue == "mesh")
                return value = RenderPassDrawMode::Geometry, true;
            if (rawValue == "geometry")
                return value = RenderPassDrawMode::Geometry, true;
            if (rawValue == "fullscreen")
                return value = RenderPassDrawMode::Fullscreen, true;
            return false;
        }

        bool parseRenderTargetReference(RenderTargetAssetReference& value)
        {
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
                    if (!parseString(value.name))
                        return false;
                    hasName = true;
                }
                    else if (key == "shared")
                    {
                        if (!parseBool(value.shared))
                            return false;
                    }
                    else if (key == "grouped")
                    {
                        if (!parseBool(value.grouped))
                            return false;
                    }
                    else if (key == "width")
                    {
                        if (!parseInt(value.width))
                            return false;
                    value.width = std::max(0, value.width);
                }
                else if (key == "height")
                {
                    if (!parseInt(value.height))
                        return false;
                    value.height = std::max(0, value.height);
                }
                else if (key == "format")
                {
                    std::string rawValue;
                    if (!parseString(rawValue) || !render::parseRenderTargetFormat(rawValue, value.format))
                        return false;
                    value.hasFormat = true;
                }
                else if (key == "bakeable")
                {
                    if (!parseBool(value.bakeable))
                        return false;
                }
                else if (key == "bakeCombine" || key == "bake_combine")
                {
                    std::string rawValue;
                    if (!parseString(rawValue) || !render::parseRenderTargetBakeCombineOp(rawValue, value.bakeCombine))
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

        bool parseBlendChannel(RenderBlendChannelDefinition& value)
        {
            if (!consume('{'))
                return false;

            while (true)
            {
                skipWhitespace();
                if (consume('}'))
                    break;

                std::string key;
                if (!parseString(key) || !consume(':'))
                    return false;

                if (key == "op")
                {
                    std::string rawValue;
                    if (!parseString(rawValue) || !parseBlendOpValue(rawValue, value.op))
                        return false;
                }
                else if (key == "src")
                {
                    std::string rawValue;
                    if (!parseString(rawValue) || !parseBlendFactorValue(rawValue, value.src))
                        return false;
                }
                else if (key == "dst")
                {
                    std::string rawValue;
                    if (!parseString(rawValue) || !parseBlendFactorValue(rawValue, value.dst))
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

            return true;
        }

        bool parseBlendState(RenderBlendStateDefinition& value)
        {
            skipWhitespace();
            if (m_index < m_content.size() && (m_content.compare(m_index, 4, "true") == 0 || m_content.compare(m_index, 5, "false") == 0))
            {
                if (!parseBool(value.enabled))
                    return false;
                return true;
            }

            if (!consume('{'))
                return false;

            value.enabled = true;
            while (true)
            {
                skipWhitespace();
                if (consume('}'))
                    break;

                std::string key;
                if (!parseString(key) || !consume(':'))
                    return false;

                if (key == "enabled")
                {
                    if (!parseBool(value.enabled))
                        return false;
                }
                else if (key == "op")
                {
                    std::string rawValue;
                    if (!parseString(rawValue) || !parseBlendOpValue(rawValue, value.rgb.op))
                        return false;
                    value.a.op = value.rgb.op;
                }
                else if (key == "src")
                {
                    std::string rawValue;
                    if (!parseString(rawValue) || !parseBlendFactorValue(rawValue, value.rgb.src))
                        return false;
                    value.a.src = value.rgb.src;
                }
                else if (key == "dst")
                {
                    std::string rawValue;
                    if (!parseString(rawValue) || !parseBlendFactorValue(rawValue, value.rgb.dst))
                        return false;
                    value.a.dst = value.rgb.dst;
                }
                else if (key == "rgb" || key == "color")
                {
                    if (!parseBlendChannel(value.rgb))
                        return false;
                }
                else if (key == "a" || key == "alpha")
                {
                    if (!parseBlendChannel(value.a))
                        return false;
                    value.separateAlpha = true;
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

            if (!value.separateAlpha)
                value.a = value.rgb;
            return true;
        }

        bool parseUniform(RenderPassUniformDefinition& value)
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
                    if (!parseString(value.name))
                        return false;
                    hasName = true;
                }
                else if (key == "type")
                {
                    std::string rawValue;
                    if (!parseString(rawValue) || !parseUniformKindValue(rawValue, value.kind))
                        return false;
                    hasType = true;
                }
                else if (key == "value")
                {
                    switch (value.kind)
                    {
                    case RenderPassUniformKind::Bool:
                        if (!parseBool(value.boolValue))
                            return false;
                        break;
                    case RenderPassUniformKind::Int:
                        if (!parseInt(value.intValue))
                            return false;
                        break;
                    case RenderPassUniformKind::Float:
                        if (!parseFloat(value.floatValue))
                            return false;
                        break;
                    case RenderPassUniformKind::Vec3:
                        if (!parseVec3(value.vec3Value))
                            return false;
                        break;
                    case RenderPassUniformKind::Mat4:
                        if (!parseMat4(value.mat4Value))
                            return false;
                        break;
                    case RenderPassUniformKind::Texture:
                        if (!parseString(value.assetPath))
                            return false;
                        break;
                    case RenderPassUniformKind::RenderTarget:
                    case RenderPassUniformKind::RenderTargetDepth:
                        if (!parseRenderTargetReference(value.renderTargetValue))
                            return false;
                        break;
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

            return hasName && hasType;
        }

        bool parseUniformArray(std::vector<RenderPassUniformDefinition>& values)
        {
            if (!consume('['))
                return false;

            values.clear();
            skipWhitespace();
            while (!consume(']'))
            {
                RenderPassUniformDefinition value;
                if (!parseUniform(value))
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

        bool parsePassStep(RenderPassStepDefinition& value)
        {
            if (!consume('{'))
                return false;

            bool hasPhase = false;
            bool hasShader = false;
            bool hasTarget = false;
            while (true)
            {
                skipWhitespace();
                if (consume('}'))
                    break;

                std::string key;
                if (!parseString(key) || !consume(':'))
                    return false;

                if (key == "phase")
                {
                    if (!parseString(value.phaseName))
                        return false;
                    hasPhase = true;
                }
                else if (key == "shader")
                {
                    if (!parseString(value.shaderPath))
                        return false;
                    hasShader = true;
                }
                else if (key == "draw")
                {
                    std::string rawValue;
                    if (!parseString(rawValue) || !parseDrawModeValue(rawValue, value.drawMode))
                        return false;
                }
                else if (key == "iterator")
                {
                    std::string rawValue;
                    if (!parseString(rawValue) || !parseIteratorValue(rawValue, value.iterator))
                        return false;
                }
                else if (key == "uniformFactory" || key == "uniform_factory")
                {
                    if (!parseString(value.uniformFactoryPath))
                        return false;
                }
                else if (key == "uniforms")
                {
                    if (!parseUniformArray(value.uniforms))
                        return false;
                }
                else if (key == "target")
                {
                    if (!parseRenderTargetReference(value.target))
                        return false;
                    hasTarget = true;
                }
                else if (key == "depthSource" || key == "depth_source")
                {
                    if (!parseRenderTargetReference(value.depthSource))
                        return false;
                    value.hasDepthSource = true;
                }
                else if (key == "clear" || key == "clearColor")
                {
                    if (!parseBool(value.clearColor))
                        return false;
                }
                else if (key == "blend")
                {
                    if (!parseBlendState(value.blend))
                        return false;
                }
                else if (key == "depth")
                {
                    std::string rawValue;
                    if (!parseString(rawValue) || !parseDepthActionValue(rawValue, value.depthAction))
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

            return hasPhase && hasShader && hasTarget;
        }

    public:
        explicit Parser(const std::string& content)
            : m_content(content)
        {
        }

        bool parseRoot(RenderPassAssetDefinition& definition)
        {
            definition = RenderPassAssetDefinition();
            if (!consume('{'))
                return false;

            bool hasPasses = false;
            while (true)
            {
                skipWhitespace();
                if (consume('}'))
                    break;

                std::string key;
                if (!parseString(key) || !consume(':'))
                    return false;

                if (key != "passes")
                    return false;

                if (!consume('['))
                    return false;

                definition.passes.clear();
                skipWhitespace();
                while (!consume(']'))
                {
                    RenderPassStepDefinition pass;
                    if (!parsePassStep(pass))
                        return false;
                    definition.passes.push_back(pass);

                    skipWhitespace();
                    if (consume(']'))
                        break;
                    if (!consume(','))
                        return false;
                }

                hasPasses = true;
                skipWhitespace();
                if (consume('}'))
                    break;
                if (!consume(','))
                    return false;
            }

            return hasPasses;
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

    static const char* blendOpValue(RenderBlendOp value)
    {
        switch (value)
        {
        case RenderBlendOp::Add:
            return "add";
        case RenderBlendOp::Subtract:
            return "subtract";
        case RenderBlendOp::ReverseSubtract:
            return "reverseSubtract";
        case RenderBlendOp::Min:
            return "min";
        case RenderBlendOp::Max:
            return "max";
        case RenderBlendOp::Mul:
            return "mul";
        }

        return "add";
    }

    static const char* blendFactorValue(RenderBlendFactor value)
    {
        switch (value)
        {
        case RenderBlendFactor::Zero:
            return "zero";
        case RenderBlendFactor::One:
            return "one";
        case RenderBlendFactor::SrcColor:
            return "srcColor";
        case RenderBlendFactor::OneMinusSrcColor:
            return "oneMinusSrcColor";
        case RenderBlendFactor::DstColor:
            return "dstColor";
        case RenderBlendFactor::OneMinusDstColor:
            return "oneMinusDstColor";
        case RenderBlendFactor::SrcAlpha:
            return "srcAlpha";
        case RenderBlendFactor::OneMinusSrcAlpha:
            return "oneMinusSrcAlpha";
        case RenderBlendFactor::DstAlpha:
            return "dstAlpha";
        case RenderBlendFactor::OneMinusDstAlpha:
            return "oneMinusDstAlpha";
        }

        return "one";
    }

    static const char* depthActionValue(RenderDepthAction value)
    {
        return value == RenderDepthAction::Clear ? "clear" : "preserve";
    }

    static const char* uniformKindValue(RenderPassUniformKind value)
    {
        switch (value)
        {
        case RenderPassUniformKind::Bool:
            return "bool";
        case RenderPassUniformKind::Int:
            return "int";
        case RenderPassUniformKind::Float:
            return "float";
        case RenderPassUniformKind::Vec3:
            return "vec3";
        case RenderPassUniformKind::Mat4:
            return "mat4";
        case RenderPassUniformKind::Texture:
            return "texture";
        case RenderPassUniformKind::RenderTarget:
            return "renderTarget";
        case RenderPassUniformKind::RenderTargetDepth:
            return "renderTargetDepth";
        }

        return "float";
    }

    static const char* iteratorValue(RenderPassIterator value)
    {
        return value == RenderPassIterator::Light ? "light" : "none";
    }

    static const char* drawModeValue(RenderPassDrawMode value)
    {
        return value == RenderPassDrawMode::Fullscreen ? "fullscreen" : "geometry";
    }

    static bool writeRenderTargetReference(std::ostream& output, const RenderTargetAssetReference& value)
    {
        output << "{ \"name\": \"" << value.name << "\", \"shared\": " << (value.shared ? "true" : "false");
        if (!value.grouped)
            output << ", \"grouped\": false";
        if (value.width > 0)
            output << ", \"width\": " << value.width;
        if (value.height > 0)
            output << ", \"height\": " << value.height;
        if (value.hasFormat)
            output << ", \"format\": \"" << render::renderTargetFormatName(value.format) << "\"";
        if (value.bakeable)
            output << ", \"bakeable\": true, \"bakeCombine\": \"" << render::renderTargetBakeCombineOpName(value.bakeCombine) << "\"";
        output << " }";
        return static_cast<bool>(output);
    }

    static bool writeBlendChannel(std::ostream& output, const RenderBlendChannelDefinition& value)
    {
        output << "{ \"op\": \"" << blendOpValue(value.op)
               << "\", \"src\": \"" << blendFactorValue(value.src)
               << "\", \"dst\": \"" << blendFactorValue(value.dst)
               << "\" }";
        return static_cast<bool>(output);
    }

public:
    static bool loadDefinitionFromContent(const std::string& content, RenderPassAssetDefinition& definition)
    {
        Parser parser(content);
        return parser.parseRoot(definition);
    }

    static bool loadDefinition(const std::string& path, RenderPassAssetDefinition& definition)
    {
        std::string content;
        if (!readFile(path, content))
            return false;

        return loadDefinitionFromContent(content, definition);
    }

    static bool writeDefinition(std::ostream& output, const RenderPassAssetDefinition& definition)
    {
        output << std::fixed << std::setprecision(3);
        output << "{\n  \"passes\": [\n";
        for (size_t passIndex = 0; passIndex < definition.passes.size(); ++passIndex)
        {
            const RenderPassStepDefinition& pass = definition.passes[passIndex];
            output << "    {\n";
            output << "      \"phase\": \"" << pass.phaseName << "\",\n";
            output << "      \"shader\": \"" << pass.shaderPath << "\",\n";
            output << "      \"draw\": \"" << drawModeValue(pass.drawMode) << "\",\n";
            output << "      \"iterator\": \"" << iteratorValue(pass.iterator) << "\",\n";
            if (!pass.uniformFactoryPath.empty())
                output << "      \"uniformFactory\": \"" << pass.uniformFactoryPath << "\",\n";
            output << "      \"uniforms\": [\n";
            for (size_t uniformIndex = 0; uniformIndex < pass.uniforms.size(); ++uniformIndex)
            {
                const RenderPassUniformDefinition& uniform = pass.uniforms[uniformIndex];
                output << "        { \"name\": \"" << uniform.name << "\", \"type\": \"" << uniformKindValue(uniform.kind) << "\", \"value\": ";
                switch (uniform.kind)
                {
                case RenderPassUniformKind::Bool:
                    output << (uniform.boolValue ? "true" : "false");
                    break;
                case RenderPassUniformKind::Int:
                    output << uniform.intValue;
                    break;
                case RenderPassUniformKind::Float:
                    output << uniform.floatValue;
                    break;
                case RenderPassUniformKind::Vec3:
                    output << '[' << uniform.vec3Value.x << ", " << uniform.vec3Value.y << ", " << uniform.vec3Value.z << ']';
                    break;
                case RenderPassUniformKind::Mat4:
                    output << '[';
                    for (int column = 0; column < 4; ++column)
                    {
                        for (int row = 0; row < 4; ++row)
                        {
                            if (column != 0 || row != 0)
                                output << ", ";
                            output << uniform.mat4Value[column][row];
                        }
                    }
                    output << ']';
                    break;
                case RenderPassUniformKind::Texture:
                    output << "\"" << uniform.assetPath << "\"";
                    break;
                case RenderPassUniformKind::RenderTarget:
                case RenderPassUniformKind::RenderTargetDepth:
                    if (!writeRenderTargetReference(output, uniform.renderTargetValue))
                        return false;
                    break;
                }
                output << " }";
                if (uniformIndex + 1 < pass.uniforms.size())
                    output << ',';
                output << '\n';
            }
            output << "      ],\n";
            output << "      \"target\": ";
            if (!writeRenderTargetReference(output, pass.target))
                return false;
            output << ",\n";
            if (pass.hasDepthSource)
            {
                output << "      \"depthSource\": ";
                if (!writeRenderTargetReference(output, pass.depthSource))
                    return false;
                output << ",\n";
            }
            output << "      \"clear\": " << (pass.clearColor ? "true" : "false") << ",\n";
            output << "      \"blend\": ";
            if (!pass.blend.enabled)
                output << "false";
            else if (!pass.blend.separateAlpha || (pass.blend.rgb.op == pass.blend.a.op && pass.blend.rgb.src == pass.blend.a.src && pass.blend.rgb.dst == pass.blend.a.dst))
            {
                if (!writeBlendChannel(output, pass.blend.rgb))
                    return false;
            }
            else
            {
                output << "{ \"rgb\": ";
                if (!writeBlendChannel(output, pass.blend.rgb))
                    return false;
                output << ", \"a\": ";
                if (!writeBlendChannel(output, pass.blend.a))
                    return false;
                output << " }";
            }
            output << ",\n";
            output << "      \"depth\": \"" << depthActionValue(pass.depthAction) << "\"\n";
            output << "    }";
            if (passIndex + 1 < definition.passes.size())
                output << ',';
            output << '\n';
        }
        output << "  ]\n}\n";
        return static_cast<bool>(output);
    }

    static std::string saveDefinitionToString(const RenderPassAssetDefinition& definition)
    {
        std::ostringstream output;
        if (!writeDefinition(output, definition))
            return "";
        return output.str();
    }

    static bool saveDefinition(const std::string& path, const RenderPassAssetDefinition& definition)
    {
        std::ofstream output(path.c_str(), std::ios::trunc);
        if (!output.is_open())
            return false;

        return writeDefinition(output, definition);
    }
};
}
