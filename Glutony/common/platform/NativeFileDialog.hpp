#pragma once

#include <optional>
#include <string>
#include <vector>

namespace platform
{
enum class FileDialogMode
{
    OpenFile,
    SaveFile,
};

struct FileDialogFilter
{
    std::string label;
    std::vector<std::string> patterns;
};

std::optional<std::string> showNativeFileDialog(
    FileDialogMode mode,
    const std::string& title,
    const std::string& defaultPath,
    const std::vector<FileDialogFilter>& filters
);
}