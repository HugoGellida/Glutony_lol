#pragma once

#include <algorithm>
#include <cctype>
#include <string>

namespace render
{
enum class RenderTargetFormat
{
    Float,
    Rg,
    Rgb,
    Rgba,
};

enum class RenderTargetBakeCombineOp
{
    Multiply,
    Add,
    Min,
    Max,
};

struct SceneRenderTargetSettings
{
    std::string name;
    int width = 0;
    int height = 0;
    RenderTargetFormat format = RenderTargetFormat::Rgba;
    std::string bakedTextureAssetPath;
};

inline std::string normalizeRenderTargetName(const std::string& rawName)
{
    std::string normalized = rawName;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');

    size_t first = 0;
    while (first < normalized.size() && std::isspace(static_cast<unsigned char>(normalized[first])) != 0)
        ++first;

    size_t last = normalized.size();
    while (last > first && std::isspace(static_cast<unsigned char>(normalized[last - 1])) != 0)
        --last;

    return normalized.substr(first, last - first);
}

inline bool isFinalRenderTargetName(const std::string& rawName)
{
    return normalizeRenderTargetName(rawName) == "final";
}

inline const char* renderTargetFormatName(RenderTargetFormat format)
{
    switch (format)
    {
    case RenderTargetFormat::Float:
        return "float";
    case RenderTargetFormat::Rg:
        return "rg";
    case RenderTargetFormat::Rgb:
        return "rgb";
    case RenderTargetFormat::Rgba:
    default:
        return "rgba";
    }
}

inline const char* renderTargetBakeCombineOpName(RenderTargetBakeCombineOp op)
{
    switch (op)
    {
    case RenderTargetBakeCombineOp::Add:
        return "add";
    case RenderTargetBakeCombineOp::Min:
        return "min";
    case RenderTargetBakeCombineOp::Max:
        return "max";
    case RenderTargetBakeCombineOp::Multiply:
    default:
        return "multiply";
    }
}

inline bool parseRenderTargetBakeCombineOp(const std::string& rawValue, RenderTargetBakeCombineOp& op)
{
    std::string normalized = rawValue;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });

    if (normalized == "mul" || normalized == "multiply")
        return op = RenderTargetBakeCombineOp::Multiply, true;
    if (normalized == "add")
        return op = RenderTargetBakeCombineOp::Add, true;
    if (normalized == "min")
        return op = RenderTargetBakeCombineOp::Min, true;
    if (normalized == "max")
        return op = RenderTargetBakeCombineOp::Max, true;
    return false;
}

inline bool parseRenderTargetFormat(const std::string& rawValue, RenderTargetFormat& format)
{
    std::string normalized = rawValue;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });

    if (normalized == "float" || normalized == "r")
        return format = RenderTargetFormat::Float, true;
    if (normalized == "rg" || normalized == "rb")
        return format = RenderTargetFormat::Rg, true;
    if (normalized == "rgb")
        return format = RenderTargetFormat::Rgb, true;
    if (normalized == "rgba")
        return format = RenderTargetFormat::Rgba, true;
    return false;
}
}