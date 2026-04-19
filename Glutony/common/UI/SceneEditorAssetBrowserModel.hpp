#pragma once

#include <common/ui/EditorUiCommon.hpp>

#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

namespace UI
{
class SceneEditorAssetBrowserModel
{
public:
    enum class RootKind
    {
        Assets,
        BuiltIn,
    };

    enum class FileKind
    {
        Generic,
        Material,
        RenderPhase,
        RenderPass,
        UniformFactory,
        Data,
        Mesh,
        SceneScript,
        ComponentScript,
        Shader,
        Texture,
        Scene,
    };

    struct FileEntry
    {
        std::string id;
        std::string label;
        std::string runtimePath;
        std::string diskPath;
        std::string extension;
        RootKind rootKind = RootKind::Assets;
        FileKind fileKind = FileKind::Generic;
        DragPayloadKind dragPayloadKind = DragPayloadKind::AssetFile;
    };

    struct DirectoryNode
    {
        std::string id;
        std::string label;
        std::string runtimePath;
        std::string diskPath;
        RootKind rootKind = RootKind::Assets;
        std::vector<DirectoryNode> children;
        std::vector<FileEntry> files;
    };

    void clear();
    void rescan(const std::filesystem::path& runtimeRoot);

    DirectoryNode* findDirectoryById(const std::string& directoryId);
    const DirectoryNode* findDirectoryById(const std::string& directoryId) const;
    const FileEntry* findFileById(const std::string& fileId) const;
    const DirectoryNode* selectedDirectory() const;

    const std::vector<DirectoryNode>& roots() const;
    const std::string& selectedDirectoryId() const;
    const std::string& selectedFileId() const;

    bool selectDirectory(const std::string& directoryId);
    void setSelectedFileId(const std::string& fileId);
    void clearSelectedFile();
    void toggleDirectoryExpansion(const std::string& directoryId);
    bool isDirectoryExpanded(const DirectoryNode& node) const;

private:
    std::vector<DirectoryNode> m_roots;
    std::unordered_set<std::string> m_expandedDirectoryIds;
    std::string m_selectedDirectoryId;
    std::string m_selectedFileId;
};
}