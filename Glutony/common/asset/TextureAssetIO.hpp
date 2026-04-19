#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace asset
{
class TextureAssetIO
{
public:
    static bool ensureParentDirectory(const std::string& path)
    {
        std::error_code errorCode;
        const std::filesystem::path diskPath(path);
        if (!diskPath.parent_path().empty())
            std::filesystem::create_directories(diskPath.parent_path(), errorCode);
        return !errorCode;
    }

    static bool saveRgba8Tga(const std::string& path, int width, int height, const std::vector<unsigned char>& pixelsBottomLeft)
    {
        if (width <= 0 || height <= 0)
            return false;

        const size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4U;
        if (pixelsBottomLeft.size() < expectedSize)
            return false;

        if (!ensureParentDirectory(path))
            return false;

        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output)
            return false;

        unsigned char header[18] = {};
        header[2] = 2;
        header[12] = static_cast<unsigned char>(width & 0xff);
        header[13] = static_cast<unsigned char>((width >> 8) & 0xff);
        header[14] = static_cast<unsigned char>(height & 0xff);
        header[15] = static_cast<unsigned char>((height >> 8) & 0xff);
        header[16] = 32;
        header[17] = 8 | 0x20;
        output.write(reinterpret_cast<const char*>(header), sizeof(header));
        if (!output)
            return false;

        std::vector<unsigned char> bgraRow(static_cast<size_t>(width) * 4U, 0);
        for (int outputY = 0; outputY < height; ++outputY)
        {
            const int sourceY = height - 1 - outputY;
            const size_t rowOffset = static_cast<size_t>(sourceY) * static_cast<size_t>(width) * 4U;
            for (int x = 0; x < width; ++x)
            {
                const size_t sourceOffset = rowOffset + static_cast<size_t>(x) * 4U;
                const size_t targetOffset = static_cast<size_t>(x) * 4U;
                bgraRow[targetOffset + 0] = pixelsBottomLeft[sourceOffset + 2];
                bgraRow[targetOffset + 1] = pixelsBottomLeft[sourceOffset + 1];
                bgraRow[targetOffset + 2] = pixelsBottomLeft[sourceOffset + 0];
                bgraRow[targetOffset + 3] = pixelsBottomLeft[sourceOffset + 3];
            }

            output.write(reinterpret_cast<const char*>(bgraRow.data()), static_cast<std::streamsize>(bgraRow.size()));
            if (!output)
                return false;
        }

        return true;
    }

    static bool saveRFloatTexture(const std::string& path, int width, int height, const std::vector<float>& pixelsBottomLeft)
    {
        if (width <= 0 || height <= 0)
            return false;

        const size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height);
        if (pixelsBottomLeft.size() < expectedSize)
            return false;

        if (!ensureParentDirectory(path))
            return false;

        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output)
            return false;

        const char magic[4] = {'R', 'T', 'F', '1'};
        const std::uint32_t widthValue = static_cast<std::uint32_t>(width);
        const std::uint32_t heightValue = static_cast<std::uint32_t>(height);
        output.write(magic, sizeof(magic));
        output.write(reinterpret_cast<const char*>(&widthValue), sizeof(widthValue));
        output.write(reinterpret_cast<const char*>(&heightValue), sizeof(heightValue));
        output.write(reinterpret_cast<const char*>(pixelsBottomLeft.data()), static_cast<std::streamsize>(expectedSize * sizeof(float)));
        return static_cast<bool>(output);
    }

    static bool loadRFloatTexture(const std::string& path, int& widthOut, int& heightOut, std::vector<float>& pixelsBottomLeftOut)
    {
        widthOut = 0;
        heightOut = 0;
        pixelsBottomLeftOut.clear();

        std::ifstream input(path, std::ios::binary);
        if (!input)
            return false;

        char magic[4] = {};
        std::uint32_t widthValue = 0;
        std::uint32_t heightValue = 0;
        input.read(magic, sizeof(magic));
        input.read(reinterpret_cast<char*>(&widthValue), sizeof(widthValue));
        input.read(reinterpret_cast<char*>(&heightValue), sizeof(heightValue));
        if (!input || magic[0] != 'R' || magic[1] != 'T' || magic[2] != 'F' || magic[3] != '1' || widthValue == 0 || heightValue == 0)
            return false;

        const size_t pixelCount = static_cast<size_t>(widthValue) * static_cast<size_t>(heightValue);
        pixelsBottomLeftOut.assign(pixelCount, 0.0f);
        input.read(reinterpret_cast<char*>(pixelsBottomLeftOut.data()), static_cast<std::streamsize>(pixelCount * sizeof(float)));
        if (!input)
        {
            pixelsBottomLeftOut.clear();
            return false;
        }

        widthOut = static_cast<int>(widthValue);
        heightOut = static_cast<int>(heightValue);
        return true;
    }
};
}