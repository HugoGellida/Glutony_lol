#include <common/UI/SceneEditorAssetBrowserModel.hpp>

#include <algorithm>
#include <system_error>

namespace
{
using RootKind = UI::SceneEditorAssetBrowserModel::RootKind;
using FileKind = UI::SceneEditorAssetBrowserModel::FileKind;
using DirectoryNode = UI::SceneEditorAssetBrowserModel::DirectoryNode;
using FileEntry = UI::SceneEditorAssetBrowserModel::FileEntry;

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

FileKind classifyAssetBrowserFileKind(const std::filesystem::path& path)
{
    if (isShaderAssetDirectory(path))
        return FileKind::Shader;

    const std::string extension = path.extension().string();
    if (extension == ".mat")
        return FileKind::Material;
    if (extension == ".render_phase")
        return FileKind::RenderPhase;
    if (extension == ".render_pass")
        return FileKind::RenderPass;
    if (extension == ".data")
        return FileKind::Data;
    if (extension == ".obj" || extension == ".off")
        return FileKind::Mesh;
    if (extension == ".scene_script")
        return FileKind::SceneScript;
    if (extension == ".component_script")
        return FileKind::ComponentScript;
    if (extension == ".glsl")
        return FileKind::Shader;
    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp" || extension == ".tga")
        return FileKind::Texture;
    if (extension == ".scene" || extension == ".snapshot")
        return FileKind::Scene;
    return FileKind::Generic;
}

DragPayloadKind dragPayloadKindForAssetFileKind(FileKind fileKind)
{
    switch (fileKind)
    {
    case FileKind::Material:
        return DragPayloadKind::MaterialAsset;
    case FileKind::RenderPhase:
        return DragPayloadKind::RenderPhaseAsset;
    case FileKind::RenderPass:
        return DragPayloadKind::RenderPassAsset;
    case FileKind::Data:
        return DragPayloadKind::DataAsset;
    case FileKind::Mesh:
        return DragPayloadKind::MeshAsset;
    case FileKind::SceneScript:
        return DragPayloadKind::SceneScriptAsset;
    case FileKind::ComponentScript:
        return DragPayloadKind::ComponentScriptAsset;
    case FileKind::Shader:
        return DragPayloadKind::ShaderAsset;
    case FileKind::Texture:
        return DragPayloadKind::TextureAsset;
    default:
        return DragPayloadKind::AssetFile;
    }
}
}

namespace UI
{
void SceneEditorAssetBrowserModel::clear()
{
    m_roots.clear();
    m_expandedDirectoryIds.clear();
    m_selectedDirectoryId.clear();
    m_selectedFileId.clear();
}

void SceneEditorAssetBrowserModel::rescan(const std::filesystem::path& runtimeRoot)
{
    const std::string previousDirectoryId = m_selectedDirectoryId;
    const std::string previousFileId = m_selectedFileId;
    const std::unordered_set<std::string> previousExpandedDirectories = m_expandedDirectoryIds;

    m_roots.clear();

    struct RootDescriptor
    {
        RootKind kind;
        std::string label;
        std::filesystem::path diskPath;
        std::string runtimePath;
    };

    const std::vector<RootDescriptor> rootDescriptors = {
        {RootKind::Assets, "Assets", runtimeRoot / "Assets", "Assets"},
        {RootKind::BuiltIn, "built-in", runtimeRoot / "built-in", "built-in"},
    };

    const auto makeDirectoryNode = [](RootKind kind, const std::string& label, const std::filesystem::path& diskPath, const std::string& runtimePath) {
        DirectoryNode node;
        node.id = runtimePath;
        node.label = label;
        node.runtimePath = runtimePath;
        node.diskPath = diskPath.string();
        node.rootKind = kind;
        return node;
    };

    auto scanDirectory = [&](auto&& self, DirectoryNode& node) -> void {
        std::error_code errorCode;
        const std::filesystem::path diskPath(node.diskPath);
        if (!std::filesystem::exists(diskPath, errorCode) || !std::filesystem::is_directory(diskPath, errorCode))
            return;

        std::vector<std::filesystem::directory_entry> childDirectories;
        std::vector<std::filesystem::directory_entry> childFiles;
        for (std::filesystem::directory_iterator iterator(diskPath, errorCode); !errorCode && iterator != std::filesystem::directory_iterator(); iterator.increment(errorCode))
        {
            const std::filesystem::directory_entry& entry = *iterator;
            if (entry.is_directory(errorCode))
                childDirectories.push_back(entry);
            else if (entry.is_regular_file(errorCode))
                childFiles.push_back(entry);
        }

        const auto sortEntries = [](auto& entries) {
            std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
                return lhs.path().filename().string() < rhs.path().filename().string();
            });
        };

        sortEntries(childDirectories);
        sortEntries(childFiles);

        for (const std::filesystem::directory_entry& entry : childDirectories)
        {
            const std::string childLabel = entry.path().filename().string();
            const std::string childRuntimePath = node.runtimePath + "/" + childLabel;

            if (isShaderAssetDirectory(entry.path()))
            {
                FileEntry file;
                file.label = childLabel;
                file.runtimePath = childRuntimePath;
                file.diskPath = entry.path().string();
                file.id = file.runtimePath;
                file.extension.clear();
                file.rootKind = node.rootKind;
                file.fileKind = FileKind::Shader;
                file.dragPayloadKind = dragPayloadKindForAssetFileKind(file.fileKind);
                node.files.push_back(std::move(file));
                continue;
            }

            node.children.push_back(makeDirectoryNode(node.rootKind, childLabel, entry.path(), childRuntimePath));
            self(self, node.children.back());
        }

        for (const std::filesystem::directory_entry& entry : childFiles)
        {
            FileEntry file;
            file.label = entry.path().filename().string();
            file.runtimePath = node.runtimePath + "/" + file.label;
            file.diskPath = entry.path().string();
            file.id = file.runtimePath;
            file.extension = entry.path().extension().string();
            file.rootKind = node.rootKind;
            file.fileKind = classifyAssetBrowserFileKind(entry.path());
            file.dragPayloadKind = dragPayloadKindForAssetFileKind(file.fileKind);
            node.files.push_back(std::move(file));
        }
    };

    for (const RootDescriptor& rootDescriptor : rootDescriptors)
    {
        m_roots.push_back(makeDirectoryNode(rootDescriptor.kind, rootDescriptor.label, rootDescriptor.diskPath, rootDescriptor.runtimePath));
        scanDirectory(scanDirectory, m_roots.back());
    }

    m_expandedDirectoryIds.clear();
    for (const std::string& expandedDirectoryId : previousExpandedDirectories)
    {
        if (findDirectoryById(expandedDirectoryId) != nullptr)
            m_expandedDirectoryIds.insert(expandedDirectoryId);
    }
    m_expandedDirectoryIds.insert("Assets");
    m_expandedDirectoryIds.insert("built-in");

    if (findDirectoryById(previousDirectoryId) != nullptr)
        m_selectedDirectoryId = previousDirectoryId;
    else if (!m_roots.empty())
        m_selectedDirectoryId = m_roots.front().id;
    else
        m_selectedDirectoryId.clear();

    if (const FileEntry* file = findFileById(previousFileId))
    {
        const DirectoryNode* selectedDirectoryNode = selectedDirectory();
        if (selectedDirectoryNode != nullptr && editor_ui::startsWith(file->runtimePath, selectedDirectoryNode->runtimePath + "/"))
            m_selectedFileId = previousFileId;
        else
            m_selectedFileId.clear();
    }
    else
    {
        m_selectedFileId.clear();
    }
}

SceneEditorAssetBrowserModel::DirectoryNode* SceneEditorAssetBrowserModel::findDirectoryById(const std::string& directoryId)
{
    auto findNode = [&](auto&& self, DirectoryNode& node) -> DirectoryNode* {
        if (node.id == directoryId)
            return &node;

        for (DirectoryNode& child : node.children)
        {
            if (DirectoryNode* found = self(self, child))
                return found;
        }

        return nullptr;
    };

    for (DirectoryNode& root : m_roots)
    {
        if (DirectoryNode* found = findNode(findNode, root))
            return found;
    }

    return nullptr;
}

const SceneEditorAssetBrowserModel::DirectoryNode* SceneEditorAssetBrowserModel::findDirectoryById(const std::string& directoryId) const
{
    return const_cast<SceneEditorAssetBrowserModel*>(this)->findDirectoryById(directoryId);
}

const SceneEditorAssetBrowserModel::FileEntry* SceneEditorAssetBrowserModel::findFileById(const std::string& fileId) const
{
    auto findInNode = [&](auto&& self, const DirectoryNode& node) -> const FileEntry* {
        for (const FileEntry& file : node.files)
        {
            if (file.id == fileId)
                return &file;
        }

        for (const DirectoryNode& child : node.children)
        {
            if (const FileEntry* found = self(self, child))
                return found;
        }

        return nullptr;
    };

    for (const DirectoryNode& root : m_roots)
    {
        if (const FileEntry* found = findInNode(findInNode, root))
            return found;
    }

    return nullptr;
}

const SceneEditorAssetBrowserModel::DirectoryNode* SceneEditorAssetBrowserModel::selectedDirectory() const
{
    return findDirectoryById(m_selectedDirectoryId);
}

const std::vector<SceneEditorAssetBrowserModel::DirectoryNode>& SceneEditorAssetBrowserModel::roots() const
{
    return m_roots;
}

const std::string& SceneEditorAssetBrowserModel::selectedDirectoryId() const
{
    return m_selectedDirectoryId;
}

const std::string& SceneEditorAssetBrowserModel::selectedFileId() const
{
    return m_selectedFileId;
}

bool SceneEditorAssetBrowserModel::selectDirectory(const std::string& directoryId)
{
    if (findDirectoryById(directoryId) == nullptr)
        return false;

    m_selectedDirectoryId = directoryId;
    m_selectedFileId.clear();
    return true;
}

void SceneEditorAssetBrowserModel::setSelectedFileId(const std::string& fileId)
{
    m_selectedFileId = fileId;
}

void SceneEditorAssetBrowserModel::clearSelectedFile()
{
    m_selectedFileId.clear();
}

void SceneEditorAssetBrowserModel::toggleDirectoryExpansion(const std::string& directoryId)
{
    if (directoryId.empty())
        return;

    const auto it = m_expandedDirectoryIds.find(directoryId);
    if (it != m_expandedDirectoryIds.end())
        m_expandedDirectoryIds.erase(it);
    else
        m_expandedDirectoryIds.insert(directoryId);
}

bool SceneEditorAssetBrowserModel::isDirectoryExpanded(const DirectoryNode& node) const
{
    if (node.id == "Assets" || node.id == "built-in")
        return true;

    return m_expandedDirectoryIds.count(node.id) > 0;
}
}