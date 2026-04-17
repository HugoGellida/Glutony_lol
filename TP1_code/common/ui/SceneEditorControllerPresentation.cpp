#include "SceneEditorController.hpp"

#include <common/UI/Container.hpp>
#include <common/UI/MarkupBlock.hpp>
#include <common/UI/MenuBar.hpp>
#include <common/UI/MenuEntry.hpp>
#include <common/UI/MenuItem.hpp>
#include <common/UI/Panel.hpp>
#include <common/UI/PanelAction.hpp>
#include <common/UI/PanelHeader.hpp>
#include <common/UI/Placeholder.hpp>
#include <common/UI/TabButton.hpp>
#include <common/UI/TabHeader.hpp>
#include <common/UI/TabItem.hpp>
#include <common/UI/TabPanel.hpp>
#include <common/UI/TextBlock.hpp>
#include <common/UI/ToolbarButton.hpp>
#include <common/UI/ToolbarGroup.hpp>
#include <common/Scene.hpp>

#include <algorithm>
#include <iomanip>
#include <sstream>

using namespace editor_ui;

namespace
{
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

std::string escapeTextAreaValue(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char character : value)
    {
        switch (character)
        {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '"': escaped += "&quot;"; break;
        case '\'': escaped += "&apos;"; break;
        default: escaped.push_back(character); break;
        }
    }
    return escaped;
}

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

std::string encodeSceneFieldToken(const std::string& value)
{
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (unsigned char character : value)
        stream << std::setw(2) << static_cast<int>(character);
    return stream.str();
}

std::string makeSceneRenderTargetFieldKey(const std::string& renderTargetName, const std::string& propertyKey)
{
    return "render_target__" + encodeSceneFieldToken(renderTargetName) + "__" + propertyKey;
}

const std::vector<component_meta::EnumOption>& renderTargetFormatOptions()
{
    static const std::vector<component_meta::EnumOption> options = {
        {"float", "float"},
        {"rg", "rg"},
        {"rgb", "rgb"},
        {"rgba", "rgba"},
        {"rgba16f", "rgba16f"},
    };
    return options;
}

std::string makeSceneInspectorGroupElementId(int nodeId, size_t componentIndex)
{
    return "scene_inspector_group_" + std::to_string(nodeId) + "__" + std::to_string(componentIndex);
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
        if (fieldKind == component_meta::FieldKind::Float)
            stream << " step='any'";
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

const char* assetBrowserRootLabel(SceneEditorController::AssetBrowserRootKind rootKind)
{
    return rootKind == SceneEditorController::AssetBrowserRootKind::Assets ? "Assets" : "built-in";
}

const char* assetBrowserRootClass(SceneEditorController::AssetBrowserRootKind rootKind)
{
    return rootKind == SceneEditorController::AssetBrowserRootKind::Assets ? "user" : "builtin";
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
}

void SceneEditorController::refreshPresentation()
{
    if (m_builderHeader == nullptr || m_leftPanel == nullptr || m_rightPanel == nullptr || m_bottomPanel == nullptr)
        return;

    m_builderHeader->SetInnerRML(buildSceneEditorMenuMarkup(m_isFileMenuOpen, m_isEditMenuOpen, m_isWindowMenuOpen));

    m_leftPanel->SetInnerRML(
        buildHierarchyMarkup()
    );
    m_viewportPanel->SetInnerRML(buildViewportMarkup());
    m_viewportSurface = m_document->GetElementById("scene_viewport_surface");
    m_playbackStatusElement = m_document->GetElementById("scene_playback_status");
    updatePlaybackStatusPresentation();
    m_rightPanel->SetInnerRML(
        buildInspectorMarkup()
    );
    m_bottomPanel->SetInnerRML(buildAssetBrowserMarkup());
    m_bottomBrowserFilesPane = m_document->GetElementById("scene_asset_browser_files_pane");
    m_bottomBrowserSplitter = m_document->GetElementById("scene_asset_browser_splitter");
    m_bottomBrowserTreePane = m_document->GetElementById("scene_asset_browser_tree_pane");
    m_consoleOutputElement = m_document->GetElementById("scene_console_output");
    m_consoleOutputSpacerElement = m_document->GetElementById("scene_console_output_spacer");
    m_consoleSelectionElement = m_document->GetElementById("scene_console_selection");
}

void SceneEditorController::refreshHierarchyPresentation()
{
    if (m_leftPanel == nullptr)
        return;

    m_leftPanel->SetInnerRML(buildHierarchyMarkup());
}

void SceneEditorController::refreshViewportPresentation()
{
    if (m_viewportPanel == nullptr)
        return;

    m_viewportPanel->SetInnerRML(buildViewportMarkup());
    m_viewportSurface = m_document != nullptr ? m_document->GetElementById("scene_viewport_surface") : nullptr;
    m_layoutManager.setViewportSurface(m_viewportSurface);
    m_playbackStatusElement = m_document != nullptr ? m_document->GetElementById("scene_playback_status") : nullptr;
    updatePlaybackStatusPresentation();
}

void SceneEditorController::refreshInspectorPresentation(bool preserveScroll)
{
    if (m_rightPanel == nullptr)
        return;

    float previousScrollTop = 0.0f;
    float previousScrollLeft = 0.0f;
    if (preserveScroll && m_document != nullptr)
    {
        if (Rml::Element* inspectorBody = m_document->GetElementById("scene_inspector_panel_body"))
        {
            previousScrollTop = inspectorBody->GetScrollTop();
            previousScrollLeft = inspectorBody->GetScrollLeft();
        }
    }

    const std::string nextMarkup = buildInspectorMarkup();
    if (m_rightPanel->GetInnerRML() == nextMarkup)
        return;

    m_rightPanel->SetInnerRML(nextMarkup);

    m_pendingInspectorScrollRestore = preserveScroll;
    m_pendingInspectorScrollTop = previousScrollTop;
    m_pendingInspectorScrollLeft = previousScrollLeft;
}

void SceneEditorController::refreshInspectorOverlayPresentation()
{
    if (m_document == nullptr)
        return;

    if (Rml::Element* overlay = m_document->GetElementById("scene_inspector_overlay"))
        overlay->SetInnerRML(buildInspectorOverlayMarkup());
}

void SceneEditorController::refreshBottomPanelPresentation(bool preserveScroll)
{
    if (m_bottomPanel == nullptr)
        return;

    float previousFilesScrollTop = 0.0f;
    float previousFilesScrollLeft = 0.0f;
    float previousTreeScrollTop = 0.0f;
    float previousTreeScrollLeft = 0.0f;
    float previousConsoleScrollTop = 0.0f;
    float previousConsoleScrollLeft = 0.0f;
    bool restoreConsoleScroll = false;
    bool stickConsoleToBottom = false;

    if (preserveScroll)
    {
        if (m_bottomBrowserFilesPane != nullptr)
        {
            previousFilesScrollTop = m_bottomBrowserFilesPane->GetScrollTop();
            previousFilesScrollLeft = m_bottomBrowserFilesPane->GetScrollLeft();
        }

        if (m_bottomBrowserTreePane != nullptr)
        {
            previousTreeScrollTop = m_bottomBrowserTreePane->GetScrollTop();
            previousTreeScrollLeft = m_bottomBrowserTreePane->GetScrollLeft();
        }

        if (m_consoleSelectionElement != nullptr)
        {
            previousConsoleScrollTop = m_consoleSelectionElement->GetScrollTop();
            previousConsoleScrollLeft = m_consoleSelectionElement->GetScrollLeft();
            restoreConsoleScroll = true;
            stickConsoleToBottom =
                (m_consoleSelectionElement->GetScrollHeight() - (previousConsoleScrollTop + m_consoleSelectionElement->GetClientHeight())) <= 4.0f;
        }
    }

    m_bottomPanel->SetInnerRML(buildAssetBrowserMarkup());
    m_bottomBrowserFilesPane = m_document != nullptr ? m_document->GetElementById("scene_asset_browser_files_pane") : nullptr;
    m_bottomBrowserSplitter = m_document != nullptr ? m_document->GetElementById("scene_asset_browser_splitter") : nullptr;
    m_bottomBrowserTreePane = m_document != nullptr ? m_document->GetElementById("scene_asset_browser_tree_pane") : nullptr;
    m_consoleOutputElement = m_document != nullptr ? m_document->GetElementById("scene_console_output") : nullptr;
    m_consoleOutputSpacerElement = m_document != nullptr ? m_document->GetElementById("scene_console_output_spacer") : nullptr;
    m_consoleSelectionElement = m_document != nullptr ? m_document->GetElementById("scene_console_selection") : nullptr;

    if (!preserveScroll)
    {
        m_pendingConsoleScrollRestore = false;
        m_pendingConsoleStickToBottom = false;
        return;
    }

    if (m_bottomBrowserFilesPane != nullptr)
    {
        m_bottomBrowserFilesPane->SetScrollTop(previousFilesScrollTop);
        m_bottomBrowserFilesPane->SetScrollLeft(previousFilesScrollLeft);
    }

    if (m_bottomBrowserTreePane != nullptr)
    {
        m_bottomBrowserTreePane->SetScrollTop(previousTreeScrollTop);
        m_bottomBrowserTreePane->SetScrollLeft(previousTreeScrollLeft);
    }

    if (restoreConsoleScroll && m_consoleSelectionElement != nullptr)
    {
        m_pendingConsoleScrollRestore = true;
        m_pendingConsoleStickToBottom = stickConsoleToBottom;
        m_pendingConsoleScrollTop = previousConsoleScrollTop;
        m_pendingConsoleScrollLeft = previousConsoleScrollLeft;
    }
    else
    {
        m_pendingConsoleScrollRestore = false;
        m_pendingConsoleStickToBottom = false;
    }
}

void SceneEditorController::refreshAssetBrowserWorkspacePresentation(bool preserveScroll)
{
    if (m_document == nullptr)
        return;

    float previousFilesScrollTop = 0.0f;
    float previousFilesScrollLeft = 0.0f;
    float previousTreeScrollTop = 0.0f;
    float previousTreeScrollLeft = 0.0f;

    if (preserveScroll)
    {
        if (m_bottomBrowserFilesPane != nullptr)
        {
            previousFilesScrollTop = m_bottomBrowserFilesPane->GetScrollTop();
            previousFilesScrollLeft = m_bottomBrowserFilesPane->GetScrollLeft();
        }

        if (m_bottomBrowserTreePane != nullptr)
        {
            previousTreeScrollTop = m_bottomBrowserTreePane->GetScrollTop();
            previousTreeScrollLeft = m_bottomBrowserTreePane->GetScrollLeft();
        }
    }

    Rml::Element* workspace = m_document->GetElementById("scene_asset_browser_workspace");
    if (workspace == nullptr)
        return;

    workspace->SetInnerRML(buildAssetBrowserWorkspaceMarkup());
    m_bottomBrowserFilesPane = m_document->GetElementById("scene_asset_browser_files_pane");
    m_bottomBrowserSplitter = m_document->GetElementById("scene_asset_browser_splitter");
    m_bottomBrowserTreePane = m_document->GetElementById("scene_asset_browser_tree_pane");

    if (!preserveScroll)
        return;

    if (m_bottomBrowserFilesPane != nullptr)
    {
        m_bottomBrowserFilesPane->SetScrollTop(previousFilesScrollTop);
        m_bottomBrowserFilesPane->SetScrollLeft(previousFilesScrollLeft);
    }

    if (m_bottomBrowserTreePane != nullptr)
    {
        m_bottomBrowserTreePane->SetScrollTop(previousTreeScrollTop);
        m_bottomBrowserTreePane->SetScrollLeft(previousTreeScrollLeft);
    }
}

void SceneEditorController::refreshAssetBrowserTreePresentation(bool preserveScroll)
{
    if (m_document == nullptr)
        return;

    float previousScrollTop = 0.0f;
    float previousScrollLeft = 0.0f;
    if (preserveScroll && m_bottomBrowserTreePane != nullptr)
    {
        previousScrollTop = m_bottomBrowserTreePane->GetScrollTop();
        previousScrollLeft = m_bottomBrowserTreePane->GetScrollLeft();
    }

    if (Rml::Element* treePane = m_document->GetElementById("scene_asset_browser_tree_pane"))
        treePane->SetInnerRML(buildAssetBrowserTreePaneMarkup());

    m_bottomBrowserTreePane = m_document->GetElementById("scene_asset_browser_tree_pane");
    if (preserveScroll && m_bottomBrowserTreePane != nullptr)
    {
        m_bottomBrowserTreePane->SetScrollTop(previousScrollTop);
        m_bottomBrowserTreePane->SetScrollLeft(previousScrollLeft);
    }
}

void SceneEditorController::refreshAssetBrowserFilesPresentation(bool preserveScroll)
{
    if (m_document == nullptr)
        return;

    float previousScrollTop = 0.0f;
    float previousScrollLeft = 0.0f;
    if (preserveScroll && m_bottomBrowserFilesPane != nullptr)
    {
        previousScrollTop = m_bottomBrowserFilesPane->GetScrollTop();
        previousScrollLeft = m_bottomBrowserFilesPane->GetScrollLeft();
    }

    if (Rml::Element* filesPane = m_document->GetElementById("scene_asset_browser_files_pane"))
        filesPane->SetInnerRML(buildAssetBrowserFilesPaneMarkup());

    m_bottomBrowserFilesPane = m_document->GetElementById("scene_asset_browser_files_pane");
    if (preserveScroll && m_bottomBrowserFilesPane != nullptr)
    {
        m_bottomBrowserFilesPane->SetScrollTop(previousScrollTop);
        m_bottomBrowserFilesPane->SetScrollLeft(previousScrollLeft);
    }
}

void SceneEditorController::refreshAssetBrowserOverlayPresentation()
{
    if (m_document == nullptr)
        return;

    if (Rml::Element* overlay = m_document->GetElementById("scene_asset_browser_overlay"))
        overlay->SetInnerRML(buildAssetBrowserOverlayMarkup());
}

void SceneEditorController::refreshAssetBrowserFileSelectionPresentation(const std::string& previousFileId)
{
    if (m_document == nullptr)
        return;

    auto updateFileElementClass = [&](const std::string& fileId) {
        if (fileId.empty())
            return;

        const AssetBrowserFileEntry* file = m_assetBrowserModel.findFileById(fileId);
        if (file == nullptr)
            return;

        Rml::Element* element = m_document->GetElementById(UI::SceneEditorDomIdCodec::makeAssetFileElementId(fileId));
        if (element == nullptr)
            return;

        std::string classNames = "asset_browser_file_card ";
        classNames += assetBrowserRootClass(file->rootKind);
        classNames += " ";
        classNames += assetBrowserFileKindClass(file->fileKind);
        if (fileId == m_assetBrowserModel.selectedFileId())
            classNames += " selected";
        if (fileId == m_draggedAssetFileId)
            classNames += " dragging";

        element->SetClassNames(classNames);
    };

    if (previousFileId != m_assetBrowserModel.selectedFileId())
        updateFileElementClass(previousFileId);
    updateFileElementClass(m_assetBrowserModel.selectedFileId());
}

void SceneEditorController::refreshAssetBrowserDirectorySelectionPresentation(const std::string& previousDirectoryId)
{
    if (m_document == nullptr)
        return;

    auto updateDirectoryElementClass = [&](const std::string& directoryId) {
        if (directoryId.empty())
            return;

        const AssetBrowserDirectoryNode* directory = m_assetBrowserModel.findDirectoryById(directoryId);
        if (directory == nullptr)
            return;

        Rml::Element* element = m_document->GetElementById(UI::SceneEditorDomIdCodec::makeAssetDirectoryElementId(directoryId));
        if (element == nullptr)
            return;

        std::string classNames = "asset_browser_tree_row ";
        classNames += assetBrowserRootClass(directory->rootKind);
        if (directoryId == m_assetBrowserModel.selectedDirectoryId())
            classNames += " selected";
        if (m_assetBrowserModel.isDirectoryExpanded(*directory))
            classNames += " expanded";
        element->SetClassNames(classNames);
    };

    if (previousDirectoryId != m_assetBrowserModel.selectedDirectoryId())
        updateDirectoryElementClass(previousDirectoryId);
    updateDirectoryElementClass(m_assetBrowserModel.selectedDirectoryId());
}

void SceneEditorController::refreshInspectorValuesPresentation()
{
    const UiGOHierarchyNode* selectedNode = m_hierarchyModel.findSelectedNode();
    if (m_document == nullptr || selectedNode == nullptr)
        return;

    const Rml::Element* focusedElement = m_context != nullptr ? m_context->GetFocusElement() : nullptr;

    auto isFieldFocused = [&](Rml::Element* fieldElement) {
        if (fieldElement == nullptr)
            return false;

        for (const Rml::Element* element = focusedElement; element != nullptr; element = element->GetParentNode())
        {
            if (element == fieldElement)
                return true;
        }

        return false;
    };

    auto refreshVec3FieldValue = [&](const std::string& elementId, const glm::vec3& value) {
        Rml::Element* fieldElement = m_document->GetElementById(elementId);
        Rml::ElementFormControl* formControl = dynamic_cast<Rml::ElementFormControl*>(fieldElement);
        if (formControl == nullptr || isFieldFocused(fieldElement))
            return;

        const std::string formattedValue = formatSerializedValue(value);
        if (formControl->GetValue() != formattedValue)
            formControl->SetValue(formattedValue);
    };

    if (Rml::Element* childrenValue = m_document->GetElementById("scene_inspector_children_value"))
    {
        const std::string nextChildrenValue = std::to_string(selectedNode->children.size());
        if (childrenValue->GetInnerRML() != nextChildrenValue)
            childrenValue->SetInnerRML(nextChildrenValue);
    }

    if (selectedNode->gameObject == nullptr)
    {
        auto refreshStringFieldValue = [&](const std::string& elementId, const std::string& value) {
            Rml::Element* fieldElement = m_document->GetElementById(elementId);
            Rml::ElementFormControl* formControl = dynamic_cast<Rml::ElementFormControl*>(fieldElement);
            if (formControl == nullptr || isFieldFocused(fieldElement))
                return;

            if (formControl->GetValue() != value)
                formControl->SetValue(value);
        };

        if (m_scene != nullptr)
        {
            refreshStringFieldValue(makeSceneFieldElementId("sceneScriptAsset"), m_scene->getSceneScriptAssetPath());
            refreshStringFieldValue(makeSceneFieldElementId("dataAsset"), m_scene->getDataAssetPath());
            const render::DirectionalLightSettings& directionalLight = m_scene->getDirectionalLightSettings();
            refreshStringFieldValue(makeSceneFieldElementId("directionalLightEnabled"), directionalLight.enabled ? "true" : "false");
            refreshStringFieldValue(makeSceneFieldElementId("directionalLightDirection"), formatSerializedValue(component_meta::SerializedValue(directionalLight.direction)));
            refreshStringFieldValue(makeSceneFieldElementId("directionalLightColor"), formatSerializedValue(component_meta::SerializedValue(directionalLight.color)));
            refreshStringFieldValue(makeSceneFieldElementId("directionalLightIntensity"), formatSerializedValue(component_meta::SerializedValue(directionalLight.intensity)));
            const std::vector<render::SceneRenderTargetSettings> renderTargets = m_scene != nullptr ? m_scene->collectVisibleRenderTargetSettings() : std::vector<render::SceneRenderTargetSettings>();
            if (!renderTargets.empty())
            {
                for (const render::SceneRenderTargetSettings& renderTarget : renderTargets)
                {
                    refreshStringFieldValue(makeSceneFieldElementId(makeSceneRenderTargetFieldKey(renderTarget.name, "width")), std::to_string(renderTarget.width));
                    refreshStringFieldValue(makeSceneFieldElementId(makeSceneRenderTargetFieldKey(renderTarget.name, "height")), std::to_string(renderTarget.height));
                    refreshStringFieldValue(makeSceneFieldElementId(makeSceneRenderTargetFieldKey(renderTarget.name, "format")), render::renderTargetFormatName(renderTarget.format));
                    refreshStringFieldValue(makeSceneFieldElementId(makeSceneRenderTargetFieldKey(renderTarget.name, "bakedTexture")), renderTarget.bakedTextureAssetPath);
                }
            }

            InspectorFieldBinding dataAssetBinding;
            dataAssetBinding.target = InspectorFieldBinding::Target::Scene;
            dataAssetBinding.nodeId = 0;
            dataAssetBinding.fieldKey = "dataAsset";
            refreshDataAssetEditorPresentation(dataAssetBinding);
        }
        return;
    }

    const glm::vec3& position = selectedNode->gameObject->transform.getPosition();
    refreshVec3FieldValue(makeTransformFieldElementId(selectedNode->id, "position"), position);

    const glm::vec3& rotation = selectedNode->gameObject->transform.getRotation();
    refreshVec3FieldValue(makeTransformFieldElementId(selectedNode->id, "rotation"), rotation);

    const glm::vec3& scale = selectedNode->gameObject->transform.getScale();
    refreshVec3FieldValue(makeTransformFieldElementId(selectedNode->id, "scale"), scale);

    for (size_t componentIndex = 0; componentIndex < selectedNode->gameObject->getComponentCount(); ++componentIndex)
    {
        const component::Component* component = selectedNode->gameObject->getComponentAt(componentIndex);
        if (component == nullptr)
            continue;

        const component_meta::ComponentDescriptor* descriptor = component->getComponentDescriptor();
        if (descriptor == nullptr)
            continue;

        for (const component_meta::ComponentFieldDescriptor& field : descriptor->fields)
        {
            if (!field.read)
                continue;

            Rml::Element* fieldElement = m_document->GetElementById(makeInspectorFieldElementId(selectedNode->id, componentIndex, field.key));
            Rml::ElementFormControl* formControl = dynamic_cast<Rml::ElementFormControl*>(fieldElement);
            if (formControl == nullptr)
                continue;

            if (isFieldFocused(fieldElement))
                continue;

            const std::string formattedValue = formatSerializedValue(field.read(*component));
            if (formControl->GetValue() != formattedValue)
                formControl->SetValue(formattedValue);

            if (field.assetReferenceKind == component_meta::AssetReferenceKind::Material)
            {
                InspectorFieldBinding binding;
                binding.target = InspectorFieldBinding::Target::Component;
                binding.nodeId = selectedNode->id;
                binding.componentIndex = componentIndex;
                binding.fieldKey = field.key;
                refreshMaterialAssetEditorPresentation(binding);
            }
            else if (field.assetReferenceKind == component_meta::AssetReferenceKind::Data)
            {
                InspectorFieldBinding binding;
                binding.target = InspectorFieldBinding::Target::Component;
                binding.nodeId = selectedNode->id;
                binding.componentIndex = componentIndex;
                binding.fieldKey = field.key;
                refreshDataAssetEditorPresentation(binding);
            }
        }
    }
}

void SceneEditorController::refreshCachedRects()
{
    m_layoutManager.setViewportSurface(m_viewportSurface);
    m_layoutManager.refreshCachedRects();
}

std::string SceneEditorController::buildHierarchyMarkup() const
{
    UI::Panel panel(0, 0);
    panel.addClassName("hierarchy_shell");
    panel.setContentDomIdOverride("scene_hierarchy_body");
    panel.addContentClassName("hierarchy_body");

    UI::PanelHeader header(0, 0, "Scene");
    header.addClassName("panel_header_with_action");

    UI::PanelAction addAction(0, 0, "+");
    addAction.setDomIdOverride("scene_hierarchy_add");
    header.addChild(&addAction);

    UI::MarkupBlock body(0, 0, buildHierarchyNodeMarkup(m_hierarchyModel.root(), 0) + buildHierarchyContextMenuMarkup());

    panel.addChild(&header);
    panel.addChild(&body);
    return panel.getRML();
}

std::string SceneEditorController::buildHierarchyContextMenuMarkup() const
{
    if (!m_hierarchyContextMenuOpen || m_hierarchyContextMenuNodeId == m_hierarchyModel.root().id)
        return "";

    std::ostringstream stream;
    stream << "<div class='hierarchy_context_menu' style='left: " << m_hierarchyContextMenuX << "px; top: " << m_hierarchyContextMenuY << "px;'>";
    stream << "<div id='scene_hierarchy_delete' class='hierarchy_context_item danger'>Delete</div>";
    stream << "</div>";
    return stream.str();
}

std::string SceneEditorController::buildHierarchyNodeMarkup(const UiGOHierarchyNode& node, int depth) const
{
    const bool isSelected = node.id == m_hierarchyModel.selectedNodeId();

    std::ostringstream stream;
    stream << "<div class='hierarchy_node depth_" << depth << "'>";
    stream << "<div id='" << UI::SceneEditorDomIdCodec::makeHierarchyNodeElementId(node.id) << "' class='hierarchy_row scene_hierarchy_row";
    if (isSelected)
        stream << " selected";
    stream << "'>";
    stream << "<div class='hierarchy_label'>" << escapeRmlText(node.label) << "</div>";
    stream << "<div class='hierarchy_meta'>&lt;" << escapeRmlText(node.tagName) << "&gt;</div>";
    stream << "</div>";

    if (!node.children.empty())
    {
        stream << "<div class='hierarchy_children'>";
        for (const UiGOHierarchyNode& child : node.children)
            stream << buildHierarchyNodeMarkup(child, depth + 1);
        stream << "</div>";
    }

    stream << "</div>";
    return stream.str();
}

std::string SceneEditorController::buildInspectorMarkup() const
{
    const UiGOHierarchyNode* selectedNode = m_hierarchyModel.findSelectedNode();
    if (selectedNode == nullptr)
    {
        return buildInspectorShellMarkup(
            buildInspectorPlaceholderMarkup("Inspector", "Select a game object in the scene hierarchy."),
            std::string());
    }

    if (selectedNode->gameObject == nullptr)
    {
        const std::string sceneScriptFieldId = makeSceneFieldElementId("sceneScriptAsset");
        const std::string dataAssetFieldId = makeSceneFieldElementId("dataAsset");
        InspectorFieldBinding dataAssetBinding;
        dataAssetBinding.target = InspectorFieldBinding::Target::Scene;
        dataAssetBinding.nodeId = 0;
        dataAssetBinding.fieldKey = "dataAsset";

        std::ostringstream stream;
        stream << "<div class='inspector_summary'>";
        stream << "<div class='inspector_summary_title'>" << escapeRmlText(selectedNode->label) << "</div>";
        stream << "<div class='inspector_summary_text'>Scene Root</div>";
        stream << "</div>";
        stream << "<div class='inspector_section'>";
        stream << "<div class='inspector_field_row'><div class='inspector_field_name'>Children</div><div id='scene_inspector_children_value' class='inspector_field_input'>" << selectedNode->children.size() << "</div></div>";
        stream << buildInspectorFieldMarkup(
            sceneScriptFieldId,
            "SceneScript",
            component_meta::FieldKind::Asset,
            m_scene != nullptr ? component_meta::SerializedValue(m_scene->getSceneScriptAssetPath()) : component_meta::SerializedValue(std::string()),
            {},
            component_meta::AssetReferenceKind::SceneScript,
            sceneScriptFieldId == m_hoveredInspectorFieldId);
        stream << buildInspectorFieldMarkup(
            dataAssetFieldId,
            "DataAsset",
            component_meta::FieldKind::Asset,
            m_scene != nullptr ? component_meta::SerializedValue(m_scene->getDataAssetPath()) : component_meta::SerializedValue(std::string()),
            {},
            component_meta::AssetReferenceKind::Data,
            dataAssetFieldId == m_hoveredInspectorFieldId);
        stream << buildDataAssetEditorMarkup(dataAssetBinding, m_scene != nullptr ? m_scene->getDataAssetPath() : std::string());

        const render::DirectionalLightSettings directionalLight = m_scene != nullptr ? m_scene->getDirectionalLightSettings() : render::DirectionalLightSettings();
        const std::string directionalLightEnabledFieldId = makeSceneFieldElementId("directionalLightEnabled");
        const std::string directionalLightDirectionFieldId = makeSceneFieldElementId("directionalLightDirection");
        const std::string directionalLightColorFieldId = makeSceneFieldElementId("directionalLightColor");
        const std::string directionalLightIntensityFieldId = makeSceneFieldElementId("directionalLightIntensity");
        stream << buildInspectorFieldMarkup(
            directionalLightEnabledFieldId,
            "Directional Light",
            component_meta::FieldKind::Bool,
            component_meta::SerializedValue(directionalLight.enabled),
            {},
            component_meta::AssetReferenceKind::None,
            directionalLightEnabledFieldId == m_hoveredInspectorFieldId);
        stream << buildInspectorFieldMarkup(
            directionalLightDirectionFieldId,
            "Light Direction",
            component_meta::FieldKind::Vec3,
            component_meta::SerializedValue(directionalLight.direction),
            {},
            component_meta::AssetReferenceKind::None,
            directionalLightDirectionFieldId == m_hoveredInspectorFieldId);
        stream << buildInspectorFieldMarkup(
            directionalLightColorFieldId,
            "Light Color",
            component_meta::FieldKind::Vec3,
            component_meta::SerializedValue(directionalLight.color),
            {},
            component_meta::AssetReferenceKind::None,
            directionalLightColorFieldId == m_hoveredInspectorFieldId);
        stream << buildInspectorFieldMarkup(
            directionalLightIntensityFieldId,
            "Light Intensity",
            component_meta::FieldKind::Float,
            component_meta::SerializedValue(directionalLight.intensity),
            {},
            component_meta::AssetReferenceKind::None,
            directionalLightIntensityFieldId == m_hoveredInspectorFieldId);
        const std::vector<render::SceneRenderTargetSettings> renderTargets = m_scene != nullptr ? m_scene->collectVisibleRenderTargetSettings() : std::vector<render::SceneRenderTargetSettings>();
        if (!renderTargets.empty())
        {
            stream << "<div class='inspector_field_row'><div class='inspector_field_name'>RenderTargets</div><div class='inspector_field_input'>" << renderTargets.size() << "</div></div>";
            for (const render::SceneRenderTargetSettings& renderTarget : renderTargets)
            {
                stream << "<div class='inspector_field_row'><div class='inspector_field_name'>Target</div><div class='inspector_field_input'>" << escapeRmlText(renderTarget.name) << "</div></div>";

                const std::string widthFieldId = makeSceneFieldElementId(makeSceneRenderTargetFieldKey(renderTarget.name, "width"));
                const std::string heightFieldId = makeSceneFieldElementId(makeSceneRenderTargetFieldKey(renderTarget.name, "height"));
                const std::string formatFieldId = makeSceneFieldElementId(makeSceneRenderTargetFieldKey(renderTarget.name, "format"));
                const std::string bakedTextureFieldId = makeSceneFieldElementId(makeSceneRenderTargetFieldKey(renderTarget.name, "bakedTexture"));

                stream << buildInspectorFieldMarkup(
                    widthFieldId,
                    "Width",
                    component_meta::FieldKind::Int,
                    component_meta::SerializedValue(renderTarget.width),
                    {},
                    component_meta::AssetReferenceKind::None,
                    widthFieldId == m_hoveredInspectorFieldId);
                stream << buildInspectorFieldMarkup(
                    heightFieldId,
                    "Height",
                    component_meta::FieldKind::Int,
                    component_meta::SerializedValue(renderTarget.height),
                    {},
                    component_meta::AssetReferenceKind::None,
                    heightFieldId == m_hoveredInspectorFieldId);
                stream << buildInspectorFieldMarkup(
                    formatFieldId,
                    "Format",
                    component_meta::FieldKind::Enum,
                    component_meta::SerializedValue(std::string(render::renderTargetFormatName(renderTarget.format))),
                    renderTargetFormatOptions(),
                    component_meta::AssetReferenceKind::None,
                    formatFieldId == m_hoveredInspectorFieldId);
                stream << buildInspectorFieldMarkup(
                    bakedTextureFieldId,
                    "Baked Texture",
                    component_meta::FieldKind::Asset,
                    component_meta::SerializedValue(renderTarget.bakedTextureAssetPath),
                    {},
                    component_meta::AssetReferenceKind::Texture,
                    bakedTextureFieldId == m_hoveredInspectorFieldId);
            }
        }
        stream << "</div>";
        return buildInspectorShellMarkup(stream.str(), buildInspectorOverlayMarkup());
    }

    const glm::vec3& position = selectedNode->gameObject->transform.getPosition();
    const glm::vec3& rotation = selectedNode->gameObject->transform.getRotation();
    const glm::vec3& scale = selectedNode->gameObject->transform.getScale();

    std::ostringstream stream;
    stream << "<div class='inspector_summary'>";
    stream << "<div class='inspector_summary_title'>" << escapeRmlText(selectedNode->label) << "</div>";
    stream << "<div class='inspector_summary_text'>GameObject</div>";
    stream << "</div>";

    stream << "<div class='inspector_section'>";
    stream << "<div class='inspector_field_row'><div class='inspector_field_name'>Children</div><div id='scene_inspector_children_value' class='inspector_field_input'>" << selectedNode->children.size() << "</div></div>";
    stream << buildInspectorFieldMarkup(
        makeTransformFieldElementId(selectedNode->id, "position"),
        "Position",
        component_meta::FieldKind::Vec3,
        position,
        {},
        component_meta::AssetReferenceKind::None,
        false
    );
    stream << buildInspectorFieldMarkup(
        makeTransformFieldElementId(selectedNode->id, "rotation"),
        "Rotation",
        component_meta::FieldKind::Vec3,
        rotation,
        {},
        component_meta::AssetReferenceKind::None,
        false
    );
    stream << buildInspectorFieldMarkup(
        makeTransformFieldElementId(selectedNode->id, "scale"),
        "Scale",
        component_meta::FieldKind::Vec3,
        scale,
        {},
        component_meta::AssetReferenceKind::None,
        false
    );
    stream << "</div>";

    size_t serializableComponentCount = 0;
    for (size_t componentIndex = 0; componentIndex < selectedNode->gameObject->getComponentCount(); ++componentIndex)
    {
        const component::Component* component = selectedNode->gameObject->getComponentAt(componentIndex);
        if (component == nullptr)
            continue;

        const component_meta::ComponentDescriptor* descriptor = component->getComponentDescriptor();
        if (descriptor == nullptr)
            continue;

        ++serializableComponentCount;
        const std::string groupId = makeInspectorGroupElementId(selectedNode->id, componentIndex);
        const bool collapsed = isInspectorGroupCollapsed(groupId);
        stream << "<div class='inspector_foldout'>";
        stream << "<div id='" << groupId << "' class='inspector_foldout_header'>";
        stream << "<div class='inspector_foldout_icon'>" << (collapsed ? ">" : "v") << "</div>";
        stream << "<div class='inspector_foldout_title'>" << escapeRmlText(descriptor->displayName) << "</div>";
        stream << "</div>";
        if (!collapsed)
        {
            stream << "<div class='inspector_foldout_body'>";
            stream << "<div class='inspector_summary_text'>type: " << escapeRmlText(descriptor->typeKey) << " | version: " << descriptor->version << "</div>";
            for (const component_meta::ComponentFieldDescriptor& field : descriptor->fields)
            {
                if (!field.read)
                    continue;

                const std::string fieldId = makeInspectorFieldElementId(selectedNode->id, componentIndex, field.key);
                const component_meta::SerializedValue fieldValue = field.read(*component);
                if (field.assetReferenceKind == component_meta::AssetReferenceKind::Material)
                {
                    InspectorFieldBinding binding;
                    binding.target = InspectorFieldBinding::Target::Component;
                    binding.nodeId = selectedNode->id;
                    binding.componentIndex = componentIndex;
                    binding.fieldKey = field.key;
                    const std::string* assetPath = std::get_if<std::string>(&fieldValue);

                    stream << buildInspectorFieldMarkup(selectedNode->id, componentIndex, field, fieldValue, fieldId == m_hoveredInspectorFieldId);
                    stream << buildMaterialAssetEditorMarkup(binding, assetPath != nullptr ? *assetPath : std::string());
                }
                else if (field.assetReferenceKind == component_meta::AssetReferenceKind::Data)
                {
                    InspectorFieldBinding binding;
                    binding.target = InspectorFieldBinding::Target::Component;
                    binding.nodeId = selectedNode->id;
                    binding.componentIndex = componentIndex;
                    binding.fieldKey = field.key;
                    const std::string* assetPath = std::get_if<std::string>(&fieldValue);

                    stream << buildInspectorFieldMarkup(selectedNode->id, componentIndex, field, fieldValue, fieldId == m_hoveredInspectorFieldId);
                    stream << buildDataAssetEditorMarkup(binding, assetPath != nullptr ? *assetPath : std::string());
                }
                else
                {
                    stream << buildInspectorFieldMarkup(selectedNode->id, componentIndex, field, fieldValue, fieldId == m_hoveredInspectorFieldId);
                }
            }
            stream << "</div>";
        }
        stream << "</div>";
    }

    if (serializableComponentCount == 0)
    {
        stream << "<div class='placeholder_block'><div class='placeholder_title'>Serializable components</div><div class='placeholder_text'>No serializable component descriptor is registered on this game object yet.</div></div>";
    }

    stream << "<div class='inspector_add_component_row'><div id='scene_inspector_add_component' class='panel_header_action inspector_add_component_button'>Add Component</div></div>";
    return buildInspectorShellMarkup(stream.str(), buildInspectorOverlayMarkup());
}

std::string SceneEditorController::buildInspectorOverlayMarkup() const
{
    return buildInspectorAddComponentMenuMarkup() + buildInspectorComponentContextMenuMarkup();
}

std::string SceneEditorController::buildInspectorAddComponentMenuMarkup() const
{
    if (!m_addComponentMenuOpen)
        return "";

    std::vector<const component_meta::ComponentDescriptor*> descriptors;
    for (const auto& entry : component_meta::componentDescriptorRegistry())
    {
        if (entry.second != nullptr)
            descriptors.push_back(entry.second);
    }

    std::sort(descriptors.begin(), descriptors.end(), [](const auto* lhs, const auto* rhs) {
        return lhs->displayName < rhs->displayName;
    });

    std::ostringstream stream;
    stream << "<div class='hierarchy_context_menu inspector_add_component_menu' style='left: " << m_addComponentMenuX << "px; top: " << m_addComponentMenuY << "px;'>";
    for (const component_meta::ComponentDescriptor* descriptor : descriptors)
    {
        std::ostringstream token;
        token << std::hex << std::setfill('0');
        for (unsigned char character : descriptor->typeKey)
            token << std::setw(2) << static_cast<int>(character);
        stream << "<div id='scene_add_component_" << token.str() << "' class='hierarchy_context_item'>";
        stream << escapeRmlText(descriptor->displayName);
        stream << "</div>";
    }
    stream << "</div>";
    return stream.str();
}

std::string SceneEditorController::buildAssetBrowserMarkup() const
{
    const bool showAssetBrowser = m_bottomPanelTab == BottomPanelTab::AssetBrowser;

    UI::TabPanel panel(0, 0);
    panel.setContentWithoutPadding(true);

    UI::TabHeader header(0, 0);

    UI::Container tabsWrapper(0, 0, UI::HORIZONTAL);
    tabsWrapper.addClassName("panel_tabs");

    UI::TabButton assetBrowserButton(0, 0, "Asset Browser");
    assetBrowserButton.setDomIdOverride("scene_bottom_tab_asset_browser");
    assetBrowserButton.setActive(showAssetBrowser);

    UI::TabButton consoleButton(0, 0, "Console");
    consoleButton.setDomIdOverride("scene_bottom_tab_console");
    consoleButton.setActive(!showAssetBrowser);

    tabsWrapper.addChild(&assetBrowserButton);
    tabsWrapper.addChild(&consoleButton);
    header.addChild(&tabsWrapper);

    UI::PanelAction clearAction(0, 0, "Clear");
    if (!showAssetBrowser)
    {
        clearAction.setDomIdOverride("scene_console_clear");
        header.addChild(&clearAction);
    }

    UI::TabItem assetBrowserTab(0, 0, "Asset Browser");
    UI::MarkupBlock assetBrowserContent(0, 0);
    assetBrowserContent.setMarkup(
        std::string("<div id='scene_asset_browser_workspace' class='asset_browser_workspace'>") +
        buildAssetBrowserWorkspaceMarkup() +
        "</div>");
    assetBrowserTab.addChild(&assetBrowserContent);

    UI::TabItem consoleTab(0, 0, "Console");
    UI::MarkupBlock consoleContent(0, 0, buildConsoleMarkup());
    consoleTab.addChild(&consoleContent);

    panel.setTabHeader(&header);
    panel.addTab(&assetBrowserTab);
    panel.addTab(&consoleTab);
    panel.setActiveTabIndex(showAssetBrowser ? 0U : 1U);
    return panel.getRML();
}

std::string SceneEditorController::buildAssetBrowserWorkspaceMarkup() const
{
    std::ostringstream stream;
    stream << "<div id='scene_asset_browser_tree_pane' class='asset_browser_pane asset_browser_tree_pane'>";
    stream << buildAssetBrowserTreePaneMarkup();
    stream << "</div>";

    stream << "<div id='scene_asset_browser_splitter' class='splitter splitter_vertical_nested'></div>";

    stream << "<div id='scene_asset_browser_files_pane' class='asset_browser_pane asset_browser_files_pane'>";
    stream << buildAssetBrowserFilesPaneMarkup();
    stream << "</div>";

    stream << "<div id='scene_asset_browser_overlay' class='asset_browser_overlay'>";
    stream << buildAssetBrowserOverlayMarkup();
    stream << "</div>";
    return stream.str();
}

std::string SceneEditorController::buildAssetBrowserTreePaneMarkup() const
{
    std::ostringstream stream;
    stream << "<div class='asset_browser_section_header'>Folders</div><div class='asset_browser_section_body asset_browser_tree_body'>";
    for (const AssetBrowserDirectoryNode& root : m_assetBrowserModel.roots())
        stream << buildAssetBrowserDirectoryMarkup(root, 0);
    stream << "</div>";
    return stream.str();
}

std::string SceneEditorController::buildAssetBrowserFilesPaneMarkup() const
{
    const AssetBrowserDirectoryNode* selectedDirectory = m_assetBrowserModel.selectedDirectory();

    std::ostringstream stream;
    stream << "<div class='asset_browser_section_header'>Files";
    if (selectedDirectory != nullptr)
        stream << "<span class='asset_browser_section_path'>" << escapeRmlText(selectedDirectory->runtimePath) << "</span>";
    stream << "</div><div class='asset_browser_section_body asset_browser_files_body'>";
    stream << buildAssetBrowserFileGridMarkup(selectedDirectory);
    stream << "</div>";
    return stream.str();
}

std::string SceneEditorController::buildAssetBrowserOverlayMarkup() const
{
    return buildAssetBrowserContextMenuMarkup();
}

std::string SceneEditorController::buildInspectorComponentContextMenuMarkup() const
{
    if (!m_inspectorComponentContextMenuOpen)
        return "";

    std::ostringstream stream;
    stream << "<div class='hierarchy_context_menu inspector_component_context_menu' style='left: " << m_inspectorComponentContextMenuX << "px; top: " << m_inspectorComponentContextMenuY << "px;'>";
    stream << "<div id='scene_inspector_component_delete' class='hierarchy_context_item danger'>Delete component</div>";
    stream << "</div>";
    return stream.str();
}

std::string SceneEditorController::buildConsoleMarkup() const
{
    std::ostringstream stream;
    stream << "<div class='console_workspace'>";

    if (m_consoleRawLines.empty())
    {
        stream << "<div class='console_output'><div class='placeholder_block'><div class='placeholder_title'>Console</div><div class='placeholder_text'>Build output and player logs will appear here.</div></div></div>";
    }
    else
    {
        stream << "<div id='scene_console_output' class='console_output console_output_rich'>";
        for (size_t index = 0; index < m_consoleLines.size(); ++index)
            stream << "<div class='console_line'>" << m_consoleLines[index] << "</div>";
        stream << "<div id='scene_console_output_spacer' class='console_output_spacer'></div>";
        stream << "</div>";
        stream << "<textarea id='scene_console_selection' class='console_output_text' wrap='nowrap'>";
        for (size_t index = 0; index < m_consoleRawLines.size(); ++index)
        {
            if (index > 0)
                stream << '\n';
            stream << escapeTextAreaValue(m_consoleRawLines[index]);
        }
        stream << "</textarea>";
    }

    stream << "</div>";
    return stream.str();
}

std::string SceneEditorController::buildAssetBrowserDirectoryMarkup(const AssetBrowserDirectoryNode& node, int depth) const
{
    const bool expanded = m_assetBrowserModel.isDirectoryExpanded(node);
    const bool selected = node.id == m_assetBrowserModel.selectedDirectoryId();

    std::ostringstream stream;
    stream << "<div class='asset_browser_tree_node depth_" << depth << "'>";
    stream << "<div id='" << UI::SceneEditorDomIdCodec::makeAssetDirectoryElementId(node.id) << "' class='asset_browser_tree_row ";
    stream << assetBrowserRootClass(node.rootKind);
    if (selected)
        stream << " selected";
    if (expanded)
        stream << " expanded";
    stream << "'>";
    stream << "<div id='" << UI::SceneEditorDomIdCodec::makeAssetDirectoryToggleElementId(node.id) << "' class='asset_browser_tree_toggle'>" << (node.children.empty() ? "-" : (expanded ? "v" : ">")) << "</div>";
    stream << "<div class='asset_browser_tree_label'>" << escapeRmlText(node.label) << "</div>";
    stream << "<div class='asset_browser_tree_meta'>" << escapeRmlText(assetBrowserRootLabel(node.rootKind)) << "</div>";
    stream << "</div>";

    if (expanded)
    {
        stream << "<div class='asset_browser_tree_children'>";
        for (const AssetBrowserDirectoryNode& child : node.children)
            stream << buildAssetBrowserDirectoryMarkup(child, depth + 1);
        stream << "</div>";
    }

    stream << "</div>";
    return stream.str();
}

std::string SceneEditorController::buildAssetBrowserFileGridMarkup(const AssetBrowserDirectoryNode* directory) const
{
    if (directory == nullptr)
    {
        return R"RML(<div class='placeholder_block'><div class='placeholder_title'>Asset browser</div><div class='placeholder_text'>Select a directory from Assets or built-in.</div></div>)RML";
    }

    if (directory->files.empty())
    {
        return R"RML(<div class='placeholder_block'><div class='placeholder_title'>No files in this folder</div><div class='placeholder_text'>Directories are listed in the tree on the right. This grid is ready for typed drag and drop payloads.</div></div>)RML";
    }

    std::ostringstream stream;
    stream << "<div class='asset_browser_file_grid'>";
    for (const AssetBrowserFileEntry& file : directory->files)
    {
        stream << "<div id='" << UI::SceneEditorDomIdCodec::makeAssetFileElementId(file.id) << "' class='asset_browser_file_card ";
        stream << assetBrowserRootClass(file.rootKind) << " " << assetBrowserFileKindClass(file.fileKind);
        if (file.id == m_assetBrowserModel.selectedFileId())
            stream << " selected";
        if (file.id == m_draggedAssetFileId)
            stream << " dragging";
        stream << "'>";
        stream << "<div class='asset_browser_file_badge'>" << escapeRmlText(assetBrowserFileKindLabel(file.fileKind)) << "</div>";
        stream << "<div class='asset_browser_file_label'>" << escapeRmlText(file.label) << "</div>";
        stream << "<div class='asset_browser_file_meta'>" << escapeRmlText(file.runtimePath) << "</div>";
        stream << "</div>";
    }
    stream << "</div>";
    return stream.str();
}

std::string SceneEditorController::buildAssetBrowserContextMenuMarkup() const
{
    if (!m_assetBrowserContextMenuOpen)
        return "";

    std::ostringstream stream;
    stream << "<div class='hierarchy_context_menu asset_browser_context_menu' style='left: " << m_assetBrowserContextMenuX << "px; top: " << m_assetBrowserContextMenuY << "px;'>";
    stream << "<div id='scene_asset_browser_refresh' class='hierarchy_context_item'>Refresh</div>";
    stream << "</div>";
    return stream.str();
}

std::string SceneEditorController::buildViewportMarkup() const
{
    const bool buildRunning = m_activeProcessKind == ActiveProcessKind::Build;
    const bool previewPlayerRunning = m_activeProcessKind == ActiveProcessKind::Player;

    UI::Container shell(0, 0, UI::VERTICAL);
    shell.addClassName("scene_viewport_shell");

    UI::Container toolbar(0, 0, UI::HORIZONTAL);
    toolbar.addClassName("scene_viewport_toolbar");

    UI::ToolbarGroup actions(0, 0);
    UI::ToolbarButton stopButton(0, 0, "Stop");
    stopButton.setDomIdOverride("scene_stop_button");
    UI::ToolbarButton pauseButton(0, 0, "Pause");
    pauseButton.setDomIdOverride("scene_pause_button");
    UI::ToolbarButton resumeButton(0, 0, "Resume");
    resumeButton.setDomIdOverride("scene_play_button");
    UI::ToolbarButton playButton(0, 0, "Play");
    playButton.setDomIdOverride("scene_play_button");

    if (buildRunning)
    {
        actions.addChild(&stopButton);
        toolbar.addChild(&actions);
    }
    else if (previewPlayerRunning && m_playbackState == PlaybackState::Playing)
    {
        actions.addChild(&pauseButton);
        actions.addChild(&stopButton);
        toolbar.addChild(&actions);
    }
    else if (previewPlayerRunning && m_playbackState == PlaybackState::Paused)
    {
        actions.addChild(&resumeButton);
        actions.addChild(&stopButton);
        toolbar.addChild(&actions);
    }
    else
    {
        actions.addChild(&playButton);
        toolbar.addChild(&actions);
    }

    UI::ToolbarGroup statusGroup(0, 0);
    UI::TextBlock documentStatus(0, 0, m_currentSceneFilePath.empty() ? std::string("Untitled scene") : m_currentSceneFilePath);
    documentStatus.addClassName("scene_document_status");
    UI::TextBlock playbackStatus(0, 0, buildPlaybackStatusText());
    playbackStatus.setDomIdOverride("scene_playback_status");
    playbackStatus.addClassName("scene_playback_status");
    statusGroup.addChild(&documentStatus);
    statusGroup.addChild(&playbackStatus);
    toolbar.addChild(&statusGroup);

    UI::Container viewportSurface(0, 0, UI::VERTICAL);
    viewportSurface.setDomIdOverride("scene_viewport_surface");
    viewportSurface.addClassName("scene_viewport_surface");

    UI::MarkupBlock dirtyPrompt(0, 0, buildSceneDirtyPromptMarkup());

    shell.addChild(&toolbar);
    shell.addChild(&viewportSurface);
    shell.addChild(&dirtyPrompt);
    return shell.getRML();
}

std::string SceneEditorController::buildSceneDirtyPromptMarkup() const
{
    if (!m_sceneSavePromptOpen)
        return "";

    std::ostringstream stream;
    stream << "<div class='scene_modal_overlay'><div class='scene_modal_card'>";
    stream << "<div class='scene_modal_title'>Unsaved scene changes</div>";
    stream << "<div class='scene_modal_text'>Save the current scene before replacing it?</div>";
    stream << "<div class='scene_modal_actions'>";
    stream << "<div id='scene_dirty_prompt_save' class='scene_modal_button primary'>Save</div>";
    stream << "<div id='scene_dirty_prompt_discard' class='scene_modal_button danger'>Discard</div>";
    stream << "<div id='scene_dirty_prompt_cancel' class='scene_modal_button'>Cancel</div>";
    stream << "</div></div></div>";
    return stream.str();
}

std::string SceneEditorController::makeTransformFieldElementId(int nodeId, const std::string& fieldKey)
{
    return makeSceneTransformFieldElementId(nodeId, fieldKey);
}

std::string SceneEditorController::makeSceneFieldElementId(const std::string& fieldKey)
{
    return makeSceneRootFieldElementId(fieldKey);
}

std::string SceneEditorController::makeInspectorFieldElementId(int nodeId, size_t componentIndex, const std::string& fieldKey)
{
    return makeSceneInspectorFieldElementId(nodeId, componentIndex, fieldKey);
}

std::string SceneEditorController::makeInspectorGroupElementId(int nodeId, size_t componentIndex)
{
    return makeSceneInspectorGroupElementId(nodeId, componentIndex);
}

std::optional<SceneEditorController::InspectorGroupBinding> SceneEditorController::parseInspectorGroupElementId(const Rml::String& elementId)
{
    const std::string value = elementId;
    const std::string prefix = "scene_inspector_group_";
    if (!startsWith(value, prefix))
        return std::nullopt;

    const size_t separator = value.find("__", prefix.size());
    if (separator == std::string::npos)
        return std::nullopt;

    InspectorGroupBinding binding;
    binding.nodeId = std::stoi(value.substr(prefix.size(), separator - prefix.size()));
    binding.componentIndex = static_cast<size_t>(std::stoul(value.substr(separator + 2)));
    return binding;
}

std::optional<SceneEditorController::InspectorFieldBinding> SceneEditorController::parseInspectorFieldElementId(const Rml::String& elementId)
{
    const std::string value = elementId;
    const std::string scenePrefix = "scene_root_field__";
    if (startsWith(value, scenePrefix))
    {
        InspectorFieldBinding binding;
        binding.target = InspectorFieldBinding::Target::Scene;
        binding.nodeId = 0;
        binding.fieldKey = value.substr(scenePrefix.size());
        return binding;
    }

    const std::string transformPrefix = "scene_transform_field_";
    if (startsWith(value, transformPrefix))
    {
        const size_t separator = value.find("__", transformPrefix.size());
        if (separator == std::string::npos)
            return std::nullopt;

        InspectorFieldBinding binding;
        binding.target = InspectorFieldBinding::Target::Transform;
        binding.nodeId = std::stoi(value.substr(transformPrefix.size(), separator - transformPrefix.size()));
        binding.fieldKey = value.substr(separator + 2);
        return binding;
    }

    const std::string prefix = "scene_inspector_field_";
    if (!startsWith(value, prefix))
        return std::nullopt;

    const size_t firstSeparator = value.find("__", prefix.size());
    if (firstSeparator == std::string::npos)
        return std::nullopt;

    const size_t secondSeparator = value.find("__", firstSeparator + 2);
    if (secondSeparator == std::string::npos)
        return std::nullopt;

    InspectorFieldBinding binding;
    binding.target = InspectorFieldBinding::Target::Component;
    binding.nodeId = std::stoi(value.substr(prefix.size(), firstSeparator - prefix.size()));
    binding.componentIndex = static_cast<size_t>(std::stoul(value.substr(firstSeparator + 2, secondSeparator - (firstSeparator + 2))));
    binding.fieldKey = value.substr(secondSeparator + 2);
    return binding;
}
