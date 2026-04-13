#pragma once

#include <filesystem>
#include <string>
#include <system_error>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace runtime_app
{
inline bool isBuildRoot(const std::filesystem::path& path)
{
    std::error_code errorCode;
    return !path.empty() && std::filesystem::exists(path / "CMakeCache.txt", errorCode) && !errorCode;
}

inline std::filesystem::path executableDirectory()
{
#if defined(_WIN32)
    std::wstring buffer(MAX_PATH, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length == buffer.size())
        return {};

    buffer.resize(length);
    return std::filesystem::path(buffer).parent_path();
#elif defined(__linux__)
    std::string buffer(4096, '\0');
    const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (length <= 0)
        return {};

    buffer.resize(static_cast<size_t>(length));
    return std::filesystem::path(buffer).parent_path();
#else
    return {};
#endif
}

inline std::filesystem::path detectRuntimeRoot()
{
    const std::filesystem::path executableDir = executableDirectory();
    if (isBuildRoot(executableDir))
        return executableDir;

    if (!executableDir.empty())
    {
        const std::filesystem::path siblingBuild = executableDir.parent_path() / "build";
        if (isBuildRoot(siblingBuild))
            return siblingBuild;

        const std::filesystem::path nestedBuild = executableDir / "build";
        if (isBuildRoot(nestedBuild))
            return nestedBuild;
    }

    std::error_code errorCode;
    const std::filesystem::path cwd = std::filesystem::current_path(errorCode);
    if (errorCode)
        return executableDir.empty() ? std::filesystem::path(".") : executableDir;

    if (isBuildRoot(cwd))
        return cwd;

    const std::filesystem::path parentBuild = cwd.parent_path() / "build";
    if (isBuildRoot(parentBuild))
        return parentBuild;

    const std::filesystem::path childBuild = cwd / "build";
    if (isBuildRoot(childBuild))
        return childBuild;

    if (!executableDir.empty())
        return executableDir;

    return cwd;
}

inline const std::filesystem::path& runtimeRoot()
{
    static const std::filesystem::path root = detectRuntimeRoot();
    return root;
}

inline void adoptProcessWorkingDirectoryToRuntimeRoot()
{
    static const bool applied = []() {
        std::error_code errorCode;
        const std::filesystem::path& root = runtimeRoot();
        if (!root.empty())
            std::filesystem::current_path(root, errorCode);
        return true;
    }();

    (void)applied;
}

inline std::string runtimePath(const std::string& relativePath)
{
    return (runtimeRoot() / std::filesystem::path(relativePath)).lexically_normal().string();
}
}