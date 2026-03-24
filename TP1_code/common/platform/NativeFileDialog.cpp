#include "NativeFileDialog.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#else
#include <cstdio>
#endif

namespace
{
#ifdef _WIN32
std::string buildFilterBuffer(const std::vector<platform::FileDialogFilter>& filters)
{
    std::string buffer;
    for (const platform::FileDialogFilter& filter : filters)
    {
        std::string label = filter.label.empty() ? "Files" : filter.label;
        std::string patternList;
        for (std::size_t index = 0; index < filter.patterns.size(); ++index)
        {
            if (index > 0)
                patternList += ";";
            patternList += filter.patterns[index];
        }

        if (patternList.empty())
            patternList = "*.*";

        buffer += label + "\0" + patternList + "\0";
    }

    if (buffer.empty())
        buffer = std::string("All Files\0*.*\0", 15);

    buffer.push_back('\0');
    return buffer;
}
#else
std::string shellQuote(const std::string& value)
{
    std::string quoted = "'";
    for (char character : value)
    {
        if (character == '\'')
            quoted += "'\\''";
        else
            quoted += character;
    }
    quoted += "'";
    return quoted;
}

bool commandExists(const char* command)
{
    const std::string probe = std::string("command -v ") + command + " >/dev/null 2>&1";
    return std::system(probe.c_str()) == 0;
}

std::optional<std::string> runCommand(const std::string& command)
{
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr)
        return std::nullopt;

    std::string output;
    char buffer[256] = {};
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        output += buffer;

    const int status = pclose(pipe);
    if (status != 0)
        return std::nullopt;

    while (!output.empty() && (output.back() == '\n' || output.back() == '\r'))
        output.pop_back();

    if (output.empty())
        return std::nullopt;

    return output;
}

std::string buildZenityFilterArguments(const std::vector<platform::FileDialogFilter>& filters)
{
    std::string arguments;
    for (const platform::FileDialogFilter& filter : filters)
    {
        if (filter.patterns.empty())
            continue;

        std::ostringstream stream;
        stream << (filter.label.empty() ? "Files" : filter.label) << " |";
        for (const std::string& pattern : filter.patterns)
            stream << ' ' << pattern;

        arguments += " --file-filter=" + shellQuote(stream.str());
    }
    return arguments;
}

std::string buildKdialogFilterString(const std::vector<platform::FileDialogFilter>& filters)
{
    if (filters.empty())
        return "All Files (*)";

    std::ostringstream stream;
    for (std::size_t index = 0; index < filters.size(); ++index)
    {
        if (index > 0)
            stream << '\n';

        stream << (filters[index].label.empty() ? "Files" : filters[index].label) << " (";
        if (filters[index].patterns.empty())
        {
            stream << '*';
        }
        else
        {
            for (std::size_t patternIndex = 0; patternIndex < filters[index].patterns.size(); ++patternIndex)
            {
                if (patternIndex > 0)
                    stream << ' ';
                stream << filters[index].patterns[patternIndex];
            }
        }
        stream << ')';
    }
    return stream.str();
}
#endif
}

namespace platform
{
std::optional<std::string> showNativeFileDialog(
    FileDialogMode mode,
    const std::string& title,
    const std::string& defaultPath,
    const std::vector<FileDialogFilter>& filters)
{
#ifdef _WIN32
    char fileBuffer[MAX_PATH] = {};
    if (!defaultPath.empty())
        strncpy(fileBuffer, defaultPath.c_str(), MAX_PATH - 1);

    std::string filterBuffer = buildFilterBuffer(filters);

    OPENFILENAMEA dialog = {};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = nullptr;
    dialog.lpstrFile = fileBuffer;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrTitle = title.empty() ? nullptr : title.c_str();
    dialog.lpstrFilter = filterBuffer.c_str();
    dialog.nFilterIndex = 1;
    dialog.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST;

    BOOL result = FALSE;
    if (mode == FileDialogMode::OpenFile)
    {
        dialog.Flags |= OFN_FILEMUSTEXIST;
        result = GetOpenFileNameA(&dialog);
    }
    else
    {
        dialog.Flags |= OFN_OVERWRITEPROMPT;
        result = GetSaveFileNameA(&dialog);
    }

    if (!result)
        return std::nullopt;

    return std::string(fileBuffer);
#else
    const bool isSave = mode == FileDialogMode::SaveFile;
    const std::string quotedTitle = shellQuote(title.empty() ? (isSave ? "Save File" : "Open File") : title);
    const std::string quotedDefaultPath = shellQuote(defaultPath);

    if (commandExists("zenity"))
    {
        std::string command = "zenity --file-selection --title=" + quotedTitle;
        if (isSave)
            command += " --save --confirm-overwrite";
        if (!defaultPath.empty())
            command += " --filename=" + quotedDefaultPath;
        command += buildZenityFilterArguments(filters);
        return runCommand(command);
    }

    if (commandExists("qarma"))
    {
        std::string command = "qarma --file-selection --title=" + quotedTitle;
        if (isSave)
            command += " --save --confirm-overwrite";
        if (!defaultPath.empty())
            command += " --filename=" + quotedDefaultPath;
        command += buildZenityFilterArguments(filters);
        return runCommand(command);
    }

    if (commandExists("kdialog"))
    {
        const std::string filterString = shellQuote(buildKdialogFilterString(filters));
        const std::string command =
            std::string("kdialog ") +
            (isSave ? "--getsavefilename " : "--getopenfilename ") +
            quotedDefaultPath + " " + filterString + " --title " + quotedTitle;
        return runCommand(command);
    }

    std::cerr << "No supported native file dialog backend found on this Linux system." << std::endl;
    return std::nullopt;
#endif
}
}