#include "SceneEditorController.hpp"

#include "EditorUiDocuments.hpp"

#include <common/UI/MenuBar.hpp>
#include <common/UI/MenuEntry.hpp>
#include <common/UI/MenuItem.hpp>
#include <common/UI/MarkupBlock.hpp>
#include <common/UI/Placeholder.hpp>
#include <common/UI/Panel.hpp>
#include <common/UI/PanelAction.hpp>
#include <common/UI/PanelHeader.hpp>
#include <common/UI/TabButton.hpp>
#include <common/UI/TabHeader.hpp>
#include <common/UI/TabItem.hpp>
#include <common/UI/TabPanel.hpp>
#include <common/UI/TextBlock.hpp>
#include <common/UI/ToolbarButton.hpp>
#include <common/UI/ToolbarGroup.hpp>

#include <common/asset/TextureAssetIO.hpp>
#include <common/app/RuntimePreviewSession.hpp>
#include <common/app/RuntimePaths.hpp>
#include <common/platform/NativeFileDialog.hpp>
#include <common/Scene.hpp>

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <limits>
#include <sstream>
#include <system_error>
#include <unordered_set>

#ifndef _WIN32
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

using namespace editor_ui;

namespace
{
constexpr int MinAssetBrowserPaneWidth = 160;
constexpr size_t MaxConsoleLines = 256;
constexpr int kPanelHeaderHeight = 34;
constexpr int kContextMenuMinWidth = 144;
constexpr int kContextMenuEstimatedRowHeight = 34;
constexpr int kContextMenuVerticalPadding = 8;

struct MenuPosition
{
    int x = 0;
    int y = 0;
};

MenuPosition clampMenuPosition(Rml::Element* container, int x, int y, int menuWidth, int menuHeight)
{
    if (container == nullptr)
        return {x, y};

    const int containerWidth = static_cast<int>(std::lround(container->GetOffsetWidth()));
    const int containerHeight = static_cast<int>(std::lround(container->GetOffsetHeight()));
    if (containerWidth <= 0 || containerHeight <= 0)
        return {x, y};

    const int maxX = std::max(0, containerWidth - menuWidth - kContextMenuVerticalPadding);
    const int maxY = std::max(0, containerHeight - menuHeight - kContextMenuVerticalPadding);

    return {
        std::clamp(x, 0, maxX),
        std::clamp(y, 0, maxY)
    };
}

int estimateSingleActionMenuHeight(int itemCount)
{
    return std::max(1, itemCount) * kContextMenuEstimatedRowHeight + 2;
}

int estimateAddComponentMenuHeight()
{
    int descriptorCount = 0;
    for (const auto& entry : component_meta::componentDescriptorRegistry())
    {
        if (entry.second != nullptr)
            ++descriptorCount;
    }

    return estimateSingleActionMenuHeight(descriptorCount);
}

std::string buildSceneEditorMenuMarkup(bool isFileMenuOpen, bool isEditMenuOpen, bool isWindowMenuOpen)
{
    UI::MenuBar menuBar(0, 0);
    menuBar.setDomIdOverride("scene_editor_menu_bar");
    menuBar.setSegmentId("menuBar");

    UI::MenuEntry fileMenu(0, 0, "File");
    fileMenu.setDomIdOverride("scene_menu_file");
    fileMenu.setButtonDomIdOverride("scene_menu_file_button");
    fileMenu.setDropdownDomIdOverride("scene_menu_file_dropdown");
    fileMenu.setExpanded(isFileMenuOpen);

    UI::MenuItem saveAs(0, 0, "Save Scene As");
    saveAs.setDomIdOverride("scene_menu_save_as");
    UI::MenuItem loadSave(0, 0, "Load Save");
    loadSave.setDomIdOverride("scene_menu_load_save");
    fileMenu.addChild(&saveAs);
    fileMenu.addChild(&loadSave);

    UI::MenuEntry editMenu(0, 0, "Edit");
    editMenu.setDomIdOverride("scene_menu_edit");
    editMenu.setButtonDomIdOverride("scene_menu_edit_button");
    editMenu.setDropdownDomIdOverride("scene_menu_edit_dropdown");
    editMenu.setExpanded(isEditMenuOpen);

    UI::MenuItem buildItem(0, 0, "Build");
    buildItem.setDomIdOverride("scene_menu_build");
    UI::MenuItem buildRunItem(0, 0, "Build & Run");
    buildRunItem.setDomIdOverride("scene_menu_build_run");
    UI::MenuItem rebuildRenderPipelineItem(0, 0, "Rebuild Render Pipeline");
    rebuildRenderPipelineItem.setDomIdOverride("scene_menu_rebuild_render_pipeline");
    UI::MenuItem bakeRenderTargetsItem(0, 0, "Bake Bakeable Targets");
    bakeRenderTargetsItem.setDomIdOverride("scene_menu_bake_render_targets");
    editMenu.addChild(&buildItem);
    editMenu.addChild(&buildRunItem);
    editMenu.addChild(&rebuildRenderPipelineItem);
    editMenu.addChild(&bakeRenderTargetsItem);

    UI::MenuEntry windowMenu(0, 0, "Window");
    windowMenu.setDomIdOverride("builder_menu_window");
    windowMenu.setButtonDomIdOverride("builder_menu_window_button");
    windowMenu.setDropdownDomIdOverride("builder_menu_window_dropdown");
    windowMenu.setExpanded(isWindowMenuOpen);

    UI::MenuItem openUiBuilder(0, 0, "UI Builder");
    openUiBuilder.setDomIdOverride("builder_menu_open_ui_builder");
    windowMenu.addChild(&openUiBuilder);

    menuBar.addChild(&fileMenu);
    menuBar.addChild(&editMenu);
    menuBar.addChild(&windowMenu);
    return menuBar.getRML();
}

std::string buildInspectorShellMarkup(const std::string& bodyMarkup, const std::string& overlayMarkup)
{
    UI::Panel panel(0, 0);
    panel.addClassName("inspector_shell");

    UI::PanelHeader header(0, 0, "Inspector");
    UI::Container body(0, 0, UI::VERTICAL);
    body.setDomIdOverride("scene_inspector_panel_body");
    body.addClassName("inspector_panel_body");

    UI::MarkupBlock bodyMarkupBlock(0, 0, bodyMarkup);
    body.addChild(&bodyMarkupBlock);

    UI::Container overlay(0, 0, UI::VERTICAL);
    overlay.setDomIdOverride("scene_inspector_overlay");
    overlay.addClassName("inspector_overlay");

    UI::MarkupBlock overlayMarkupBlock(0, 0, overlayMarkup);
    overlay.addChild(&overlayMarkupBlock);

    panel.addChild(&header);
    panel.addChild(&body);
    panel.addChild(&overlay);
    return panel.getRML();
}

std::string buildInspectorPlaceholderMarkup(const std::string& title, const std::string& description)
{
    UI::Placeholder placeholder(0, 0);
    placeholder.setTitle(title);
    placeholder.setDescription(description);
    return placeholder.getRML();
}

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

std::string escapeCppStringLiteral(const std::string& value)
{
    std::ostringstream stream;
    for (char character : value)
    {
        switch (character)
        {
        case '\\':
            stream << "\\\\";
            break;
        case '"':
            stream << "\\\"";
            break;
        case '\n':
            stream << "\\n";
            break;
        case '\t':
            stream << "\\t";
            break;
        default:
            stream << character;
            break;
        }
    }
    return stream.str();
}

std::string normalizeSceneScriptSourcePath(const std::string& rawPath)
{
    std::string normalized = asset::AssetManager::normalizeRelativePath(rawPath);
    const std::string legacyBuiltInPrefix = "built_in/";
    if (editor_ui::startsWith(normalized, legacyBuiltInPrefix))
        normalized.replace(0, legacyBuiltInPrefix.size(), "built-in/");
    return normalized;
}

std::string trimCopy(const std::string& value)
{
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0)
        ++start;

    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
        --end;

    return value.substr(start, end - start);
}

std::string sanitizeAssetToken(const std::string& value)
{
    std::string sanitized;
    sanitized.reserve(value.size());
    for (unsigned char character : value)
    {
        if (std::isalnum(character) != 0 || character == '_' || character == '-')
            sanitized.push_back(static_cast<char>(character));
        else
            sanitized.push_back('_');
    }

    while (!sanitized.empty() && sanitized.front() == '_')
        sanitized.erase(sanitized.begin());
    while (!sanitized.empty() && sanitized.back() == '_')
        sanitized.pop_back();

    return sanitized.empty() ? std::string("target") : sanitized;
}

std::string applyBakedTextureGroupSuffix(const std::string& baseAssetPath, const std::string& groupSuffix)
{
    const std::string normalizedBasePath = asset::AssetManager::normalizeRelativePath(baseAssetPath);
    if (normalizedBasePath.empty() || groupSuffix.empty())
        return normalizedBasePath;

    const std::filesystem::path basePath(normalizedBasePath);
    const std::filesystem::path fileName = basePath.stem().string() + groupSuffix + basePath.extension().string();
    return asset::AssetManager::normalizeRelativePath((basePath.parent_path() / fileName).generic_string());
}

std::string defaultBakedTextureBaseAssetPath(const std::string& currentSceneFilePath, const std::string& targetName, const std::string& extension = ".tga")
{
    std::string sceneStem = "untitled_scene";
    if (!currentSceneFilePath.empty())
        sceneStem = sanitizeAssetToken(std::filesystem::path(currentSceneFilePath).stem().string());

    return asset::AssetManager::normalizeRelativePath(
        (std::filesystem::path("Assets") / "baked" / sceneStem / (sanitizeAssetToken(targetName) + extension)).generic_string());
}

std::string lowercaseCopy(const std::string& value)
{
    std::string lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return lowered;
}

std::string classifyConsoleLine(const std::string& line, const std::string& sourceClass)
{
    if (!sourceClass.empty())
        return sourceClass;

    const std::string lowered = lowercaseCopy(line);
    if (lowered.find("error") != std::string::npos || lowered.find("failed") != std::string::npos)
        return "console_line_error";
    if (lowered.find("warning") != std::string::npos)
        return "console_line_warning";
    if (lowered.find("success") != std::string::npos || lowered.find("built target") != std::string::npos)
        return "console_line_success";
    return "console_line_neutral";
}

std::string ansiClassFromCode(int code)
{
    switch (code)
    {
    case 30:
    case 90:
        return "console_ansi_black";
    case 31:
    case 91:
        return "console_ansi_red";
    case 32:
    case 92:
        return "console_ansi_green";
    case 33:
    case 93:
        return "console_ansi_yellow";
    case 34:
    case 94:
        return "console_ansi_blue";
    case 35:
    case 95:
        return "console_ansi_magenta";
    case 36:
    case 96:
        return "console_ansi_cyan";
    case 37:
    case 97:
        return "console_ansi_white";
    default:
        return "";
    }
}

std::string renderConsoleLineMarkup(const std::string& line, const std::string& fallbackClass)
{
    std::ostringstream stream;
    std::string activeClass = fallbackClass;
    bool activeBold = false;
    bool wroteSegment = false;

    auto appendSegment = [&](const std::string& segment) {
        if (segment.empty())
            return;

        stream << "<span class='console_line_segment";
        if (!activeClass.empty())
            stream << " " << activeClass;
        if (activeBold)
            stream << " console_line_bold";
        stream << "'>" << escapeRmlText(segment) << "</span>";
        wroteSegment = true;
    };

    size_t cursor = 0;
    while (cursor < line.size())
    {
        const size_t escapePosition = line.find("\x1b[", cursor);
        if (escapePosition == std::string::npos)
        {
            appendSegment(line.substr(cursor));
            break;
        }

        appendSegment(line.substr(cursor, escapePosition - cursor));
        const size_t commandEnd = line.find('m', escapePosition + 2);
        if (commandEnd == std::string::npos)
        {
            appendSegment(line.substr(escapePosition));
            break;
        }

        const std::string codeList = line.substr(escapePosition + 2, commandEnd - (escapePosition + 2));
        std::stringstream codeStream(codeList);
        std::string codeToken;
        bool sawCode = false;
        while (std::getline(codeStream, codeToken, ';'))
        {
            sawCode = true;
            const int code = codeToken.empty() ? 0 : std::stoi(codeToken);
            if (code == 0)
            {
                activeClass = fallbackClass;
                activeBold = false;
            }
            else if (code == 1)
            {
                activeBold = true;
            }
            else if (code == 22)
            {
                activeBold = false;
            }
            else
            {
                const std::string ansiClass = ansiClassFromCode(code);
                if (!ansiClass.empty())
                    activeClass = ansiClass;
            }
        }

        if (!sawCode)
        {
            activeClass = fallbackClass;
            activeBold = false;
        }

        cursor = commandEnd + 1;
    }

    if (!wroteSegment)
        appendSegment(line.empty() ? std::string(" ") : line);

    return stream.str();
}

bool writeRuntimePreviewSequenceFile(const std::filesystem::path& tempPath, const std::filesystem::path& targetPath, uint64_t sequence)
{
    std::error_code errorCode;
    std::filesystem::create_directories(targetPath.parent_path(), errorCode);
    if (errorCode)
        return false;

    std::ofstream output(tempPath, std::ios::trunc);
    if (!output)
        return false;

    output << sequence << '\n';
    output.close();

    std::filesystem::rename(tempPath, targetPath, errorCode);
    if (errorCode)
    {
        std::filesystem::remove(tempPath, errorCode);
        return false;
    }

    return true;
}

bool writeRuntimePreviewPauseStateFile(
    const std::filesystem::path& tempPath,
    const std::filesystem::path& targetPath,
    uint64_t sequence,
    bool paused)
{
    std::error_code errorCode;
    std::filesystem::create_directories(targetPath.parent_path(), errorCode);
    if (errorCode)
        return false;

    std::ofstream output(tempPath, std::ios::trunc);
    if (!output)
        return false;

    output << sequence << ' ' << (paused ? 1 : 0) << '\n';
    output.close();

    std::filesystem::rename(tempPath, targetPath, errorCode);
    if (errorCode)
    {
        std::filesystem::remove(tempPath, errorCode);
        return false;
    }

    return true;
}

bool writeRuntimePreviewSelectionFile(
    const std::filesystem::path& tempPath,
    const std::filesystem::path& targetPath,
    uint64_t sequence,
    int selectedGameObjectId)
{
    std::error_code errorCode;
    std::filesystem::create_directories(targetPath.parent_path(), errorCode);
    if (errorCode)
        return false;

    std::ofstream output(tempPath, std::ios::trunc);
    if (!output)
        return false;

    output << sequence << ' ' << selectedGameObjectId << '\n';
    output.close();

    std::filesystem::rename(tempPath, targetPath, errorCode);
    if (errorCode)
    {
        std::filesystem::remove(tempPath, errorCode);
        return false;
    }

    return true;
}

bool writeRuntimePreviewGameObjectFile(
    const std::filesystem::path& tempPath,
    const std::filesystem::path& targetPath,
    uint64_t sequence,
    const scene_serialization::GameObjectSnapshot& snapshot)
{
    std::error_code errorCode;
    std::filesystem::create_directories(targetPath.parent_path(), errorCode);
    if (errorCode)
        return false;

    std::ofstream output(tempPath, std::ios::trunc);
    if (!output)
        return false;

    output << sequence << '\n';
    if (!scene_serialization::detail::saveGameObjectSnapshotToStream(output, snapshot))
        return false;
    output.close();

    std::filesystem::rename(tempPath, targetPath, errorCode);
    if (errorCode)
    {
        std::filesystem::remove(tempPath, errorCode);
        return false;
    }

    return true;
}


#ifndef _WIN32
bool spawnShellProcess(const std::string& workingDirectory, const std::string& command, int& pid, int& outputFd)
{
    int pipeFds[2] = {-1, -1};
    if (pipe(pipeFds) != 0)
        return false;

    const pid_t childPid = fork();
    if (childPid < 0)
    {
        close(pipeFds[0]);
        close(pipeFds[1]);
        return false;
    }

    if (childPid == 0)
    {
        dup2(pipeFds[1], STDOUT_FILENO);
        dup2(pipeFds[1], STDERR_FILENO);
        close(pipeFds[0]);
        close(pipeFds[1]);

        if (!workingDirectory.empty())
            chdir(workingDirectory.c_str());

        execl("/bin/sh", "sh", "-lc", command.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    close(pipeFds[1]);
    const int flags = fcntl(pipeFds[0], F_GETFL, 0);
    fcntl(pipeFds[0], F_SETFL, flags | O_NONBLOCK);
    pid = static_cast<int>(childPid);
    outputFd = pipeFds[0];
    return true;
}

bool spawnCapturedProcess(const std::string& workingDirectory, const std::filesystem::path& executablePath, const std::vector<std::string>& arguments, int& pid, int& outputFd)
{
    int pipeFds[2] = {-1, -1};
    if (pipe(pipeFds) != 0)
        return false;

    const pid_t childPid = fork();
    if (childPid < 0)
    {
        close(pipeFds[0]);
        close(pipeFds[1]);
        return false;
    }

    if (childPid == 0)
    {
        dup2(pipeFds[1], STDOUT_FILENO);
        dup2(pipeFds[1], STDERR_FILENO);
        close(pipeFds[0]);
        close(pipeFds[1]);

        if (!workingDirectory.empty())
            chdir(workingDirectory.c_str());

        std::vector<std::string> ownedArguments;
        ownedArguments.push_back(executablePath.string());
        ownedArguments.insert(ownedArguments.end(), arguments.begin(), arguments.end());

        std::vector<char*> argv;
        argv.reserve(ownedArguments.size() + 1);
        for (std::string& argument : ownedArguments)
            argv.push_back(argument.data());
        argv.push_back(nullptr);

        execv(executablePath.c_str(), argv.data());
        _exit(127);
    }

    close(pipeFds[1]);
    const int flags = fcntl(pipeFds[0], F_GETFL, 0);
    fcntl(pipeFds[0], F_SETFL, flags | O_NONBLOCK);
    pid = static_cast<int>(childPid);
    outputFd = pipeFds[0];
    return true;
}

bool spawnDetachedProcess(const std::string& workingDirectory, const std::filesystem::path& executablePath, const std::vector<std::string>& arguments)
{
    const pid_t childPid = fork();
    if (childPid < 0)
        return false;

    if (childPid > 0)
        return true;

    setsid();

    if (!workingDirectory.empty())
        chdir(workingDirectory.c_str());

    FILE* nullFile = fopen("/dev/null", "w");
    if (nullFile != nullptr)
    {
        dup2(fileno(nullFile), STDOUT_FILENO);
        dup2(fileno(nullFile), STDERR_FILENO);
        dup2(fileno(nullFile), STDIN_FILENO);
    }

    std::vector<std::string> ownedArguments;
    ownedArguments.push_back(executablePath.string());
    ownedArguments.insert(ownedArguments.end(), arguments.begin(), arguments.end());

    std::vector<char*> argv;
    argv.reserve(ownedArguments.size() + 1);
    for (std::string& argument : ownedArguments)
        argv.push_back(argument.data());
    argv.push_back(nullptr);

    execv(executablePath.c_str(), argv.data());
    _exit(127);
}
#endif

std::string formatFloat(float value)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(6) << value;
    return trimCopy(stream.str());
}

std::string formatSerializedValue(const component_meta::SerializedValue& value)
{
    return std::visit([](const auto& storedValue) -> std::string {
        using StoredType = std::decay_t<decltype(storedValue)>;

        if constexpr (std::is_same_v<StoredType, bool>)
        {
            return storedValue ? "true" : "false";
        }
        else if constexpr (std::is_same_v<StoredType, int>)
        {
            return std::to_string(storedValue);
        }
        else if constexpr (std::is_same_v<StoredType, float>)
        {
            return formatFloat(storedValue);
        }
        else if constexpr (std::is_same_v<StoredType, glm::vec3>)
        {
            return formatFloat(storedValue.x) + ", " + formatFloat(storedValue.y) + ", " + formatFloat(storedValue.z);
        }
        else
        {
            return storedValue;
        }
    }, value);
}

std::string makeSceneInspectorFieldElementId(int nodeId, size_t componentIndex, const std::string& fieldKey)
{
    return "scene_inspector_field_" + std::to_string(nodeId) + "__" + std::to_string(componentIndex) + "__" + fieldKey;
}

std::string makeSceneTransformFieldElementId(int nodeId, const std::string& fieldKey)
{
    return "scene_transform_field_" + std::to_string(nodeId) + "__" + fieldKey;
}

std::string makeSceneRootFieldElementId(const std::string& fieldKey)
{
    return "scene_root_field__" + fieldKey;
}

std::string makeSceneInspectorGroupElementId(int nodeId, size_t componentIndex)
{
    return "scene_inspector_group_" + std::to_string(nodeId) + "__" + std::to_string(componentIndex);
}

bool parseVec3(const std::string& rawValue, glm::vec3& outValue)
{
    std::string normalized = rawValue;
    for (char& character : normalized)
    {
        if (character == ',')
            character = ' ';
    }

    std::istringstream stream(normalized);
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    if (!(stream >> x >> y >> z))
        return false;

    stream >> std::ws;
    if (!stream.eof())
        return false;

    outValue = glm::vec3(x, y, z);
    return true;
}

bool parseSerializedValue(component_meta::FieldKind kind, const std::string& rawValue, component_meta::SerializedValue& outValue)
{
    const std::string trimmedValue = trimCopy(rawValue);

    try
    {
        switch (kind)
        {
        case component_meta::FieldKind::Bool:
            if (trimmedValue == "true" || trimmedValue == "1")
            {
                outValue = true;
                return true;
            }
            if (trimmedValue == "false" || trimmedValue == "0")
            {
                outValue = false;
                return true;
            }
            return false;

        case component_meta::FieldKind::Int:
        {
            size_t parsedLength = 0;
            const int parsedValue = std::stoi(trimmedValue, &parsedLength);
            if (parsedLength != trimmedValue.size())
                return false;
            outValue = parsedValue;
            return true;
        }

        case component_meta::FieldKind::Float:
        {
            size_t parsedLength = 0;
            const float parsedValue = std::stof(trimmedValue, &parsedLength);
            if (parsedLength != trimmedValue.size())
                return false;
            outValue = parsedValue;
            return true;
        }

        case component_meta::FieldKind::Vec3:
        {
            glm::vec3 parsedValue(0.0f, 0.0f, 0.0f);
            if (!parseVec3(trimmedValue, parsedValue))
                return false;
            outValue = parsedValue;
            return true;
        }

        case component_meta::FieldKind::String:
        case component_meta::FieldKind::Enum:
        case component_meta::FieldKind::Asset:
            outValue = trimmedValue;
            return true;
        }
    }
    catch (const std::exception&)
    {
        return false;
    }

    return false;
}

const char* assetReferenceKindLabel(component_meta::AssetReferenceKind kind)
{
    switch (kind)
    {
    case component_meta::AssetReferenceKind::Mesh:
        return "Drop mesh asset";
    case component_meta::AssetReferenceKind::Shader:
        return "Drop shader asset";
    case component_meta::AssetReferenceKind::Material:
        return "Drop material asset";
    case component_meta::AssetReferenceKind::Texture:
        return "Drop texture asset";
    case component_meta::AssetReferenceKind::Data:
        return "Drop data asset";
    case component_meta::AssetReferenceKind::SceneScript:
        return "Drop scene script asset";
    case component_meta::AssetReferenceKind::ComponentScript:
        return "Drop component script asset";
    case component_meta::AssetReferenceKind::Scene:
        return "Drop scene asset";
    case component_meta::AssetReferenceKind::Generic:
        return "Drop asset";
    case component_meta::AssetReferenceKind::None:
    default:
        return "";
    }
}

std::string buildInspectorFieldMarkup(
    const std::string& fieldId,
    const std::string& fieldLabel,
    component_meta::FieldKind fieldKind,
    const component_meta::SerializedValue& value,
    const std::vector<component_meta::EnumOption>& enumOptions,
    component_meta::AssetReferenceKind assetReferenceKind,
    bool dropHighlighted)
{
    std::ostringstream stream;
    stream << "<div class='inspector_field_row'><div class='inspector_field_name'>" << escapeRmlText(fieldLabel) << "</div>";

    const std::string formattedValue = formatSerializedValue(value);
    const bool isAssetField = assetReferenceKind != component_meta::AssetReferenceKind::None || fieldKind == component_meta::FieldKind::Asset;
    const std::string fieldClass = std::string("inspector_field_input") +
        (isAssetField ? " inspector_asset_field" : "") +
        (dropHighlighted ? " inspector_asset_field_active" : "");

    if (fieldKind == component_meta::FieldKind::Bool)
    {
        stream << "<select id='" << fieldId << "' class='" << fieldClass << "'>";
        stream << "<option value='true'" << (formattedValue == "true" ? " selected='selected'" : "") << ">True</option>";
        stream << "<option value='false'" << (formattedValue == "false" ? " selected='selected'" : "") << ">False</option>";
        stream << "</select>";
    }
    else if (fieldKind == component_meta::FieldKind::Enum)
    {
        stream << "<select id='" << fieldId << "' class='" << fieldClass << "'>";
        for (const component_meta::EnumOption& option : enumOptions)
        {
            stream << "<option value='" << escapeRmlText(option.value) << "'";
            if (option.value == formattedValue)
                stream << " selected='selected'";
            stream << ">" << escapeRmlText(option.label) << "</option>";
        }
        stream << "</select>";
    }
    else
    {
        stream << "<input id='" << fieldId << "' class='" << fieldClass << "' type='";
        stream << ((fieldKind == component_meta::FieldKind::Float || fieldKind == component_meta::FieldKind::Int) ? "number" : "text");
        stream << "' value='" << escapeRmlText(formattedValue) << "'";
        if (isAssetField)
            stream << " placeholder='" << escapeRmlText(assetReferenceKindLabel(assetReferenceKind)) << "'";
        stream << " />";
    }

    if (isAssetField)
        stream << "<div class='inspector_asset_hint'>" << escapeRmlText(assetReferenceKindLabel(assetReferenceKind)) << "</div>";

    stream << "</div>";
    return stream.str();
}

std::string buildInspectorFieldMarkup(
    int nodeId,
    size_t componentIndex,
    const component_meta::ComponentFieldDescriptor& field,
    const component_meta::SerializedValue& value,
    bool dropHighlighted)
{
    const std::string fieldId = makeSceneInspectorFieldElementId(nodeId, componentIndex, field.key);
    return buildInspectorFieldMarkup(fieldId, field.label, field.kind, value, field.enumOptions, field.assetReferenceKind, dropHighlighted);
}

Rml::String findAncestorElementId(Rml::Element* targetElement, const std::function<bool(const Rml::String&)>& predicate)
{
    for (Rml::Element* element = targetElement; element != nullptr; element = element->GetParentNode())
    {
        const Rml::String elementId = element->GetId();
        if (!elementId.empty() && predicate(elementId))
            return elementId;
    }
    return "";
}

Rml::Element* findAncestorElement(Rml::Element* targetElement, const std::function<bool(const Rml::Element&)>& predicate)
{
    for (Rml::Element* element = targetElement; element != nullptr; element = element->GetParentNode())
    {
        if (predicate(*element))
            return element;
    }

    return nullptr;
}

std::string encodeElementToken(const std::string& value)
{
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (unsigned char character : value)
        stream << std::setw(2) << static_cast<int>(character);
    return stream.str();
}

int decodeHexDigit(char value)
{
    if (value >= '0' && value <= '9')
        return value - '0';
    if (value >= 'a' && value <= 'f')
        return 10 + (value - 'a');
    if (value >= 'A' && value <= 'F')
        return 10 + (value - 'A');
    return -1;
}

std::optional<std::string> decodeElementToken(const std::string& token)
{
    if ((token.size() % 2) != 0)
        return std::nullopt;

    std::string decoded;
    decoded.reserve(token.size() / 2);

    for (size_t index = 0; index < token.size(); index += 2)
    {
        const int high = decodeHexDigit(token[index]);
        const int low = decodeHexDigit(token[index + 1]);
        if (high < 0 || low < 0)
            return std::nullopt;

        decoded.push_back(static_cast<char>((high << 4) | low));
    }

    return decoded;
}

bool parseSceneRenderTargetFieldKey(const std::string& fieldKey, std::string& renderTargetName, std::string& propertyKey)
{
    const std::string prefix = "render_target__";
    if (!startsWith(fieldKey, prefix))
        return false;

    const size_t separator = fieldKey.find("__", prefix.size());
    if (separator == std::string::npos)
        return false;

    const std::optional<std::string> decodedName = decodeElementToken(fieldKey.substr(prefix.size(), separator - prefix.size()));
    if (!decodedName.has_value())
        return false;

    renderTargetName = render::normalizeRenderTargetName(*decodedName);
    propertyKey = fieldKey.substr(separator + 2);
    return !renderTargetName.empty() && !propertyKey.empty();
}

template <typename Predicate>
std::optional<std::string> parseEncodedElementId(const Rml::String& elementId, const std::string& prefix, Predicate&& validator)
{
    const std::string value = elementId;
    if (!startsWith(value, prefix))
        return std::nullopt;

    const std::optional<std::string> decoded = decodeElementToken(value.substr(prefix.size()));
    if (!decoded.has_value() || !validator(*decoded))
        return std::nullopt;

    return decoded;
}

std::filesystem::file_time_type safeLastWriteTimeLocal(const std::string& path)
{
    std::error_code errorCode;
    const std::filesystem::file_time_type writeTime = std::filesystem::last_write_time(path, errorCode);
    if (errorCode)
        return std::filesystem::file_time_type::min();
    return writeTime;
}

const char* assetBrowserRootLabel(SceneEditorController::AssetBrowserRootKind rootKind)
{
    return rootKind == SceneEditorController::AssetBrowserRootKind::Assets ? "Assets" : "built-in";
}

const char* assetBrowserRootClass(SceneEditorController::AssetBrowserRootKind rootKind)
{
    return rootKind == SceneEditorController::AssetBrowserRootKind::Assets ? "user" : "builtin";
}

bool isShaderAssetDirectory(const std::filesystem::path& path)
{
    std::error_code errorCode;
    if (!std::filesystem::exists(path, errorCode) || !std::filesystem::is_directory(path, errorCode))
        return false;

    bool hasVertexShader = false;
    bool hasFragmentShader = false;
    int regularFileCount = 0;

    for (std::filesystem::directory_iterator iterator(path, errorCode); !errorCode && iterator != std::filesystem::directory_iterator(); iterator.increment(errorCode))
    {
        const std::filesystem::directory_entry& entry = *iterator;
        if (entry.is_directory(errorCode))
            return false;
        if (!entry.is_regular_file(errorCode))
            return false;

        ++regularFileCount;
        const std::string fileName = entry.path().filename().string();
        if (fileName == "vertex.glsl")
            hasVertexShader = true;
        else if (fileName == "fragment.glsl")
            hasFragmentShader = true;
        else
            return false;
    }

    if (errorCode)
        return false;

    return regularFileCount == 2 && hasVertexShader && hasFragmentShader;
}

SceneEditorController::AssetBrowserFileKind classifyAssetBrowserFileKind(const std::filesystem::path& path)
{
    if (isShaderAssetDirectory(path))
        return SceneEditorController::AssetBrowserFileKind::Shader;

    const std::string extension = path.extension().string();
    if (extension == ".mat")
        return SceneEditorController::AssetBrowserFileKind::Material;
    if (extension == ".render_phase")
        return SceneEditorController::AssetBrowserFileKind::RenderPhase;
    if (extension == ".render_pass")
        return SceneEditorController::AssetBrowserFileKind::RenderPass;
    if (extension == ".uniform_factory")
        return SceneEditorController::AssetBrowserFileKind::UniformFactory;
    if (extension == ".data")
        return SceneEditorController::AssetBrowserFileKind::Data;
    if (extension == ".obj" || extension == ".off")
        return SceneEditorController::AssetBrowserFileKind::Mesh;
    if (extension == ".scene_script")
        return SceneEditorController::AssetBrowserFileKind::SceneScript;
    if (extension == ".component_script")
        return SceneEditorController::AssetBrowserFileKind::ComponentScript;
    if (extension == ".glsl")
        return SceneEditorController::AssetBrowserFileKind::Shader;
    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp" || extension == ".tga")
        return SceneEditorController::AssetBrowserFileKind::Texture;
    if (extension == ".scene" || extension == ".snapshot")
        return SceneEditorController::AssetBrowserFileKind::Scene;
    return SceneEditorController::AssetBrowserFileKind::Generic;
}

DragPayloadKind dragPayloadKindForAssetFileKind(SceneEditorController::AssetBrowserFileKind fileKind)
{
    switch (fileKind)
    {
    case SceneEditorController::AssetBrowserFileKind::Material:
        return DragPayloadKind::MaterialAsset;
    case SceneEditorController::AssetBrowserFileKind::RenderPhase:
        return DragPayloadKind::RenderPhaseAsset;
    case SceneEditorController::AssetBrowserFileKind::RenderPass:
        return DragPayloadKind::RenderPassAsset;
    case SceneEditorController::AssetBrowserFileKind::UniformFactory:
        return DragPayloadKind::UniformFactoryAsset;
    case SceneEditorController::AssetBrowserFileKind::Data:
        return DragPayloadKind::DataAsset;
    case SceneEditorController::AssetBrowserFileKind::Mesh:
        return DragPayloadKind::MeshAsset;
    case SceneEditorController::AssetBrowserFileKind::SceneScript:
        return DragPayloadKind::SceneScriptAsset;
    case SceneEditorController::AssetBrowserFileKind::ComponentScript:
        return DragPayloadKind::ComponentScriptAsset;
    case SceneEditorController::AssetBrowserFileKind::Shader:
        return DragPayloadKind::ShaderAsset;
    case SceneEditorController::AssetBrowserFileKind::Texture:
        return DragPayloadKind::TextureAsset;
    default:
        return DragPayloadKind::AssetFile;
    }
}

const char* assetBrowserFileKindLabel(SceneEditorController::AssetBrowserFileKind fileKind)
{
    switch (fileKind)
    {
    case SceneEditorController::AssetBrowserFileKind::Material:
        return "Material";
    case SceneEditorController::AssetBrowserFileKind::RenderPhase:
        return "RenderPhase";
    case SceneEditorController::AssetBrowserFileKind::RenderPass:
        return "RenderPass";
    case SceneEditorController::AssetBrowserFileKind::UniformFactory:
        return "UniformFactory";
    case SceneEditorController::AssetBrowserFileKind::Data:
        return "Data";
    case SceneEditorController::AssetBrowserFileKind::Mesh:
        return "Mesh";
    case SceneEditorController::AssetBrowserFileKind::SceneScript:
        return "SceneScript";
    case SceneEditorController::AssetBrowserFileKind::ComponentScript:
        return "ComponentScript";
    case SceneEditorController::AssetBrowserFileKind::Shader:
        return "Shader";
    case SceneEditorController::AssetBrowserFileKind::Texture:
        return "Texture";
    case SceneEditorController::AssetBrowserFileKind::Scene:
        return "Scene";
    default:
        return "File";
    }
}

const char* assetBrowserFileKindClass(SceneEditorController::AssetBrowserFileKind fileKind)
{
    switch (fileKind)
    {
    case SceneEditorController::AssetBrowserFileKind::Material:
        return "material";
    case SceneEditorController::AssetBrowserFileKind::RenderPhase:
        return "render_phase";
    case SceneEditorController::AssetBrowserFileKind::RenderPass:
        return "render_pass";
    case SceneEditorController::AssetBrowserFileKind::UniformFactory:
        return "uniform_factory";
    case SceneEditorController::AssetBrowserFileKind::Data:
        return "data";
    case SceneEditorController::AssetBrowserFileKind::Mesh:
        return "mesh";
    case SceneEditorController::AssetBrowserFileKind::SceneScript:
        return "scene_script";
    case SceneEditorController::AssetBrowserFileKind::ComponentScript:
        return "component_script";
    case SceneEditorController::AssetBrowserFileKind::Shader:
        return "shader";
    case SceneEditorController::AssetBrowserFileKind::Texture:
        return "texture";
    case SceneEditorController::AssetBrowserFileKind::Scene:
        return "scene";
    default:
        return "generic";
    }
}

std::vector<platform::FileDialogFilter> buildSceneFileDialogFilters()
{
    return {
        {"Scene Save", {"*.scene", "*.snapshot", "*.txt"}},
        {"All Files", {"*"}},
    };
}
}

bool SceneEditorController::initialize(Rml::Context* context)
{
    m_context = context;
    m_assetBrowserModel.clear();
    m_hierarchyModel.clear();
    if (m_context != nullptr)
    {
        const Rml::Vector2i dimensions = m_context->GetDimensions();
        m_layoutManager.setWindowSize(std::max(dimensions.x, 1), std::max(dimensions.y, 1));
    }
    return m_context != nullptr;
}

void SceneEditorController::shutdown()
{
    deactivate();
    m_context = nullptr;
}

void SceneEditorController::activate()
{
    if (m_context == nullptr || m_document != nullptr)
        return;

    m_document = m_context->LoadDocumentFromMemory(getEditorLayoutDocument(), "[scene-editor]");
    if (m_document == nullptr)
        return;

    m_root = m_document->GetElementById("root");
    m_builderHeader = m_document->GetElementById("builder_header");
    m_leftPanel = m_document->GetElementById("left_panel");
    m_leftSplitter = m_document->GetElementById("left_splitter");
    m_centerPanel = m_document->GetElementById("center_panel");
    m_viewportPanel = m_document->GetElementById("viewport_panel");
    m_viewportSurface = nullptr;
    m_horizontalSplitter = m_document->GetElementById("horizontal_splitter");
    m_bottomPanel = m_document->GetElementById("bottom_panel");
    m_rightSplitter = m_document->GetElementById("right_splitter");
    m_rightPanel = m_document->GetElementById("right_panel");

    if (m_root == nullptr ||
        m_builderHeader == nullptr ||
        m_leftPanel == nullptr ||
        m_leftSplitter == nullptr ||
        m_centerPanel == nullptr ||
        m_viewportPanel == nullptr ||
        m_horizontalSplitter == nullptr ||
        m_bottomPanel == nullptr ||
        m_rightSplitter == nullptr ||
        m_rightPanel == nullptr)
    {
        deactivate();
        return;
    }

    const Rml::Vector2i dimensions = m_context->GetDimensions();
    m_windowWidth = std::max(dimensions.x, 1);
    m_windowHeight = std::max(dimensions.y, 1);
    m_layoutManager.setWindowSize(m_windowWidth, m_windowHeight);

    m_assetBrowserModel.rescan(std::filesystem::current_path());
    attachListeners();
    refreshPresentation();
    m_document->Show();
    applyLayout();
    refreshCachedRects();
}

void SceneEditorController::deactivate()
{
    detachListeners();
    stopExternalProcess(false);

    if (m_context != nullptr && m_document != nullptr)
        m_context->UnloadDocument(m_document);

    m_document = nullptr;
    m_root = nullptr;
    m_builderHeader = nullptr;
    m_leftPanel = nullptr;
    m_leftSplitter = nullptr;
    m_centerPanel = nullptr;
    m_viewportPanel = nullptr;
    m_viewportSurface = nullptr;
    m_horizontalSplitter = nullptr;
    m_bottomPanel = nullptr;
    m_bottomBrowserFilesPane = nullptr;
    m_bottomBrowserSplitter = nullptr;
    m_bottomBrowserTreePane = nullptr;
    m_rightSplitter = nullptr;
    m_rightPanel = nullptr;
    m_isWindowMenuOpen = false;
    m_assetBrowserContextMenuOpen = false;
    m_scene = nullptr;
    m_dragTarget = DragTarget::None;
    m_dragPayloadKind = DragPayloadKind::None;
    m_draggedAssetFileId.clear();
    m_draggedAssetRuntimePath.clear();
    m_isFileMenuOpen = false;
    m_runtimeSceneSnapshot.reset();
    m_assetBrowserContextMenuOpen = false;
    m_hierarchyContextMenuOpen = false;
    m_addComponentMenuOpen = false;
    m_inspectorComponentContextMenuOpen = false;
    m_sceneSavePromptOpen = false;
    m_pendingSceneAction = PendingSceneAction::None;
    m_pendingSceneTargetPath.clear();
    m_hoveredInspectorFieldId.clear();
    m_pendingLaunchAction = PendingLaunchAction::None;
    m_pendingLaunchScenePath.clear();
    m_consolePartialLine.clear();
    m_assetBrowserModel.clear();
    m_hierarchyModel.clear();
    m_layoutManager.clear();
}

void SceneEditorController::setModeChangeCallback(const std::function<void(EditorMode)>& callback)
{
    m_modeChangeCallback = callback;
}

void SceneEditorController::syncToWindow(int width, int height)
{
    m_windowWidth = std::max(width, 1);
    m_windowHeight = std::max(height, 1);
    m_layoutManager.setWindowSize(m_windowWidth, m_windowHeight);
}

void SceneEditorController::sync(Scene& scene)
{
    m_scene = &scene;
    const UiGOHierarchyNode previousHierarchy = m_hierarchyModel.root();
    const int previousSelectedHierarchyNodeId = m_hierarchyModel.selectedNodeId();

    m_hierarchyModel.rebuildFromScene(scene);

    if (const GameObject* selectedGameObject = scene.getSelectedGameObject())
    {
        if (const UiGOHierarchyNode* selectedNode = m_hierarchyModel.findNodeByGameObject(selectedGameObject))
            m_hierarchyModel.setSelectedNodeId(selectedNode->id);
    }
    else
    {
        m_hierarchyModel.setSelectedNodeId(m_hierarchyModel.root().id);
    }

    if (m_hierarchyModel.findNodeById(m_hierarchyModel.selectedNodeId()) == nullptr)
        m_hierarchyModel.setSelectedNodeId(m_hierarchyModel.root().id);

    const bool hierarchyChanged = !UI::SceneEditorHierarchyModel::nodesEqual(previousHierarchy, m_hierarchyModel.root());
    const bool selectionChanged = previousSelectedHierarchyNodeId != m_hierarchyModel.selectedNodeId();
    if (hierarchyChanged)
        requestHierarchyRefresh();
    else if (selectionChanged)
        requestSelectionRefresh();
}

void SceneEditorController::setShowStylePanel(bool showStylePanel)
{
    (void)showStylePanel;
}

void SceneEditorController::update()
{
    if (m_context == nullptr || m_document == nullptr)
        return;

    pollExternalProcess();
    pollRuntimePreviewSceneState();
    pollRuntimePreviewState();
    pollRuntimePreviewDataAssetState();
    pollDataAssetExternalChanges();
    syncRuntimePreviewSelectionIfNeeded();
    syncRuntimePreviewGameObjectIfNeeded();
    syncRuntimePreviewSceneIfNeeded();

    if (m_hierarchyRefreshPending)
    {
        refreshHierarchyPresentation();
        if (shouldRefreshInspectorPresentation())
            refreshInspectorPresentation(true);
        else
            refreshInspectorValuesPresentation();
        m_hierarchyRefreshPending = false;
    }
    else if (m_selectionRefreshPending)
    {
        refreshHierarchyPresentation();
        refreshInspectorPresentation();
        m_selectionRefreshPending = false;
    }
    else if (m_consoleRefreshPending)
    {
        refreshBottomPanelPresentation(true);
        m_consoleRefreshPending = false;
    }
    else if (shouldRefreshInspectorPresentation())
    {
        refreshInspectorValuesPresentation();
    }

    updatePlaybackStatusPresentation();
    applyLayout();
    m_context->Update();
    refreshCachedRects();
}

void SceneEditorController::render()
{
    if (m_context != nullptr)
        m_context->Render();
}

UiRect SceneEditorController::getViewportRect() const
{
    return m_layoutManager.viewportRect();
}

bool SceneEditorController::isViewportHovered(double mouseX, double mouseY) const
{
    return m_layoutManager.viewportRect().contains(mouseX, mouseY);
}

bool SceneEditorController::isDragging() const
{
    return m_dragTarget != DragTarget::None;
}

bool SceneEditorController::isExternalPreviewActive() const
{
    return m_activeProcessKind != ActiveProcessKind::None || m_pendingLaunchAction != PendingLaunchAction::None;
}

void SceneEditorController::ProcessEvent(Rml::Event& event)
{
    if (m_root == nullptr)
        return;

    const Rml::EventId eventId = event.GetId();
    Rml::Element* targetElement = event.GetTargetElement();
    const Rml::String elementId = targetElement ? targetElement->GetId() : "";
    const Rml::Vector2f mousePosition = event.GetUnprojectedMouseScreenPos();
    const float mouseX = mousePosition.x;
    const float mouseY = mousePosition.y;
    const Rml::String playButtonElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_play_button"; });
    const Rml::String pauseButtonElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_pause_button"; });
    const Rml::String stopButtonElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_stop_button"; });
    const Rml::String buildButtonElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_build_button"; });
    const Rml::String buildRunButtonElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_build_run_button"; });
    const Rml::String assetBrowserTabElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_bottom_tab_asset_browser"; });
    const Rml::String consoleTabElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_bottom_tab_console"; });
    const Rml::String clearConsoleElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_console_clear"; });
    const Rml::String fileMenuButtonElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_menu_file_button"; });
    const Rml::String editMenuButtonElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_menu_edit_button"; });
    const Rml::String saveSceneAsElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_menu_save_as"; });
    const Rml::String loadSceneElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_menu_load_save"; });
    const Rml::String buildMenuItemElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_menu_build"; });
    const Rml::String buildRunMenuItemElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_menu_build_run"; });
    const Rml::String rebuildRenderPipelineMenuItemElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_menu_rebuild_render_pipeline"; });
    const Rml::String bakeRenderTargetsMenuItemElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_menu_bake_render_targets"; });
    const Rml::String windowMenuButtonElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "builder_menu_window_button"; });
    const Rml::String openUiBuilderElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "builder_menu_open_ui_builder"; });
    const Rml::String hierarchyNodeElementId = ::findAncestorElementId(
        targetElement,
        [&](const Rml::String& candidateId) { return UI::SceneEditorDomIdCodec::parseHierarchyNodeId(candidateId).has_value(); });
    const Rml::String assetDirectoryElementId = ::findAncestorElementId(
        targetElement,
        [&](const Rml::String& candidateId) { return UI::SceneEditorDomIdCodec::parseAssetDirectoryElementId(candidateId).has_value(); });
    const Rml::String assetDirectoryToggleElementId = ::findAncestorElementId(
        targetElement,
        [&](const Rml::String& candidateId) { return UI::SceneEditorDomIdCodec::parseAssetDirectoryToggleElementId(candidateId).has_value(); });
    const Rml::String assetFileElementId = ::findAncestorElementId(
        targetElement,
        [&](const Rml::String& candidateId) { return UI::SceneEditorDomIdCodec::parseAssetFileElementId(candidateId).has_value(); });
    Rml::Element* assetBrowserWorkspaceElement = ::findAncestorElement(
        targetElement,
        [](const Rml::Element& candidate) { return candidate.GetId() == "scene_asset_browser_workspace"; });
    const Rml::String inspectorGroupElementId = ::findAncestorElementId(
        targetElement,
        [&](const Rml::String& candidateId) { return parseInspectorGroupElementId(candidateId).has_value(); });
    const Rml::String inspectorFieldElementId = ::findAncestorElementId(
        targetElement,
        [&](const Rml::String& candidateId) { return parseInspectorFieldElementId(candidateId).has_value(); });
    const Rml::String materialAssetFieldElementId = ::findAncestorElementId(
        targetElement,
        [&](const Rml::String& candidateId) { return parseMaterialAssetEditorFieldElementId(candidateId).has_value(); });
    const Rml::String materialAssetGroupElementId = ::findAncestorElementId(
        targetElement,
        [&](const Rml::String& candidateId) { return parseMaterialAssetEditorGroupElementId(candidateId).has_value(); });
    const Rml::String dataAssetFieldElementId = ::findAncestorElementId(
        targetElement,
        [&](const Rml::String& candidateId) { return parseDataAssetEditorFieldElementId(candidateId).has_value(); });
    const Rml::String dataAssetGroupElementId = ::findAncestorElementId(
        targetElement,
        [&](const Rml::String& candidateId) { return parseDataAssetEditorGroupElementId(candidateId).has_value(); });
    const Rml::String dataAssetNodeGroupElementId = ::findAncestorElementId(
        targetElement,
        [&](const Rml::String& candidateId) { return parseDataAssetEditorNodeGroupElementId(candidateId).has_value(); });
    const Rml::String addGameObjectElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_hierarchy_add"; });
    const Rml::String deleteHierarchyElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_hierarchy_delete"; });
    const Rml::String promptSaveElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_dirty_prompt_save"; });
    const Rml::String promptDiscardElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_dirty_prompt_discard"; });
    const Rml::String promptCancelElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_dirty_prompt_cancel"; });
    const Rml::String addComponentButtonElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_inspector_add_component"; });
    const Rml::String addComponentItemElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return startsWith(std::string(candidateId), "scene_add_component_"); });
    const Rml::String deleteInspectorComponentElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_inspector_component_delete"; });

    if (m_sceneSavePromptOpen)
    {
        if (eventId == Rml::EventId::Click)
        {
            if (!promptSaveElementId.empty())
            {
                if (saveScene())
                {
                    m_sceneSavePromptOpen = false;
                    executePendingSceneAction();
                    refreshPresentation();
                }
                requestHierarchyRefresh();
                event.StopPropagation();
                return;
            }

            if (!promptDiscardElementId.empty())
            {
                m_sceneSavePromptOpen = false;
                if (executePendingSceneAction())
                    clearSceneDirty();
                refreshPresentation();
                requestHierarchyRefresh();
                event.StopPropagation();
                return;
            }

            if (!promptCancelElementId.empty())
            {
                closePendingSceneActionPrompt();
                refreshPresentation();
                requestHierarchyRefresh();
                event.StopPropagation();
                return;
            }
        }

        event.StopPropagation();
        return;
    }

    if (eventId == Rml::EventId::Change || eventId == Rml::EventId::Blur)
    {
        const std::optional<DataAssetEditorBinding> dataAssetField = parseDataAssetEditorFieldElementId(elementId);
        if (dataAssetField.has_value())
        {
            const Rml::ElementFormControl* formControl = dynamic_cast<const Rml::ElementFormControl*>(targetElement);
            if (formControl == nullptr)
                return;

            const std::string tagName = targetElement->GetTagName();
            if (eventId == Rml::EventId::Change && tagName == "input")
            {
                const std::string inputType = targetElement->GetAttribute<Rml::String>("type", "text").c_str();
                if (inputType == "text" || inputType == "number")
                    return;
            }

            if (applyDataAssetEditorFieldValue(*dataAssetField, formControl->GetValue().c_str()))
                refreshDataAssetEditorPresentation(dataAssetField->parentField);

            event.StopPropagation();
            return;
        }

        const std::optional<MaterialAssetEditorBinding> materialAssetField = parseMaterialAssetEditorFieldElementId(elementId);
        if (materialAssetField.has_value())
        {
            const Rml::ElementFormControl* formControl = dynamic_cast<const Rml::ElementFormControl*>(targetElement);
            if (formControl == nullptr)
                return;

            const std::string tagName = targetElement->GetTagName();
            if (eventId == Rml::EventId::Change && tagName == "input")
            {
                const std::string inputType = targetElement->GetAttribute<Rml::String>("type", "text").c_str();
                if (inputType == "text" || inputType == "number")
                    return;
            }

            if (applyMaterialAssetEditorFieldValue(*materialAssetField, formControl->GetValue().c_str()))
                refreshMaterialAssetEditorPresentation(materialAssetField->parentField);

            event.StopPropagation();
            return;
        }

        const std::optional<InspectorFieldBinding> inspectorField = parseInspectorFieldElementId(elementId);
        if (!inspectorField.has_value())
            return;

        const Rml::ElementFormControl* formControl = dynamic_cast<const Rml::ElementFormControl*>(targetElement);
        if (formControl == nullptr)
            return;

        const std::string tagName = targetElement->GetTagName();
        if (eventId == Rml::EventId::Change && tagName == "input")
        {
            const std::string inputType = targetElement->GetAttribute<Rml::String>("type", "text").c_str();
            if (inputType == "text" || inputType == "number")
                return;
        }

        if (applyInspectorFieldValue(*inspectorField, formControl->GetValue().c_str()))
        {
            if (inspectorField->target == InspectorFieldBinding::Target::Scene)
                markSceneDirty(true);
            else
            {
                markSceneDirty(false);
                queueRuntimeGameObjectSync(inspectorField->nodeId);
            }
            refreshInspectorValuesPresentation();
            if (isMaterialAssetInspectorField(*inspectorField))
                refreshMaterialAssetEditorPresentation(*inspectorField);
            else if (isDataAssetInspectorField(*inspectorField))
                refreshDataAssetEditorPresentation(*inspectorField);
        }

        event.StopPropagation();
        return;
    }

    if (eventId == Rml::EventId::Click)
    {
        if (!playButtonElementId.empty())
        {
            if (m_activeProcessKind == ActiveProcessKind::Player && m_playbackState == PlaybackState::Paused)
                resumePreviewPlayer();
            else if (m_activeProcessKind == ActiveProcessKind::None)
                startBuild(PendingLaunchAction::PlayPreview);
            refreshViewportPresentation();
            event.StopPropagation();
            return;
        }

        if (!pauseButtonElementId.empty())
        {
            pausePreviewPlayer();
            refreshViewportPresentation();
            event.StopPropagation();
            return;
        }

        if (!stopButtonElementId.empty())
        {
            stopExternalProcess();
            refreshViewportPresentation();
            event.StopPropagation();
            return;
        }

        if (!buildButtonElementId.empty())
        {
            startBuild(PendingLaunchAction::None);
            refreshViewportPresentation();
            event.StopPropagation();
            return;
        }

        if (!buildRunButtonElementId.empty())
        {
            startBuild(PendingLaunchAction::RunDetached);
            refreshViewportPresentation();
            event.StopPropagation();
            return;
        }

        if (!assetBrowserTabElementId.empty())
        {
            m_bottomPanelTab = BottomPanelTab::AssetBrowser;
            refreshBottomPanelPresentation(true);
            event.StopPropagation();
            return;
        }

        if (!consoleTabElementId.empty())
        {
            m_bottomPanelTab = BottomPanelTab::Console;
            refreshBottomPanelPresentation(true);
            event.StopPropagation();
            return;
        }

        if (!clearConsoleElementId.empty())
        {
            clearConsole();
            refreshBottomPanelPresentation(true);
            event.StopPropagation();
            return;
        }

        if (!fileMenuButtonElementId.empty())
        {
            m_isFileMenuOpen = !m_isFileMenuOpen;
            m_isEditMenuOpen = false;
            m_isWindowMenuOpen = false;
            refreshPresentation();
            event.StopPropagation();
            return;
        }

        if (!editMenuButtonElementId.empty())
        {
            m_isFileMenuOpen = false;
            m_isEditMenuOpen = !m_isEditMenuOpen;
            m_isWindowMenuOpen = false;
            refreshPresentation();
            event.StopPropagation();
            return;
        }

        if (!saveSceneAsElementId.empty())
        {
            closeHeaderMenus();
            saveSceneAs();
            refreshPresentation();
            event.StopPropagation();
            return;
        }

        if (!loadSceneElementId.empty())
        {
            closeHeaderMenus();
            beginPendingSceneAction(PendingSceneAction::LoadFromDialog);
            refreshPresentation();
            event.StopPropagation();
            return;
        }

        if (!buildMenuItemElementId.empty())
        {
            closeHeaderMenus();
            startBuild(PendingLaunchAction::None);
            refreshPresentation();
            event.StopPropagation();
            return;
        }

        if (!buildRunMenuItemElementId.empty())
        {
            closeHeaderMenus();
            startBuild(PendingLaunchAction::RunDetached);
            refreshPresentation();
            event.StopPropagation();
            return;
        }

        if (!rebuildRenderPipelineMenuItemElementId.empty())
        {
            closeHeaderMenus();
            rebuildSceneRenderPipelines();
            refreshPresentation();
            event.StopPropagation();
            return;
        }

        if (!bakeRenderTargetsMenuItemElementId.empty())
        {
            closeHeaderMenus();
            bakeSceneRenderTargets();
            refreshPresentation();
            event.StopPropagation();
            return;
        }

        if (!addGameObjectElementId.empty())
        {
            if (m_scene != nullptr)
            {
                GameObject* gameObject = m_scene->addGameObject("GameObject");
                m_scene->setSelectedGameObject(gameObject);
                sync(*m_scene);
                markSceneDirty();
            }

            event.StopPropagation();
            return;
        }

        if (!deleteHierarchyElementId.empty())
        {
            m_hierarchyContextMenuOpen = false;
            if (m_scene != nullptr)
            {
                const UiGOHierarchyNode* node = m_hierarchyModel.findNodeById(m_hierarchyContextMenuNodeId);
                if (node != nullptr && node->gameObject != nullptr)
                {
                    m_scene->removeGameObject(node->gameObject->getId());
                    sync(*m_scene);
                    markSceneDirty();
                }
            }

            event.StopPropagation();
            return;
        }

        if (!addComponentButtonElementId.empty())
        {
            m_addComponentMenuOpen = !m_addComponentMenuOpen;
            if (m_rightPanel != nullptr)
            {
                const MenuPosition position = clampMenuPosition(
                    m_rightPanel,
                    static_cast<int>(std::lround(mouseX - m_rightPanel->GetAbsoluteLeft())) - 12,
                    static_cast<int>(std::lround(mouseY - m_rightPanel->GetAbsoluteTop())) + 8,
                    220,
                    estimateAddComponentMenuHeight());
                m_addComponentMenuX = position.x;
                m_addComponentMenuY = position.y;
            }

            refreshInspectorOverlayPresentation();
            event.StopPropagation();
            return;
        }

        if (!addComponentItemElementId.empty())
        {
            const std::string prefix = "scene_add_component_";
            const std::optional<std::string> typeKey = decodeElementToken(std::string(addComponentItemElementId).substr(prefix.size()));
            const component_meta::ComponentDescriptor* descriptor = typeKey.has_value() ? component_meta::findComponentDescriptor(*typeKey) : nullptr;
            const UiGOHierarchyNode* selectedNode = m_hierarchyModel.findSelectedNode();

            m_addComponentMenuOpen = false;
            if (descriptor != nullptr && descriptor->factory != nullptr && selectedNode != nullptr && selectedNode->gameObject != nullptr && m_scene != nullptr)
            {
                GameObject* gameObject = const_cast<GameObject*>(selectedNode->gameObject);
                if (component::Component* component = descriptor->factory(gameObject))
                {
                    gameObject->addComponent(component);
                    markSceneDirty();
                    sync(*m_scene);
                    requestSelectionRefresh();
                }
            }

            event.StopPropagation();
            return;
        }

        if (!deleteInspectorComponentElementId.empty())
        {
            m_inspectorComponentContextMenuOpen = false;
            if (m_scene != nullptr)
            {
                UiGOHierarchyNode* node = m_hierarchyModel.findNodeById(m_inspectorComponentContextMenuNodeId);
                GameObject* gameObject = node != nullptr ? const_cast<GameObject*>(node->gameObject) : nullptr;
                if (gameObject != nullptr && gameObject->removeComponentAt(m_inspectorComponentContextMenuIndex))
                {
                    markSceneDirty();
                    sync(*m_scene);
                    requestSelectionRefresh();
                }
            }

            event.StopPropagation();
            return;
        }

        if (::findAncestorElementId(targetElement, [](const Rml::String& candidateId) { return candidateId == "scene_asset_browser_refresh"; }) == "scene_asset_browser_refresh")
        {
            m_assetBrowserContextMenuOpen = false;
            m_assetBrowserModel.rescan(std::filesystem::current_path());
            refreshAssetBrowserWorkspacePresentation(true);
            event.StopPropagation();
            return;
        }

        if (!inspectorGroupElementId.empty())
        {
            toggleInspectorGroup(inspectorGroupElementId);
            refreshInspectorPresentation(true);
            event.StopPropagation();
            return;
        }

        const std::optional<InspectorFieldBinding> materialAssetGroup = parseMaterialAssetEditorGroupElementId(materialAssetGroupElementId);
        if (materialAssetGroup.has_value())
        {
            toggleMaterialAssetEditor(*materialAssetGroup);
            refreshMaterialAssetEditorPresentation(*materialAssetGroup);
            event.StopPropagation();
            return;
        }

        const std::optional<InspectorFieldBinding> dataAssetGroup = parseDataAssetEditorGroupElementId(dataAssetGroupElementId);
        if (dataAssetGroup.has_value())
        {
            toggleDataAssetEditor(*dataAssetGroup);
            refreshInspectorPresentation(true);
            event.StopPropagation();
            return;
        }

        const std::optional<DataAssetEditorBinding> dataAssetNodeGroup = parseDataAssetEditorNodeGroupElementId(dataAssetNodeGroupElementId);
        if (dataAssetNodeGroup.has_value())
        {
            toggleDataAssetEditorNode(*dataAssetNodeGroup);
            refreshInspectorPresentation(true);
            event.StopPropagation();
            return;
        }

        if (!windowMenuButtonElementId.empty())
        {
            m_isFileMenuOpen = false;
            m_isEditMenuOpen = false;
            m_isWindowMenuOpen = !m_isWindowMenuOpen;
            refreshPresentation();
            event.StopPropagation();
            return;
        }

        if (!openUiBuilderElementId.empty())
        {
            closeHeaderMenus();
            refreshPresentation();
            if (m_modeChangeCallback)
                m_modeChangeCallback(EditorMode::UiBuilder);
            event.StopPropagation();
            return;
        }

        if (const std::optional<std::string> directoryId = UI::SceneEditorDomIdCodec::parseAssetDirectoryToggleElementId(assetDirectoryToggleElementId))
        {
            if (const AssetBrowserDirectoryNode* directory = m_assetBrowserModel.findDirectoryById(*directoryId))
            {
                if (!directory->children.empty())
                    m_assetBrowserModel.toggleDirectoryExpansion(*directoryId);
            }

            refreshAssetBrowserTreePresentation(true);
            event.StopPropagation();
            return;
        }

        if (const std::optional<std::string> directoryId = UI::SceneEditorDomIdCodec::parseAssetDirectoryElementId(assetDirectoryElementId))
        {
            m_assetBrowserContextMenuOpen = false;
            m_hierarchyContextMenuOpen = false;
            const std::string previousDirectoryId = m_assetBrowserModel.selectedDirectoryId();
            m_assetBrowserModel.selectDirectory(*directoryId);
            refreshAssetBrowserDirectorySelectionPresentation(previousDirectoryId);
            refreshAssetBrowserFilesPresentation(false);
            refreshAssetBrowserOverlayPresentation();
            event.StopPropagation();
            return;
        }

        if (const std::optional<std::string> fileId = UI::SceneEditorDomIdCodec::parseAssetFileElementId(assetFileElementId))
        {
            m_assetBrowserContextMenuOpen = false;
            m_hierarchyContextMenuOpen = false;
            const std::string previousFileId = m_assetBrowserModel.selectedFileId();
            m_assetBrowserModel.setSelectedFileId(*fileId);
            refreshAssetBrowserFileSelectionPresentation(previousFileId);
            event.StopPropagation();
            return;
        }

        if (const std::optional<int> hierarchyNodeId = UI::SceneEditorDomIdCodec::parseHierarchyNodeId(hierarchyNodeElementId))
        {
            m_hierarchyContextMenuOpen = false;
            const UiGOHierarchyNode* hierarchyNode = m_hierarchyModel.findNodeById(*hierarchyNodeId);
            if (hierarchyNode != nullptr && m_scene != nullptr)
            {
                GameObject* clickedGameObject = const_cast<GameObject*>(hierarchyNode->gameObject);
                m_scene->toggleSelectedGameObject(clickedGameObject);
                if (m_activeProcessKind == ActiveProcessKind::Player)
                    m_pendingRuntimeSelectionId = m_scene->getSelectedGameObject() != nullptr ? std::optional<int>(m_scene->getSelectedGameObject()->getId()) : std::optional<int>(-1);
                m_hierarchyModel.setSelectedNodeId((clickedGameObject != nullptr && m_scene->getSelectedGameObject() == clickedGameObject)
                    ? *hierarchyNodeId
                    : m_hierarchyModel.root().id);
            }
            else
            {
                m_hierarchyModel.setSelectedNodeId(*hierarchyNodeId);
            }

            requestHierarchyRefresh();
            event.StopPropagation();
            return;
        }

        if (m_isFileMenuOpen || m_isEditMenuOpen || m_isWindowMenuOpen)
        {
            const Rml::String menuHit = ::findAncestorElementId(targetElement, [](const Rml::String& candidateId) {
                return candidateId == "scene_editor_menu_bar";
            });
            if (menuHit.empty())
            {
                closeHeaderMenus();
                refreshPresentation();
            }
        }

        if (m_assetBrowserContextMenuOpen && assetBrowserWorkspaceElement == nullptr)
        {
            m_assetBrowserContextMenuOpen = false;
            refreshAssetBrowserOverlayPresentation();
        }

        if (m_hierarchyContextMenuOpen && deleteHierarchyElementId.empty() && hierarchyNodeElementId.empty())
        {
            m_hierarchyContextMenuOpen = false;
            requestHierarchyRefresh();
        }

        if (m_addComponentMenuOpen && addComponentButtonElementId.empty() && addComponentItemElementId.empty())
        {
            m_addComponentMenuOpen = false;
            refreshInspectorOverlayPresentation();
        }

        if (m_inspectorComponentContextMenuOpen && deleteInspectorComponentElementId.empty() && inspectorGroupElementId.empty())
        {
            m_inspectorComponentContextMenuOpen = false;
            refreshInspectorOverlayPresentation();
        }
        return;
    }

    if (eventId == Rml::EventId::Dblclick)
    {
        if (const std::optional<std::string> fileId = UI::SceneEditorDomIdCodec::parseAssetFileElementId(assetFileElementId))
        {
            if (const AssetBrowserFileEntry* file = m_assetBrowserModel.findFileById(*fileId))
            {
                if (file->fileKind == AssetBrowserFileKind::Scene)
                {
                    beginPendingSceneAction(PendingSceneAction::OpenFile, file->diskPath);
                    refreshPresentation();
                    event.StopPropagation();
                    return;
                }
            }
        }
    }

    if (eventId == Rml::EventId::Dragstart)
    {
        if (const std::optional<std::string> fileId = UI::SceneEditorDomIdCodec::parseAssetFileElementId(assetFileElementId))
        {
            if (const AssetBrowserFileEntry* file = m_assetBrowserModel.findFileById(*fileId))
            {
                m_dragPayloadKind = file->dragPayloadKind;
                m_draggedAssetFileId = *fileId;
                m_draggedAssetRuntimePath = file->runtimePath;
                m_assetBrowserModel.setSelectedFileId(*fileId);
                event.StopPropagation();
                return;
            }
        }
    }

    if (eventId == Rml::EventId::Dragover)
    {
        const std::optional<InspectorFieldBinding> inspectorField = parseInspectorFieldElementId(inspectorFieldElementId);
        const std::optional<MaterialAssetEditorBinding> materialAssetField = parseMaterialAssetEditorFieldElementId(materialAssetFieldElementId);

        std::string nextHoveredFieldId;
        if (materialAssetField.has_value() && canDropDraggedAssetOnMaterialAssetEditorField(*materialAssetField))
            nextHoveredFieldId = materialAssetFieldElementId;
        else if (inspectorField.has_value() && canDropDraggedAssetOnInspectorField(*inspectorField))
            nextHoveredFieldId = inspectorFieldElementId;

        if (nextHoveredFieldId != m_hoveredInspectorFieldId)
            m_hoveredInspectorFieldId = nextHoveredFieldId;

        if (!m_hoveredInspectorFieldId.empty())
        {
            event.StopPropagation();
            return;
        }
    }

    if (eventId == Rml::EventId::Dragdrop)
    {
        const std::optional<MaterialAssetEditorBinding> materialAssetField = parseMaterialAssetEditorFieldElementId(materialAssetFieldElementId);
        if (materialAssetField.has_value() && applyDraggedAssetToMaterialAssetEditorField(*materialAssetField))
        {
            m_hoveredInspectorFieldId.clear();
            refreshMaterialAssetEditorPresentation(materialAssetField->parentField);
            event.StopPropagation();
            return;
        }

        const std::optional<InspectorFieldBinding> inspectorField = parseInspectorFieldElementId(inspectorFieldElementId);
        if (inspectorField.has_value() && applyDraggedAssetToInspectorField(*inspectorField))
        {
            m_hoveredInspectorFieldId.clear();
            if (inspectorField->target == InspectorFieldBinding::Target::Scene)
                markSceneDirty(true);
            else
            {
                markSceneDirty(false);
                queueRuntimeGameObjectSync(inspectorField->nodeId);
            }
            refreshInspectorValuesPresentation();
            if (isMaterialAssetInspectorField(*inspectorField))
                refreshMaterialAssetEditorPresentation(*inspectorField);
            else if (isDataAssetInspectorField(*inspectorField))
                refreshDataAssetEditorPresentation(*inspectorField);
            else
                requestSelectionRefresh();
            event.StopPropagation();
            return;
        }
    }

    if (eventId == Rml::EventId::Dragend)
    {
        m_dragPayloadKind = DragPayloadKind::None;
        m_draggedAssetFileId.clear();
        m_draggedAssetRuntimePath.clear();
        m_hoveredInspectorFieldId.clear();
        return;
    }

    if (eventId == Rml::EventId::Mousedown)
    {
        const int mouseButton = event.GetParameter<int>("button", 0);
        if (mouseButton == 1 && assetBrowserWorkspaceElement != nullptr)
        {
            closeHeaderMenus();
            m_hierarchyContextMenuOpen = false;
            m_addComponentMenuOpen = false;
            m_inspectorComponentContextMenuOpen = false;
            m_assetBrowserContextMenuOpen = true;
            const MenuPosition position = clampMenuPosition(
                assetBrowserWorkspaceElement,
                static_cast<int>(std::lround(mouseX - assetBrowserWorkspaceElement->GetAbsoluteLeft())),
                static_cast<int>(std::lround(mouseY - assetBrowserWorkspaceElement->GetAbsoluteTop())),
                kContextMenuMinWidth,
                estimateSingleActionMenuHeight(1));
            m_assetBrowserContextMenuX = position.x;
            m_assetBrowserContextMenuY = position.y;
            refreshAssetBrowserOverlayPresentation();
            event.StopPropagation();
            return;
        }

        if (mouseButton == 1)
        {
            if (const std::optional<int> hierarchyNodeId = UI::SceneEditorDomIdCodec::parseHierarchyNodeId(hierarchyNodeElementId))
            {
                m_assetBrowserContextMenuOpen = false;
                m_addComponentMenuOpen = false;
                m_inspectorComponentContextMenuOpen = false;
                m_hierarchyContextMenuOpen = true;
                m_hierarchyContextMenuNodeId = *hierarchyNodeId;
                const MenuPosition position = clampMenuPosition(
                    m_leftPanel,
                    static_cast<int>(std::lround(mouseX - m_leftPanel->GetAbsoluteLeft())),
                    static_cast<int>(std::lround(mouseY - m_leftPanel->GetAbsoluteTop())),
                    kContextMenuMinWidth,
                    estimateSingleActionMenuHeight(1));
                m_hierarchyContextMenuX = position.x;
                m_hierarchyContextMenuY = position.y;

                const UiGOHierarchyNode* hierarchyNode = m_hierarchyModel.findNodeById(*hierarchyNodeId);
                if (hierarchyNode != nullptr && m_scene != nullptr)
                {
                    m_scene->setSelectedGameObject(const_cast<GameObject*>(hierarchyNode->gameObject));
                    if (m_activeProcessKind == ActiveProcessKind::Player)
                        m_pendingRuntimeSelectionId = hierarchyNode->gameObject != nullptr ? std::optional<int>(hierarchyNode->gameObject->getId()) : std::optional<int>(-1);
                    m_hierarchyModel.setSelectedNodeId(*hierarchyNodeId);
                }

                requestSelectionRefresh();
                event.StopPropagation();
                return;
            }

            if (const std::optional<InspectorGroupBinding> inspectorGroup = parseInspectorGroupElementId(inspectorGroupElementId))
            {
                m_assetBrowserContextMenuOpen = false;
                m_hierarchyContextMenuOpen = false;
                m_addComponentMenuOpen = false;
                m_inspectorComponentContextMenuOpen = true;
                m_inspectorComponentContextMenuNodeId = inspectorGroup->nodeId;
                m_inspectorComponentContextMenuIndex = inspectorGroup->componentIndex;
                const MenuPosition position = clampMenuPosition(
                    m_rightPanel,
                    static_cast<int>(std::lround(mouseX - m_rightPanel->GetAbsoluteLeft())),
                    static_cast<int>(std::lround(mouseY - m_rightPanel->GetAbsoluteTop())),
                    kContextMenuMinWidth,
                    estimateSingleActionMenuHeight(1));
                m_inspectorComponentContextMenuX = position.x;
                m_inspectorComponentContextMenuY = position.y;
                refreshInspectorOverlayPresentation();
                event.StopPropagation();
                return;
            }

            if (m_hierarchyContextMenuOpen)
            {
                m_hierarchyContextMenuOpen = false;
                requestHierarchyRefresh();
            }

            if (m_addComponentMenuOpen && addComponentButtonElementId.empty() && addComponentItemElementId.empty())
            {
                m_addComponentMenuOpen = false;
                refreshInspectorOverlayPresentation();
            }

            if (m_inspectorComponentContextMenuOpen && deleteInspectorComponentElementId.empty() && inspectorGroupElementId.empty())
            {
                m_inspectorComponentContextMenuOpen = false;
                refreshInspectorOverlayPresentation();
            }
        }

        if (elementId == "left_splitter")
            m_dragTarget = DragTarget::LeftSplitter;
        else if (elementId == "right_splitter")
            m_dragTarget = DragTarget::RightSplitter;
        else if (elementId == "horizontal_splitter")
            m_dragTarget = DragTarget::HorizontalSplitter;
        else if (elementId == "scene_asset_browser_splitter")
            m_dragTarget = DragTarget::BottomBrowserSplitter;

        if (m_dragTarget != DragTarget::None)
            event.StopPropagation();
        return;
    }

    if (eventId == Rml::EventId::Mouseup)
    {
        m_dragTarget = DragTarget::None;
        return;
    }

    if (eventId != Rml::EventId::Mousemove || m_dragTarget == DragTarget::None)
        return;

    if (m_dragTarget == DragTarget::LeftSplitter || m_dragTarget == DragTarget::RightSplitter)
    {
        if (m_dragTarget == DragTarget::LeftSplitter)
            m_layoutManager.dragLeftSplitter(mouseX);
        else
            m_layoutManager.dragRightSplitter(mouseX);

        applyLayout();
        event.StopPropagation();
        return;
    }

    if (m_dragTarget == DragTarget::HorizontalSplitter)
    {
        m_layoutManager.dragHorizontalSplitter(mouseY);
        applyLayout();
        event.StopPropagation();
        return;
    }

    if (m_dragTarget == DragTarget::BottomBrowserSplitter && m_bottomPanel != nullptr)
    {
        m_layoutManager.dragBottomBrowserSplitter(mouseX);
        applyLayout();
        event.StopPropagation();
    }
}

void SceneEditorController::attachListeners()
{
    if (m_document == nullptr)
        return;

    m_document->AddEventListener(Rml::EventId::Change, this);
    m_document->AddEventListener(Rml::EventId::Blur, this, true);
    m_document->AddEventListener(Rml::EventId::Click, this);
    m_document->AddEventListener(Rml::EventId::Dblclick, this);
    m_document->AddEventListener(Rml::EventId::Dragstart, this);
    m_document->AddEventListener(Rml::EventId::Dragover, this);
    m_document->AddEventListener(Rml::EventId::Dragdrop, this);
    m_document->AddEventListener(Rml::EventId::Dragend, this);
    m_document->AddEventListener(Rml::EventId::Mousedown, this);
    m_document->AddEventListener(Rml::EventId::Mousemove, this);
    m_document->AddEventListener(Rml::EventId::Mouseup, this);
}

void SceneEditorController::detachListeners()
{
    if (m_document == nullptr)
        return;

    m_document->RemoveEventListener(Rml::EventId::Change, this);
    m_document->RemoveEventListener(Rml::EventId::Blur, this, true);
    m_document->RemoveEventListener(Rml::EventId::Click, this);
    m_document->RemoveEventListener(Rml::EventId::Dblclick, this);
    m_document->RemoveEventListener(Rml::EventId::Dragstart, this);
    m_document->RemoveEventListener(Rml::EventId::Dragover, this);
    m_document->RemoveEventListener(Rml::EventId::Dragdrop, this);
    m_document->RemoveEventListener(Rml::EventId::Dragend, this);
    m_document->RemoveEventListener(Rml::EventId::Mousedown, this);
    m_document->RemoveEventListener(Rml::EventId::Mousemove, this);
    m_document->RemoveEventListener(Rml::EventId::Mouseup, this);
}

void SceneEditorController::applyLayout()
{
    m_layoutManager.setElements({
        m_root,
        m_builderHeader,
        m_leftPanel,
        m_leftSplitter,
        m_centerPanel,
        m_viewportPanel,
        m_horizontalSplitter,
        m_bottomPanel,
        m_bottomBrowserFilesPane,
        m_bottomBrowserSplitter,
        m_bottomBrowserTreePane,
        m_rightSplitter,
        m_rightPanel,
    });
    m_layoutManager.setViewportSurface(m_viewportSurface);
    m_layoutManager.applyLayout();
}

bool SceneEditorController::shouldRefreshInspectorPresentation() const
{
    if (m_context == nullptr)
        return true;

    const Rml::Element* focusedElement = m_context->GetFocusElement();
    if (focusedElement == nullptr)
        return true;

    for (const Rml::Element* element = focusedElement; element != nullptr; element = element->GetParentNode())
    {
        const Rml::String elementId = element->GetId();
        if (!elementId.empty() && parseInspectorFieldElementId(elementId).has_value())
            return false;
    }

    return true;
}

const component_meta::ComponentFieldDescriptor* SceneEditorController::findInspectorFieldDescriptor(const InspectorFieldBinding& binding) const
{
    if (binding.target != InspectorFieldBinding::Target::Component)
        return nullptr;

    const UiGOHierarchyNode* node = m_hierarchyModel.findNodeById(binding.nodeId);
    if (node == nullptr || node->gameObject == nullptr)
        return nullptr;

    const component::Component* component = node->gameObject->getComponentAt(binding.componentIndex);
    if (component == nullptr)
        return nullptr;

    const component_meta::ComponentDescriptor* descriptor = component->getComponentDescriptor();
    if (descriptor == nullptr)
        return nullptr;

    return component_meta::findComponentFieldDescriptor(*descriptor, binding.fieldKey);
}

bool SceneEditorController::applyInspectorFieldValue(const InspectorFieldBinding& binding, const std::string& value)
{
    if (binding.target == InspectorFieldBinding::Target::Scene)
    {
        if (m_scene == nullptr)
            return false;

        if (binding.fieldKey == "directionalLightEnabled" ||
            binding.fieldKey == "directionalLightDirection" ||
            binding.fieldKey == "directionalLightColor" ||
            binding.fieldKey == "directionalLightIntensity")
        {
            render::DirectionalLightSettings settings = m_scene->getDirectionalLightSettings();

            if (binding.fieldKey == "directionalLightEnabled")
            {
                component_meta::SerializedValue parsedValue;
                if (!parseSerializedValue(component_meta::FieldKind::Bool, value, parsedValue))
                    return false;

                const bool enabled = std::get<bool>(parsedValue);
                if (settings.enabled == enabled)
                    return false;
                settings.enabled = enabled;
                return m_scene->setDirectionalLightSettings(settings);
            }

            if (binding.fieldKey == "directionalLightIntensity")
            {
                component_meta::SerializedValue parsedValue;
                if (!parseSerializedValue(component_meta::FieldKind::Float, value, parsedValue))
                    return false;

                const float intensity = std::get<float>(parsedValue);
                if (settings.intensity == intensity)
                    return false;
                settings.intensity = intensity;
                return m_scene->setDirectionalLightSettings(settings);
            }

            glm::vec3 parsedVec3(0.0f, 0.0f, 0.0f);
            if (!parseVec3(value, parsedVec3))
                return false;

            if (binding.fieldKey == "directionalLightDirection")
            {
                if (formatSerializedValue(component_meta::SerializedValue(settings.direction)) == formatSerializedValue(component_meta::SerializedValue(parsedVec3)))
                    return false;
                settings.direction = parsedVec3;
                return m_scene->setDirectionalLightSettings(settings);
            }

            if (binding.fieldKey == "directionalLightColor")
            {
                if (formatSerializedValue(component_meta::SerializedValue(settings.color)) == formatSerializedValue(component_meta::SerializedValue(parsedVec3)))
                    return false;
                settings.color = parsedVec3;
                return m_scene->setDirectionalLightSettings(settings);
            }

            return false;
        }

        std::string renderTargetName;
        std::string renderTargetProperty;
        if (parseSceneRenderTargetFieldKey(binding.fieldKey, renderTargetName, renderTargetProperty))
        {
            render::SceneRenderTargetSettings settings = m_scene->resolveRenderTargetSettings(renderTargetName);

            if (renderTargetProperty == "width" || renderTargetProperty == "height")
            {
                component_meta::SerializedValue parsedValue;
                if (!parseSerializedValue(component_meta::FieldKind::Int, value, parsedValue))
                    return false;

                const int numericValue = std::max(0, std::get<int>(parsedValue));
                if (renderTargetProperty == "width")
                {
                    if (settings.width == numericValue)
                        return false;
                    settings.width = numericValue;
                }
                else
                {
                    if (settings.height == numericValue)
                        return false;
                    settings.height = numericValue;
                }

                return m_scene->upsertRenderTargetSettings(settings);
            }

            if (renderTargetProperty == "format")
            {
                render::RenderTargetFormat format = settings.format;
                if (!render::parseRenderTargetFormat(value, format) || settings.format == format)
                    return false;
                settings.format = format;
                return m_scene->upsertRenderTargetSettings(settings);
            }

            if (renderTargetProperty == "bakedTexture")
            {
                const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(value);
                if (!normalizedPath.empty() && !asset::AssetManager::isTextureAssetPath(normalizedPath))
                    return false;
                if (settings.bakedTextureAssetPath == normalizedPath)
                    return false;
                settings.bakedTextureAssetPath = normalizedPath;
                return m_scene->upsertRenderTargetSettings(settings);
            }

            return false;
        }

        const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(value);
        if (binding.fieldKey == "sceneScriptAsset")
        {
            if (!normalizedPath.empty() && !asset::AssetManager::hasExtension(normalizedPath, ".scene_script"))
                return false;
            if (m_scene->getSceneScriptAssetPath() == normalizedPath)
                return false;
            m_scene->setSceneScriptAssetPath(normalizedPath);
            return true;
        }

        if (binding.fieldKey == "dataAsset")
        {
            if (!normalizedPath.empty() && !asset::AssetManager::hasExtension(normalizedPath, ".data"))
                return false;
            if (m_scene->getDataAssetPath() == normalizedPath)
                return false;
            m_scene->setDataAssetPath(normalizedPath);
            return true;
        }

        return false;
    }

    UiGOHierarchyNode* node = m_hierarchyModel.findNodeById(binding.nodeId);
    if (node == nullptr || node->gameObject == nullptr)
        return false;

    GameObject* gameObject = const_cast<GameObject*>(node->gameObject);

    if (binding.target == InspectorFieldBinding::Target::Transform)
    {
        glm::vec3 parsedValue(0.0f, 0.0f, 0.0f);
        if (!parseVec3(value, parsedValue))
            return false;

        if (binding.fieldKey == "position")
        {
            if (formatSerializedValue(gameObject->transform.getPosition()) == formatSerializedValue(parsedValue))
                return false;
            gameObject->transform.setPosition(parsedValue);
            return true;
        }

        if (binding.fieldKey == "rotation")
        {
            if (formatSerializedValue(gameObject->transform.getRotation()) == formatSerializedValue(parsedValue))
                return false;
            gameObject->transform.setRotation(parsedValue);
            return true;
        }

        if (binding.fieldKey == "scale")
        {
            if (formatSerializedValue(gameObject->transform.getScale()) == formatSerializedValue(parsedValue))
                return false;
            gameObject->transform.setScale(parsedValue);
            return true;
        }

        return false;
    }

    component::Component* component = gameObject->getComponentAt(binding.componentIndex);
    if (component == nullptr)
        return false;

    const component_meta::ComponentDescriptor* descriptor = component->getComponentDescriptor();
    if (descriptor == nullptr)
        return false;

    const component_meta::ComponentFieldDescriptor* field = component_meta::findComponentFieldDescriptor(*descriptor, binding.fieldKey);
    if (field == nullptr || !field->write)
        return false;

    component_meta::SerializedValue parsedValue;
    if (!parseSerializedValue(field->kind, value, parsedValue))
        return false;

    if (field->read && formatSerializedValue(field->read(*component)) == formatSerializedValue(parsedValue))
        return false;

    return field->write(*component, parsedValue);
}

bool SceneEditorController::canDropDraggedAssetOnInspectorField(const InspectorFieldBinding& binding) const
{
    if (m_dragPayloadKind == DragPayloadKind::None || m_draggedAssetRuntimePath.empty())
        return false;

    if (binding.target == InspectorFieldBinding::Target::Scene)
    {
        if (binding.fieldKey == "sceneScriptAsset")
            return m_dragPayloadKind == DragPayloadKind::SceneScriptAsset;
        if (binding.fieldKey == "dataAsset")
            return m_dragPayloadKind == DragPayloadKind::DataAsset;
        return false;
    }

    const component_meta::ComponentFieldDescriptor* field = findInspectorFieldDescriptor(binding);
    if (field == nullptr || field->assetReferenceKind == component_meta::AssetReferenceKind::None)
        return false;

    switch (field->assetReferenceKind)
    {
    case component_meta::AssetReferenceKind::Mesh:
        return m_dragPayloadKind == DragPayloadKind::MeshAsset;
    case component_meta::AssetReferenceKind::Shader:
        return m_dragPayloadKind == DragPayloadKind::ShaderAsset;
    case component_meta::AssetReferenceKind::Material:
        return m_dragPayloadKind == DragPayloadKind::MaterialAsset;
    case component_meta::AssetReferenceKind::Texture:
        return m_dragPayloadKind == DragPayloadKind::TextureAsset;
    case component_meta::AssetReferenceKind::Data:
        return m_dragPayloadKind == DragPayloadKind::DataAsset;
    case component_meta::AssetReferenceKind::SceneScript:
        return m_dragPayloadKind == DragPayloadKind::SceneScriptAsset;
    case component_meta::AssetReferenceKind::ComponentScript:
        return m_dragPayloadKind == DragPayloadKind::ComponentScriptAsset;
    case component_meta::AssetReferenceKind::Scene:
        return m_dragPayloadKind == DragPayloadKind::AssetFile;
    case component_meta::AssetReferenceKind::Generic:
        return m_dragPayloadKind != DragPayloadKind::None;
    case component_meta::AssetReferenceKind::None:
    default:
        return false;
    }
}

bool SceneEditorController::applyDraggedAssetToInspectorField(const InspectorFieldBinding& binding)
{
    if (!canDropDraggedAssetOnInspectorField(binding))
        return false;

    return applyInspectorFieldValue(binding, m_draggedAssetRuntimePath);
}

void SceneEditorController::toggleInspectorGroup(const std::string& groupId)
{
    const auto it = m_collapsedInspectorGroups.find(groupId);
    if (it != m_collapsedInspectorGroups.end())
        m_collapsedInspectorGroups.erase(it);
    else
        m_collapsedInspectorGroups.insert(groupId);
}

bool SceneEditorController::isInspectorGroupCollapsed(const std::string& groupId) const
{
    return m_collapsedInspectorGroups.count(groupId) > 0;
}

void SceneEditorController::markSceneDirty(bool requestFullRuntimeSync)
{
    if (requestFullRuntimeSync)
        m_runtimeSceneSyncPending = true;
    if (!m_sceneDirty)
    {
        m_sceneDirty = true;
        requestHierarchyRefresh();
    }
}

void SceneEditorController::queueRuntimeGameObjectSync(int nodeId)
{
    if (nodeId <= 0)
        return;

    m_runtimeGameObjectSyncId = nodeId;
}

void SceneEditorController::clearSceneDirty()
{
    if (m_sceneDirty)
    {
        m_sceneDirty = false;
        requestHierarchyRefresh();
    }
}

void SceneEditorController::closeHeaderMenus()
{
    m_isFileMenuOpen = false;
    m_isEditMenuOpen = false;
    m_isWindowMenuOpen = false;
}

bool SceneEditorController::prepareRuntimeSceneFile(std::string& outputPath)
{
    if (m_scene == nullptr)
        return false;

    if (!flushDirtyMaterialAssetsForRuntimeScene())
        return false;

    std::error_code errorCode;
    const std::filesystem::path sessionDirectory = runtime_preview::sessionDirectory();
    std::filesystem::create_directories(sessionDirectory, errorCode);
    if (errorCode)
        return false;

    std::filesystem::remove(runtime_preview::frameMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::frameDataPath(), errorCode);
    std::filesystem::remove(runtime_preview::frameMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::frameDataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::resizeMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::resizeMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::inputMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::inputMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::clickMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::clickMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::selectionMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::selectionMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::sceneSyncMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::sceneSyncMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::objectPatchPath(), errorCode);
    std::filesystem::remove(runtime_preview::objectPatchTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::objectStatePath(), errorCode);
    std::filesystem::remove(runtime_preview::objectStateTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::pauseMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::pauseMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::stateMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::stateMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::materialMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::materialMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::materialStatePath(), errorCode);
    std::filesystem::remove(runtime_preview::materialStateTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::dataAssetMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::dataAssetMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::dataAssetStatePath(), errorCode);
    std::filesystem::remove(runtime_preview::dataAssetStateTempPath(), errorCode);

    const std::filesystem::path scenePath = runtime_preview::previewScenePath();
    if (!scene_serialization::saveSceneToFile(*m_scene, scenePath.string()))
        return false;

    m_runtimeSceneSyncPending = false;
    m_lastSentRuntimeSelectionId.reset();
    outputPath = scenePath.string();
    return true;
}

bool SceneEditorController::flushDirtyMaterialAssetsForRuntimeScene()
{
    if (m_scene == nullptr)
        return true;

    // Clear any pending dirty flags first, then persist the current in-memory material state
    // for every referenced material so Play starts from the live editor values.
    while (true)
    {
        std::string materialAssetPath;
        asset::MaterialAssetDefinition definition;
        if (!asset::AssetManager::instance().popDirtyMaterialState(materialAssetPath, definition) || materialAssetPath.empty())
            break;
    }

    std::vector<std::string> materialPaths = m_scene->getSceneAssetRegistry().getAssets(asset::AssetType::Material);
    std::unordered_set<std::string> seenMaterialPaths(materialPaths.begin(), materialPaths.end());

    for (size_t index = 0; index < m_scene->getGameObjectCount(); ++index)
    {
        GameObject* gameObject = m_scene->getGameObject(index);
        if (gameObject == nullptr)
            continue;

        MeshRenderer* meshRenderer = gameObject->getComponent<MeshRenderer>();
        if (meshRenderer == nullptr)
            continue;

        const std::string normalizedMaterialPath = asset::AssetManager::normalizeRelativePath(meshRenderer->getMaterialAssetPath());
        if (normalizedMaterialPath.empty() || !seenMaterialPaths.insert(normalizedMaterialPath).second)
            continue;

        materialPaths.push_back(normalizedMaterialPath);
    }

    for (const std::string& rawMaterialPath : materialPaths)
    {
        const std::string normalizedMaterialPath = asset::AssetManager::normalizeRelativePath(rawMaterialPath);
        if (normalizedMaterialPath.empty())
            continue;

        dataStruct::Material* material = m_scene->resolveMaterialAsset(normalizedMaterialPath);
        if (material == nullptr)
        {
            appendConsoleSystemMessage("[play] Failed to resolve material before preview launch: " + normalizedMaterialPath, "console_line_error");
            return false;
        }

        const std::string diskPath = asset::AssetManager::runtimePath(normalizedMaterialPath);
        if (!asset::MaterialAssetIO::saveDefinition(diskPath, material->getRuntimeDefinition()))
        {
            appendConsoleSystemMessage("[play] Failed to persist current material state before preview launch: " + normalizedMaterialPath, "console_line_error");
            return false;
        }
    }

    return true;
}

bool SceneEditorController::rebuildSceneRenderPipelines()
{
    if (m_scene == nullptr)
        return false;

    if (!prepareGeneratedGameplaySource())
    {
        requestHierarchyRefresh();
        return false;
    }

    std::vector<std::string> issues;
    if (!m_scene->buildRenderPipelines(&issues))
    {
        if (issues.empty())
            issues.push_back("Render pipeline rebuild failed.");
        for (const std::string& issue : issues)
            appendConsoleSystemMessage("[render] " + issue, "console_line_error");
        requestHierarchyRefresh();
        return false;
    }

    appendConsoleSystemMessage("[render] Render pipeline assets rebuilt.", "console_line_success");
    requestHierarchyRefresh();
    return true;
}

bool SceneEditorController::bakeSceneRenderTargets()
{
    if (m_scene == nullptr)
        return false;

    std::vector<render::RenderPipeline::BakedRenderTargetResult> bakedTargets;
    if (!m_scene->bakeRenderTargets(bakedTargets) || bakedTargets.empty())
    {
        appendConsoleSystemMessage("[bake] No bakeable render targets were produced.", "console_line_warning");
        return false;
    }

    bool wroteAnyTexture = false;
    bool sceneSettingsChanged = false;
    std::unordered_set<std::string> updatedTargetNames;

    for (const render::RenderPipeline::BakedRenderTargetResult& bakedTarget : bakedTargets)
    {
        const std::string bakedExtension = bakedTarget.format == render::RenderTargetFormat::Float ? ".rttex" : ".tga";
        const std::string baseAssetPath = defaultBakedTextureBaseAssetPath(m_currentSceneFilePath, bakedTarget.targetName, bakedExtension);
        const std::string outputAssetPath = applyBakedTextureGroupSuffix(baseAssetPath, bakedTarget.targetGroupSuffix);
        const std::string diskPath = asset::AssetManager::runtimePath(outputAssetPath);

        const bool saved =
            bakedTarget.format == render::RenderTargetFormat::Float
                ? asset::TextureAssetIO::saveRFloatTexture(diskPath, bakedTarget.width, bakedTarget.height, bakedTarget.pixelsFloat)
                : asset::TextureAssetIO::saveRgba8Tga(diskPath, bakedTarget.width, bakedTarget.height, bakedTarget.pixelsRgba8);
        if (!saved)
        {
            appendConsoleSystemMessage("[bake] Failed to save baked texture asset: " + outputAssetPath, "console_line_error");
            continue;
        }

        wroteAnyTexture = true;
        appendConsoleSystemMessage(
            "[bake] Saved " + bakedTarget.targetName + bakedTarget.targetGroupSuffix + " -> " + outputAssetPath,
            "console_line_success");

        if (!updatedTargetNames.insert(bakedTarget.targetName).second)
            continue;

        render::SceneRenderTargetSettings settings = m_scene->resolveRenderTargetSettings(bakedTarget.targetName);
        if (settings.bakedTextureAssetPath == baseAssetPath)
            continue;

        settings.bakedTextureAssetPath = baseAssetPath;
        sceneSettingsChanged = m_scene->upsertRenderTargetSettings(settings) || sceneSettingsChanged;
    }

    if (!wroteAnyTexture)
        return false;

    if (sceneSettingsChanged)
        markSceneDirty(true);

    refreshInspectorValuesPresentation();
    return true;
}

bool SceneEditorController::startBuild(PendingLaunchAction launchAction)
{
    m_bottomPanelTab = BottomPanelTab::Console;

    if (launchAction != PendingLaunchAction::None && m_scene != nullptr)
    {
        std::vector<std::string> issues;
        if (!m_scene->validateRenderPipelines(&issues))
        {
            appendConsoleSystemMessage("[render] Preview launch blocked: rebuild render pipelines first.", "console_line_error");
            for (const std::string& issue : issues)
                appendConsoleSystemMessage("[render] " + issue, "console_line_error");
            requestHierarchyRefresh();
            return false;
        }
    }

    if (launchAction == PendingLaunchAction::PlayPreview && m_scene != nullptr && !m_runtimeSceneSnapshot.has_value())
        m_runtimeSceneSnapshot = scene_serialization::captureScene(*m_scene);

    if (!prepareGeneratedGameplaySource())
    {
        requestHierarchyRefresh();
        return false;
    }

    stopExternalProcess(false);

    const std::filesystem::path buildDirectory = runtime_preview::runtimeRoot();
    if (!std::filesystem::exists(buildDirectory))
    {
        appendConsoleSystemMessage("[editor] Build directory not found: " + buildDirectory.string(), "console_line_error");
        requestHierarchyRefresh();
        return false;
    }

#ifdef _WIN32
    appendConsoleSystemMessage("[editor] External runtime build is not implemented on Windows yet.", "console_line_warning");
    requestHierarchyRefresh();
    return false;
#else
    const std::string command =
        std::string("cmake --build ") + shellQuote(buildDirectory.string()) +
        " --target runtime_game --parallel 4";

    int childPid = -1;
    int outputFd = -1;
    if (!spawnShellProcess(buildDirectory.parent_path().string(), command, childPid, outputFd))
    {
        appendConsoleSystemMessage("[editor] Failed to start build process.", "console_line_error");
        requestHierarchyRefresh();
        return false;
    }

    m_activeProcessPid = childPid;
    m_activeProcessOutputFd = outputFd;
    m_activeProcessKind = ActiveProcessKind::Build;
    m_pendingLaunchAction = launchAction;
    m_pendingLaunchScenePath.clear();
    m_playbackState = PlaybackState::Stopped;
    appendConsoleSystemMessage("[build] Building runtime_game...", "console_line_info");
    refreshViewportPresentation();
    requestHierarchyRefresh();
    return true;
#endif
}

bool SceneEditorController::startPreviewPlayer(const std::string& scenePath)
{
    if (m_scene != nullptr)
    {
        std::vector<std::string> issues;
        if (!m_scene->validateRenderPipelines(&issues))
        {
            appendConsoleSystemMessage("[render] Preview launch blocked: render pipelines are stale or invalid.", "console_line_error");
            for (const std::string& issue : issues)
                appendConsoleSystemMessage("[render] " + issue, "console_line_error");
            return false;
        }
    }

#ifdef _WIN32
    (void)scenePath;
    appendConsoleSystemMessage("[editor] External preview player is not implemented on Windows yet.", "console_line_warning");
    return false;
#else
    const std::filesystem::path runtimeRoot = runtime_preview::runtimeRoot();
    const std::filesystem::path playerPath = runtimeRoot / "runtime_game";
    if (!std::filesystem::exists(playerPath))
    {
        appendConsoleSystemMessage("[editor] runtime_game executable not found: " + playerPath.string(), "console_line_error");
        return false;
    }

    const UiRect viewportRect = m_layoutManager.viewportRect();
    const int previewWidth = std::max(viewportRect.width, 1);
    const int previewHeight = std::max(viewportRect.height, 1);

    int childPid = -1;
    int outputFd = -1;
    if (!spawnCapturedProcess(
            runtimeRoot.string(),
            playerPath,
            {"--scene", scenePath, "--preview-width", std::to_string(previewWidth), "--preview-height", std::to_string(previewHeight), "--hidden-preview"},
            childPid,
            outputFd))
    {
        appendConsoleSystemMessage("[editor] Failed to launch runtime_game.", "console_line_error");
        return false;
    }

    m_activeProcessPid = childPid;
    m_activeProcessOutputFd = outputFd;
    m_activeProcessKind = ActiveProcessKind::Player;
    m_playbackState = PlaybackState::Playing;
    m_runtimePreviewFps = -1;
    m_runtimeStateSequence = 0;
    m_runtimeDataAssetSyncSequence = 0;
    m_runtimeDataAssetStateSequence = 0;
    m_runtimeMaterialStateSequence = 0;
    m_runtimeSelectionSyncSequence = 0;
    m_lastPlaybackStatusText.clear();
    m_lastSentRuntimeSelectionId.reset();
    appendConsoleSystemMessage("[play] runtime_game started.", "console_line_success");
    refreshViewportPresentation();
    requestHierarchyRefresh();
    return true;
#endif
}

bool SceneEditorController::startDetachedPlayer(const std::string& scenePath)
{
#ifdef _WIN32
    (void)scenePath;
    appendConsoleSystemMessage("[editor] Detached runtime launch is not implemented on Windows yet.", "console_line_warning");
    return false;
#else
    const std::filesystem::path runtimeRoot = runtime_preview::runtimeRoot();
    const std::filesystem::path playerPath = runtimeRoot / "runtime_game";
    if (!std::filesystem::exists(playerPath))
    {
        appendConsoleSystemMessage("[editor] runtime_game executable not found: " + playerPath.string(), "console_line_error");
        return false;
    }

    if (!spawnDetachedProcess(runtimeRoot.string(), playerPath, {"--scene", scenePath}))
    {
        appendConsoleSystemMessage("[editor] Failed to launch detached runtime_game.", "console_line_error");
        return false;
    }

    appendConsoleSystemMessage("[run] runtime_game launched in a detached process.", "console_line_success");
    requestHierarchyRefresh();
    return true;
#endif
}

void SceneEditorController::stopExternalProcess(bool restoreEditorScene)
{
    const ActiveProcessKind stoppedKind = m_activeProcessKind;
#ifndef _WIN32
    if (m_activeProcessPid > 0)
    {
        kill(static_cast<pid_t>(m_activeProcessPid), SIGTERM);

        int status = 0;
        for (int attempt = 0; attempt < 10; ++attempt)
        {
            const pid_t result = waitpid(static_cast<pid_t>(m_activeProcessPid), &status, WNOHANG);
            if (result == static_cast<pid_t>(m_activeProcessPid))
                break;
            usleep(20000);
        }

        if (waitpid(static_cast<pid_t>(m_activeProcessPid), &status, WNOHANG) == 0)
        {
            kill(static_cast<pid_t>(m_activeProcessPid), SIGKILL);
            waitpid(static_cast<pid_t>(m_activeProcessPid), &status, 0);
        }
    }
#endif

    if (m_activeProcessOutputFd >= 0)
    {
        if (!m_consolePartialLine.empty())
        {
            appendConsoleLine(m_consolePartialLine);
            m_consolePartialLine.clear();
        }
#ifndef _WIN32
        close(m_activeProcessOutputFd);
#endif
    }

    if (m_activeProcessKind != ActiveProcessKind::None)
        appendConsoleSystemMessage("[editor] External process stopped.", "console_line_warning");

    m_activeProcessPid = -1;
    m_activeProcessOutputFd = -1;
    m_activeProcessKind = ActiveProcessKind::None;
    m_pendingLaunchAction = PendingLaunchAction::None;
    m_pendingLaunchScenePath.clear();
    m_playbackState = PlaybackState::Stopped;
    m_runtimePreviewFps = -1;
    m_runtimeStateSequence = 0;
    m_runtimeDataAssetStateSequence = 0;
    m_runtimeMaterialStateSequence = 0;
    m_runtimeSelectionSyncSequence = 0;
    m_lastPlaybackStatusText.clear();
    m_runtimeGameObjectSyncId = -1;
    m_runtimePauseSequence = 0;
    m_runtimeDataAssetSyncSequence = 0;
    m_runtimeSceneSyncPending = false;
    m_runtimeSceneStateSequence = 0;
    m_lastSentRuntimeSelectionId.reset();

    std::error_code errorCode;
    std::filesystem::remove(runtime_preview::frameMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::frameDataPath(), errorCode);
    std::filesystem::remove(runtime_preview::frameMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::frameDataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::resizeMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::resizeMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::inputMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::inputMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::clickMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::clickMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::selectionMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::selectionMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::sceneSyncMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::sceneSyncMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::sceneStatePath(), errorCode);
    std::filesystem::remove(runtime_preview::sceneStateTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::sceneStateMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::sceneStateMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::objectPatchPath(), errorCode);
    std::filesystem::remove(runtime_preview::objectPatchTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::objectStatePath(), errorCode);
    std::filesystem::remove(runtime_preview::objectStateTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::pauseMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::pauseMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::stateMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::stateMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::materialMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::materialMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::materialStatePath(), errorCode);
    std::filesystem::remove(runtime_preview::materialStateTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::dataAssetMetadataPath(), errorCode);
    std::filesystem::remove(runtime_preview::dataAssetMetadataTempPath(), errorCode);
    std::filesystem::remove(runtime_preview::dataAssetStatePath(), errorCode);
    std::filesystem::remove(runtime_preview::dataAssetStateTempPath(), errorCode);

    if (restoreEditorScene && stoppedKind == ActiveProcessKind::Player && m_scene != nullptr && m_runtimeSceneSnapshot.has_value())
    {
        if (scene_serialization::applySceneSnapshot(*m_scene, *m_runtimeSceneSnapshot))
        {
            m_scene->setPhysicsSimulationEnabled(false);
            clearSceneDirty();
            sync(*m_scene);
            requestHierarchyRefresh();
        }
    }

    if (stoppedKind == ActiveProcessKind::Player)
    {
        applyPendingExternalDataAssetReloads();
        m_runtimeSceneSnapshot.reset();
    }

    refreshViewportPresentation();
}

void SceneEditorController::pollDataAssetExternalChanges()
{
    if (m_scene == nullptr)
        return;

    const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(m_scene->getDataAssetPath());
    if (normalizedPath.empty())
        return;

    const std::string diskPath = asset::AssetManager::runtimePath(normalizedPath);
    const std::filesystem::file_time_type currentWriteTime = safeLastWriteTimeLocal(diskPath);
    if (currentWriteTime == std::filesystem::file_time_type::min())
        return;

    auto observedIt = m_observedDataAssetWriteTimes.find(normalizedPath);
    if (observedIt == m_observedDataAssetWriteTimes.end())
    {
        m_observedDataAssetWriteTimes[normalizedPath] = currentWriteTime;
        return;
    }

    if (observedIt->second == currentWriteTime)
        return;

    observedIt->second = currentWriteTime;
    if (m_activeProcessKind == ActiveProcessKind::Player)
    {
        m_pendingExternalDataAssetReloadPaths.insert(normalizedPath);
        return;
    }

    asset::DataAssetDefinition* definition = nullptr;
    if (!asset::AssetManager::instance().reloadDataAssetDefinition(normalizedPath, definition))
    {
        appendConsoleSystemMessage("[asset] Failed to reload external data asset: " + normalizedPath, "console_line_error");
        return;
    }

    InspectorFieldBinding dataAssetBinding;
    dataAssetBinding.target = InspectorFieldBinding::Target::Scene;
    dataAssetBinding.nodeId = 0;
    dataAssetBinding.fieldKey = "dataAsset";
    refreshDataAssetEditorPresentation(dataAssetBinding);
}

void SceneEditorController::applyPendingExternalDataAssetReloads()
{
    if (m_pendingExternalDataAssetReloadPaths.empty())
        return;

    bool refreshedCurrentSceneDataAsset = false;
    const std::string currentSceneDataAsset = m_scene != nullptr
        ? asset::AssetManager::normalizeRelativePath(m_scene->getDataAssetPath())
        : std::string();

    for (const std::string& assetPath : m_pendingExternalDataAssetReloadPaths)
    {
        asset::DataAssetDefinition* definition = nullptr;
        if (!asset::AssetManager::instance().reloadDataAssetDefinition(assetPath, definition))
        {
            appendConsoleSystemMessage("[asset] Failed to apply deferred data asset reload: " + assetPath, "console_line_error");
            continue;
        }

        if (assetPath == currentSceneDataAsset)
            refreshedCurrentSceneDataAsset = true;
    }

    m_pendingExternalDataAssetReloadPaths.clear();

    if (!refreshedCurrentSceneDataAsset)
        return;

    InspectorFieldBinding dataAssetBinding;
    dataAssetBinding.target = InspectorFieldBinding::Target::Scene;
    dataAssetBinding.nodeId = 0;
    dataAssetBinding.fieldKey = "dataAsset";
    refreshDataAssetEditorPresentation(dataAssetBinding);
}

void SceneEditorController::syncRuntimePreviewSceneIfNeeded()
{
    if (m_scene == nullptr || !isExternalPreviewActive() || !m_runtimeSceneSyncPending)
        return;

    if (!flushDirtyMaterialAssetsForRuntimeScene())
        return;

    const std::filesystem::path scenePath = runtime_preview::previewScenePath();
    if (!scene_serialization::saveSceneToFile(*m_scene, scenePath.string()))
        return;

    if (!writeRuntimePreviewSequenceFile(
            runtime_preview::sceneSyncMetadataTempPath(),
            runtime_preview::sceneSyncMetadataPath(),
            m_runtimeSceneSyncSequence + 1))
    {
        return;
    }

    ++m_runtimeSceneSyncSequence;
    m_runtimeSceneSyncPending = false;
}

void SceneEditorController::syncRuntimePreviewSelectionIfNeeded()
{
    if (!isExternalPreviewActive() || !m_pendingRuntimeSelectionId.has_value())
        return;

    if (m_lastSentRuntimeSelectionId.has_value() && *m_lastSentRuntimeSelectionId == *m_pendingRuntimeSelectionId)
        return;

    if (!writeRuntimePreviewSelectionFile(
            runtime_preview::selectionMetadataTempPath(),
            runtime_preview::selectionMetadataPath(),
            m_runtimeSelectionSyncSequence + 1,
            *m_pendingRuntimeSelectionId))
    {
        return;
    }

    ++m_runtimeSelectionSyncSequence;
    m_lastSentRuntimeSelectionId = *m_pendingRuntimeSelectionId;
}

void SceneEditorController::syncRuntimePreviewGameObjectIfNeeded()
{
    if (m_scene == nullptr || !isExternalPreviewActive() || m_runtimeSceneSyncPending || m_runtimeGameObjectSyncId <= 0)
        return;

    GameObject* gameObject = m_scene->getGameObjectById(m_runtimeGameObjectSyncId);
    if (gameObject == nullptr)
    {
        m_runtimeGameObjectSyncId = -1;
        return;
    }

    const scene_serialization::GameObjectSnapshot snapshot = scene_serialization::captureGameObject(*gameObject);
    if (!writeRuntimePreviewGameObjectFile(
            runtime_preview::objectPatchTempPath(),
            runtime_preview::objectPatchPath(),
            m_runtimeGameObjectSyncSequence + 1,
            snapshot))
    {
        return;
    }

    ++m_runtimeGameObjectSyncSequence;
    m_runtimeGameObjectSyncId = -1;
}

void SceneEditorController::pollRuntimePreviewState()
{
    if (m_activeProcessKind != ActiveProcessKind::Player)
    {
        m_pendingRuntimeSelectionId.reset();
        m_runtimePreviewFps = -1;
        return;
    }

    std::ifstream input(runtime_preview::stateMetadataPath());
    if (!input)
        return;

    uint64_t nextSequence = 0;
    int selectedGameObjectId = -1;
    int captureEnabled = 0;
    int fps = -1;
    if (!(input >> nextSequence >> selectedGameObjectId >> captureEnabled >> fps) || nextSequence <= m_runtimeStateSequence)
        return;

    m_runtimeStateSequence = nextSequence;
    m_runtimePreviewFps = std::clamp(fps, 0, 60);
    (void)captureEnabled;

    if (m_scene == nullptr)
        return;

    if (m_pendingRuntimeSelectionId.has_value())
    {
        if (selectedGameObjectId != *m_pendingRuntimeSelectionId)
            return;

        m_pendingRuntimeSelectionId.reset();
        m_lastSentRuntimeSelectionId.reset();
    }

    const GameObject* selectedGameObject = m_scene->getSelectedGameObject();
    const int currentSelectedId = selectedGameObject != nullptr ? selectedGameObject->getId() : -1;
    if (currentSelectedId == selectedGameObjectId)
        return;

    m_scene->setSelectedGameObjectById(selectedGameObjectId);
    sync(*m_scene);
    requestSelectionRefresh();
}

void SceneEditorController::pollRuntimePreviewSceneState()
{
    if (m_activeProcessKind != ActiveProcessKind::Player || m_scene == nullptr)
        return;

    std::ifstream input(runtime_preview::sceneStateMetadataPath());
    if (!input)
        return;

    uint64_t nextSequence = 0;
    if (!(input >> nextSequence) || nextSequence <= m_runtimeSceneStateSequence)
        return;

    std::ifstream sceneInput(runtime_preview::sceneStatePath());
    if (!sceneInput)
        return;

    scene_serialization::SceneSnapshot snapshot;
    if (!scene_serialization::detail::loadSnapshotFromStream(sceneInput, snapshot))
        return;

    const int localSelectedId = m_scene->getSelectedGameObject() != nullptr ? m_scene->getSelectedGameObject()->getId() : -1;

    if (!scene_serialization::mergeSceneSnapshot(*m_scene, snapshot))
        return;

    if (m_pendingRuntimeSelectionId.has_value())
        m_scene->setSelectedGameObjectById(localSelectedId);

    clearSceneDirty();
    sync(*m_scene);
    m_runtimeSceneStateSequence = nextSequence;
}

void SceneEditorController::updatePlaybackStatusPresentation()
{
    if (m_playbackStatusElement == nullptr)
        return;

    const std::string nextText = buildPlaybackStatusText();
    if (nextText == m_lastPlaybackStatusText)
        return;

    m_playbackStatusElement->SetInnerRML(escapeRmlText(nextText));
    m_lastPlaybackStatusText = nextText;
}

std::string SceneEditorController::buildPlaybackStatusText() const
{
    if (m_activeProcessKind == ActiveProcessKind::Player && m_playbackState == PlaybackState::Playing && m_runtimePreviewFps >= 0)
        return std::to_string(m_runtimePreviewFps) + "/60 fps";

    return "--/60 fps";
}

void SceneEditorController::pausePreviewPlayer()
{
    if (m_activeProcessKind == ActiveProcessKind::Player && m_activeProcessPid > 0 && m_playbackState == PlaybackState::Playing)
    {
        if (!writeRuntimePreviewPauseStateFile(
                runtime_preview::pauseMetadataTempPath(),
                runtime_preview::pauseMetadataPath(),
                m_runtimePauseSequence + 1,
                true))
        {
            appendConsoleSystemMessage("[play] Failed to pause player simulation.", "console_line_error");
            return;
        }

        ++m_runtimePauseSequence;
        m_playbackState = PlaybackState::Paused;
        appendConsoleSystemMessage("[play] Player paused.", "console_line_info");
        refreshViewportPresentation();
    }
}

void SceneEditorController::resumePreviewPlayer()
{
    if (m_activeProcessKind == ActiveProcessKind::Player && m_activeProcessPid > 0 && m_playbackState == PlaybackState::Paused)
    {
        if (!writeRuntimePreviewPauseStateFile(
                runtime_preview::pauseMetadataTempPath(),
                runtime_preview::pauseMetadataPath(),
                m_runtimePauseSequence + 1,
                false))
        {
            appendConsoleSystemMessage("[play] Failed to resume player simulation.", "console_line_error");
            return;
        }

        ++m_runtimePauseSequence;
        m_playbackState = PlaybackState::Playing;
        appendConsoleSystemMessage("[play] Player resumed.", "console_line_info");
        refreshViewportPresentation();
    }
}

void SceneEditorController::pollExternalProcess()
{
#ifndef _WIN32
    if (m_activeProcessOutputFd >= 0)
    {
        char buffer[4096] = {};
        while (true)
        {
            const ssize_t readCount = read(m_activeProcessOutputFd, buffer, sizeof(buffer));
            if (readCount > 0)
            {
                appendConsoleOutput(std::string(buffer, static_cast<size_t>(readCount)));
                continue;
            }

            if (readCount == 0 || (readCount < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
                break;

            break;
        }
    }

    if (m_activeProcessPid <= 0)
        return;

    int status = 0;
    const pid_t waitResult = waitpid(static_cast<pid_t>(m_activeProcessPid), &status, WNOHANG);
    if (waitResult != static_cast<pid_t>(m_activeProcessPid))
        return;

    if (m_activeProcessOutputFd >= 0)
    {
        char buffer[4096] = {};
        ssize_t readCount = 0;
        while ((readCount = read(m_activeProcessOutputFd, buffer, sizeof(buffer))) > 0)
            appendConsoleOutput(std::string(buffer, static_cast<size_t>(readCount)));
        close(m_activeProcessOutputFd);
    }

    const ActiveProcessKind completedKind = m_activeProcessKind;
    const PendingLaunchAction launchAction = m_pendingLaunchAction;
    const bool succeeded = WIFEXITED(status) && WEXITSTATUS(status) == 0;

    m_activeProcessPid = -1;
    m_activeProcessOutputFd = -1;
    m_activeProcessKind = ActiveProcessKind::None;
    m_pendingLaunchAction = PendingLaunchAction::None;
    m_pendingLaunchScenePath.clear();

    if (completedKind == ActiveProcessKind::Build)
    {
        appendConsoleSystemMessage(
            succeeded ? "[build] runtime_game build completed." : "[build] runtime_game build failed.",
            succeeded ? "console_line_success" : "console_line_error");

        if (succeeded)
        {
            std::string launchScenePath;
            if (launchAction != PendingLaunchAction::None && !prepareRuntimeSceneFile(launchScenePath))
            {
                appendConsoleSystemMessage("[editor] Failed to prepare runtime scene snapshot after build.", "console_line_error");
                m_playbackState = PlaybackState::Stopped;
                refreshViewportPresentation();
                requestHierarchyRefresh();
                return;
            }

            if (launchAction == PendingLaunchAction::PlayPreview)
            {
                if (!startPreviewPlayer(launchScenePath))
                    m_playbackState = PlaybackState::Stopped;
            }
            else if (launchAction == PendingLaunchAction::RunDetached)
            {
                startDetachedPlayer(launchScenePath);
                m_playbackState = PlaybackState::Stopped;
            }
        }
        else
        {
            m_playbackState = PlaybackState::Stopped;
        }

        refreshViewportPresentation();
        requestHierarchyRefresh();
        return;
    }

    if (completedKind == ActiveProcessKind::Player)
    {
        m_playbackState = PlaybackState::Stopped;
        appendConsoleSystemMessage(
            succeeded ? "[play] Player exited normally." : "[play] Player exited with an error.",
            succeeded ? "console_line_info" : "console_line_error");
        refreshViewportPresentation();
        requestHierarchyRefresh();
    }
#endif
}

void SceneEditorController::appendConsoleOutput(const std::string& text, const std::string& sourceClass)
{
    std::string buffer = m_consolePartialLine + text;
    m_consolePartialLine.clear();

    size_t lineStart = 0;
    while (lineStart < buffer.size())
    {
        const size_t lineEnd = buffer.find('\n', lineStart);
        if (lineEnd == std::string::npos)
        {
            m_consolePartialLine = buffer.substr(lineStart);
            break;
        }

        std::string line = buffer.substr(lineStart, lineEnd - lineStart);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        appendConsoleLine(line, sourceClass);
        lineStart = lineEnd + 1;
    }
}

void SceneEditorController::appendConsoleLine(const std::string& line, const std::string& sourceClass)
{
    const std::string lineClass = classifyConsoleLine(line, sourceClass);
    m_consoleLines.push_back(renderConsoleLineMarkup(line, lineClass));
    if (m_consoleLines.size() > MaxConsoleLines)
        m_consoleLines.erase(m_consoleLines.begin(), m_consoleLines.begin() + static_cast<long>(m_consoleLines.size() - MaxConsoleLines));
    m_consoleRefreshPending = true;
}

void SceneEditorController::appendConsoleSystemMessage(const std::string& message, const std::string& sourceClass)
{
    appendConsoleLine(message, sourceClass);
}

void SceneEditorController::clearConsole()
{
    m_consoleLines.clear();
    m_consolePartialLine.clear();
    m_consoleRefreshPending = true;
}

bool SceneEditorController::saveSceneAs()
{
    if (m_scene == nullptr)
        return false;

    const std::filesystem::path defaultPath = m_currentSceneFilePath.empty()
        ? (std::filesystem::current_path() / "scene.scene")
        : std::filesystem::path(m_currentSceneFilePath);

    const std::optional<std::string> selectedPath = platform::showNativeFileDialog(
        platform::FileDialogMode::SaveFile,
        "Save Scene As",
        defaultPath.string(),
        buildSceneFileDialogFilters());
    if (!selectedPath.has_value())
        return false;

    if (!scene_serialization::saveSceneToFile(*m_scene, *selectedPath))
        return false;

    m_currentSceneFilePath = *selectedPath;
    clearSceneDirty();
    return true;
}

bool SceneEditorController::prepareGeneratedGameplaySource()
{
    const std::filesystem::path buildRoot = runtime_preview::runtimeRoot();
    const std::filesystem::path generatedSourcePath = buildRoot / "GeneratedSceneScripts.cpp";

    std::error_code errorCode;
    std::filesystem::create_directories(generatedSourcePath.parent_path(), errorCode);
    if (errorCode)
    {
        appendConsoleSystemMessage("[build] Failed to prepare scene script source directory.", "console_line_error");
        return false;
    }

    std::ofstream output(generatedSourcePath, std::ios::trunc);
    if (!output)
    {
        appendConsoleSystemMessage("[build] Failed to write GeneratedSceneScripts.cpp.", "console_line_error");
        return false;
    }

    output << "#include \"GameplayEntry.hpp\"\n\n";

    std::vector<std::string> includePaths;
    std::vector<std::string> sceneScriptRegistrationLines;
    std::vector<std::string> renderFactoryRegistrationLines;
    std::unordered_set<std::string> includedPaths;

    const auto resolveGeneratedIncludePath = [&](const std::string& assetPath, const std::string& sourcePath, const std::string& assetLabel) -> std::optional<std::string> {
        const std::string normalizedAssetPath = normalizeSceneScriptSourcePath(assetPath);
        const std::string normalizedSourcePath = normalizeSceneScriptSourcePath(sourcePath);
        if (normalizedSourcePath.empty())
        {
            appendConsoleSystemMessage("[build] " + assetLabel + " source path is empty: " + assetPath, "console_line_error");
            return std::nullopt;
        }

        const std::filesystem::path assetDirectory = std::filesystem::path(normalizedAssetPath).parent_path();
        const std::vector<std::filesystem::path> candidates = {
            buildRoot / normalizedSourcePath,
            buildRoot / assetDirectory / normalizedSourcePath,
        };

        std::filesystem::path resolvedSourcePath;
        for (const std::filesystem::path& candidate : candidates)
        {
            std::error_code candidateError;
            if (std::filesystem::exists(candidate, candidateError) && !candidateError)
            {
                resolvedSourcePath = candidate;
                break;
            }
        }

        if (resolvedSourcePath.empty())
        {
            appendConsoleSystemMessage(
                "[build] " + assetLabel + " source file not found for asset: " + assetPath + " (source=" + normalizedSourcePath + ")",
                "console_line_error");
            return std::nullopt;
        }

        std::error_code relativeError;
        std::filesystem::path relativePath = std::filesystem::relative(resolvedSourcePath, buildRoot, relativeError);
        if (relativeError)
            relativePath = resolvedSourcePath.filename();

        return relativePath.generic_string();
    };

    const auto registerIncludePath = [&](const std::string& includePath) {
        if (includedPaths.insert(includePath).second)
            includePaths.push_back(includePath);
    };

    const std::string sceneScriptAssetPath = m_scene != nullptr
        ? asset::AssetManager::normalizeRelativePath(m_scene->getSceneScriptAssetPath())
        : std::string();
    if (!sceneScriptAssetPath.empty())
    {
        asset::SceneScriptAssetDefinition* definition = asset::AssetManager::instance().loadSceneScriptAssetDefinition(sceneScriptAssetPath);
        if (definition == nullptr)
        {
            appendConsoleSystemMessage("[build] Failed to load scene script asset: " + sceneScriptAssetPath, "console_line_error");
            return false;
        }

        const std::optional<std::string> includePath = resolveGeneratedIncludePath(sceneScriptAssetPath, definition->sourcePath, "Scene script");
        if (!includePath.has_value())
            return false;

        registerIncludePath(*includePath);
        sceneScriptRegistrationLines.push_back(
            "    registerSceneScript(\"" + escapeCppStringLiteral(sceneScriptAssetPath) + "\", &" + definition->entryName + ");");
    }

    if (m_scene != nullptr)
    {
        std::vector<std::string> componentScriptAssetPaths;
        std::unordered_set<std::string> seenComponentScriptAssets;
        for (size_t gameObjectIndex = 0; gameObjectIndex < m_scene->getGameObjectCount(); ++gameObjectIndex)
        {
            const GameObject* gameObject = m_scene->getGameObject(gameObjectIndex);
            if (gameObject == nullptr)
                continue;

            for (size_t componentIndex = 0; componentIndex < gameObject->getComponentCount(); ++componentIndex)
            {
                const auto* scriptComponent = dynamic_cast<const component::ScriptComponent*>(gameObject->getComponentAt(componentIndex));
                if (scriptComponent == nullptr)
                    continue;

                const std::string componentScriptAssetPath = asset::AssetManager::normalizeRelativePath(scriptComponent->getScriptAssetPath());
                if (componentScriptAssetPath.empty() || !seenComponentScriptAssets.insert(componentScriptAssetPath).second)
                    continue;

                componentScriptAssetPaths.push_back(componentScriptAssetPath);
            }
        }

        std::sort(componentScriptAssetPaths.begin(), componentScriptAssetPaths.end());
        for (const std::string& componentScriptAssetPath : componentScriptAssetPaths)
        {
            asset::ComponentScriptAssetDefinition* definition = asset::AssetManager::instance().loadComponentScriptAssetDefinition(componentScriptAssetPath);
            if (definition == nullptr)
            {
                appendConsoleSystemMessage("[build] Failed to load component script asset: " + componentScriptAssetPath, "console_line_error");
                return false;
            }

            const std::optional<std::string> includePath = resolveGeneratedIncludePath(componentScriptAssetPath, definition->sourcePath, "Component script");
            if (!includePath.has_value())
                return false;

            registerIncludePath(*includePath);
            sceneScriptRegistrationLines.push_back(
                "    registerComponentScript(\"" + escapeCppStringLiteral(componentScriptAssetPath) + "\", &" + definition->startEntryName + ", &" + definition->updateEntryName + ");");
        }
    }

    if (m_scene != nullptr)
    {
        const std::vector<std::string> renderPassAssetPaths = m_scene->collectReferencedRenderPassAssets();
        std::vector<std::string> uniformFactoryAssetPaths;
        std::unordered_set<std::string> seenUniformFactoryAssets;

        for (const std::string& renderPassAssetPath : renderPassAssetPaths)
        {
            asset::RenderPassAssetDefinition* renderPassDefinition = nullptr;
            if (!asset::AssetManager::instance().reloadRenderPassDefinition(renderPassAssetPath, renderPassDefinition) || renderPassDefinition == nullptr)
            {
                appendConsoleSystemMessage("[build] Failed to load render pass asset: " + renderPassAssetPath, "console_line_error");
                return false;
            }

            for (const asset::RenderPassStepDefinition& pass : renderPassDefinition->passes)
            {
                const std::string uniformFactoryAssetPath = asset::AssetManager::normalizeRelativePath(pass.uniformFactoryPath);
                if (uniformFactoryAssetPath.rfind("built-in/uniform_factory/", 0) == 0)
                    continue;
                if (uniformFactoryAssetPath.empty() || !seenUniformFactoryAssets.insert(uniformFactoryAssetPath).second)
                    continue;

                uniformFactoryAssetPaths.push_back(uniformFactoryAssetPath);
            }
        }

        std::sort(uniformFactoryAssetPaths.begin(), uniformFactoryAssetPaths.end());
        for (const std::string& uniformFactoryAssetPath : uniformFactoryAssetPaths)
        {
            asset::UniformFactoryAssetDefinition* definition = asset::AssetManager::instance().loadUniformFactoryDefinition(uniformFactoryAssetPath);
            if (definition == nullptr)
            {
                appendConsoleSystemMessage("[build] Failed to load uniform factory asset: " + uniformFactoryAssetPath, "console_line_error");
                return false;
            }

            const std::optional<std::string> includePath = resolveGeneratedIncludePath(uniformFactoryAssetPath, definition->sourcePath, "Uniform factory");
            if (!includePath.has_value())
                return false;

            registerIncludePath(*includePath);
            const std::string groupEntryExpression = definition->groupEntryName.empty()
                ? "nullptr"
                : "&" + definition->groupEntryName;
            const std::string bakedGroupEntryExpression = definition->bakedGroupEntryName.empty()
                ? "nullptr"
                : "&" + definition->bakedGroupEntryName;
            renderFactoryRegistrationLines.push_back(
                "    registerRenderUniformFactory(\"" + escapeCppStringLiteral(uniformFactoryAssetPath) + "\", &" + definition->entryName + ", &" + definition->iterationEntryName + ", " + groupEntryExpression + ", " + bakedGroupEntryExpression + ");");
        }
    }

    for (const std::string& includePath : includePaths)
        output << "#include \"./" << includePath << "\"\n";

    if (!includePaths.empty())
        output << "\n";

    output << "namespace gameplay\n{\n";
    output << "void registerGeneratedSceneScripts()\n{\n";
    output << "    static bool registered = false;\n";
    output << "    if (registered)\n";
    output << "        return;\n";
    output << "\n";
    output << "    registered = true;\n";
    for (const std::string& registrationLine : sceneScriptRegistrationLines)
        output << registrationLine << '\n';
    output << "}\n";
    output << "\n";
    output << "void registerGeneratedRenderUniformFactories()\n{\n";
    output << "    static bool registered = false;\n";
    output << "    if (registered)\n";
    output << "        return;\n";
    output << "\n";
    output << "    registered = true;\n";
    for (const std::string& registrationLine : renderFactoryRegistrationLines)
        output << registrationLine << '\n';
    output << "}\n";
    output << "}\n";
    return static_cast<bool>(output);
}

bool SceneEditorController::saveScene()
{
    if (m_scene == nullptr)
        return false;

    if (m_currentSceneFilePath.empty())
        return saveSceneAs();

    if (!scene_serialization::saveSceneToFile(*m_scene, m_currentSceneFilePath))
        return false;

    clearSceneDirty();
    return true;
}

bool SceneEditorController::loadSceneFromFilePath(const std::string& filePath)
{
    if (m_scene == nullptr)
        return false;

    stopExternalProcess(false);
    m_scene->setPhysicsSimulationEnabled(false);
    m_playbackState = PlaybackState::Stopped;
    m_runtimeSceneSnapshot.reset();

    if (!scene_serialization::loadSceneFromFile(*m_scene, filePath))
        return false;

    m_currentSceneFilePath = filePath;
    clearSceneDirty();
    sync(*m_scene);
    requestHierarchyRefresh();
    return true;
}

bool SceneEditorController::loadSceneFromDialog()
{
    if (m_scene == nullptr)
        return false;

    const std::filesystem::path defaultPath = m_currentSceneFilePath.empty()
        ? std::filesystem::current_path()
        : std::filesystem::path(m_currentSceneFilePath);

    const std::optional<std::string> selectedPath = platform::showNativeFileDialog(
        platform::FileDialogMode::OpenFile,
        "Load Scene Save",
        defaultPath.string(),
        buildSceneFileDialogFilters());
    if (!selectedPath.has_value())
        return false;

    return loadSceneFromFilePath(*selectedPath);
}

void SceneEditorController::beginPendingSceneAction(PendingSceneAction action, const std::string& targetPath)
{
    if (m_sceneDirty)
    {
        m_pendingSceneAction = action;
        m_pendingSceneTargetPath = targetPath;
        m_sceneSavePromptOpen = true;
        requestHierarchyRefresh();
        return;
    }

    m_pendingSceneAction = action;
    m_pendingSceneTargetPath = targetPath;
    executePendingSceneAction();
}

bool SceneEditorController::executePendingSceneAction()
{
    const PendingSceneAction action = m_pendingSceneAction;
    const std::string targetPath = m_pendingSceneTargetPath;
    m_pendingSceneAction = PendingSceneAction::None;
    m_pendingSceneTargetPath.clear();

    switch (action)
    {
    case PendingSceneAction::LoadFromDialog:
        return loadSceneFromDialog();
    case PendingSceneAction::OpenFile:
        return !targetPath.empty() && loadSceneFromFilePath(targetPath);
    case PendingSceneAction::None:
    default:
        return false;
    }
}

void SceneEditorController::closePendingSceneActionPrompt()
{
    m_sceneSavePromptOpen = false;
    m_pendingSceneAction = PendingSceneAction::None;
    m_pendingSceneTargetPath.clear();
}

void SceneEditorController::requestHierarchyRefresh()
{
    m_hierarchyRefreshPending = true;
}

void SceneEditorController::requestSelectionRefresh()
{
    m_selectionRefreshPending = true;
}