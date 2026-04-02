#include "SceneEditorController.hpp"

#include "EditorUiDocuments.hpp"

#include <common/platform/NativeFileDialog.hpp>
#include <common/Scene.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <limits>
#include <sstream>
#include <system_error>
#include <unordered_set>

using namespace editor_ui;

namespace
{
constexpr int MinAssetBrowserPaneWidth = 160;

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

std::string formatFloat(float value)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(3) << value;
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

const char* assetBrowserRootLabel(SceneEditorController::AssetBrowserRootKind rootKind)
{
    return rootKind == SceneEditorController::AssetBrowserRootKind::Assets ? "Assets" : "built-in";
}

const char* assetBrowserRootClass(SceneEditorController::AssetBrowserRootKind rootKind)
{
    return rootKind == SceneEditorController::AssetBrowserRootKind::Assets ? "user" : "builtin";
}

SceneEditorController::AssetBrowserFileKind classifyAssetBrowserFileKind(const std::filesystem::path& path)
{
    const std::string extension = path.extension().string();
    if (extension == ".mat")
        return SceneEditorController::AssetBrowserFileKind::Material;
    if (extension == ".obj" || extension == ".off")
        return SceneEditorController::AssetBrowserFileKind::Mesh;
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
    case SceneEditorController::AssetBrowserFileKind::Mesh:
        return DragPayloadKind::MeshAsset;
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
    case SceneEditorController::AssetBrowserFileKind::Mesh:
        return "Mesh";
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
    case SceneEditorController::AssetBrowserFileKind::Mesh:
        return "mesh";
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
    m_expandedAssetDirectoryIds.insert("Assets");
    m_expandedAssetDirectoryIds.insert("built-in");
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

    rescanAssetBrowser();
    attachListeners();
    refreshPresentation();
    m_document->Show();
    applyLayout();
    refreshCachedRects();
}

void SceneEditorController::deactivate()
{
    detachListeners();

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
    m_viewportRect = {};
    m_centerRect = {};
    m_isFileMenuOpen = false;
    m_runtimeSceneSnapshot.reset();
    m_assetBrowserContextMenuOpen = false;
    m_hierarchyContextMenuOpen = false;
    m_addComponentMenuOpen = false;
    m_sceneSavePromptOpen = false;
    m_pendingSceneAction = PendingSceneAction::None;
    m_pendingSceneTargetPath.clear();
    m_hoveredInspectorFieldId.clear();
}

void SceneEditorController::setModeChangeCallback(const std::function<void(EditorMode)>& callback)
{
    m_modeChangeCallback = callback;
}

void SceneEditorController::syncToWindow(int width, int height)
{
    m_windowWidth = std::max(width, 1);
    m_windowHeight = std::max(height, 1);
}

void SceneEditorController::sync(Scene& scene)
{
    m_scene = &scene;
    const UiGOHierarchyNode previousHierarchy = m_hierarchyRoot;
    const int previousSelectedHierarchyNodeId = m_selectedHierarchyNodeId;

    rebuildHierarchyFromScene(scene);

    if (const GameObject* selectedGameObject = scene.getSelectedGameObject())
    {
        if (const UiGOHierarchyNode* selectedNode = findHierarchyNodeByGameObject(selectedGameObject))
            m_selectedHierarchyNodeId = selectedNode->id;
    }
    else
    {
        m_selectedHierarchyNodeId = m_hierarchyRoot.id;
    }

    if (findHierarchyNodeById(m_selectedHierarchyNodeId) == nullptr)
        m_selectedHierarchyNodeId = m_hierarchyRoot.id;

    if (!hierarchyNodesEqual(previousHierarchy, m_hierarchyRoot) || previousSelectedHierarchyNodeId != m_selectedHierarchyNodeId)
        requestHierarchyRefresh();
}

void SceneEditorController::setShowStylePanel(bool showStylePanel)
{
    (void)showStylePanel;
}

void SceneEditorController::update()
{
    if (m_context == nullptr || m_document == nullptr)
        return;

    if (m_hierarchyRefreshPending)
    {
        refreshPresentation();
        m_hierarchyRefreshPending = false;
    }
    else if (shouldRefreshInspectorPresentation())
    {
        refreshInspectorValuesPresentation();
    }

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
    return m_viewportRect;
}

bool SceneEditorController::isViewportHovered(double mouseX, double mouseY) const
{
    return m_viewportRect.contains(mouseX, mouseY);
}

bool SceneEditorController::isDragging() const
{
    return m_dragTarget != DragTarget::None;
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
    const Rml::String fileMenuButtonElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_menu_file_button"; });
    const Rml::String saveSceneAsElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_menu_save_as"; });
    const Rml::String loadSceneElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "scene_menu_load_save"; });
    const Rml::String windowMenuButtonElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "builder_menu_window_button"; });
    const Rml::String openUiBuilderElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "builder_menu_open_ui_builder"; });
    const Rml::String hierarchyNodeElementId = ::findAncestorElementId(
        targetElement,
        [&](const Rml::String& candidateId) { return parseHierarchyNodeId(candidateId).has_value(); });
    const Rml::String assetDirectoryElementId = ::findAncestorElementId(
        targetElement,
        [&](const Rml::String& candidateId) { return parseAssetDirectoryElementId(candidateId).has_value(); });
    const Rml::String assetFileElementId = ::findAncestorElementId(
        targetElement,
        [&](const Rml::String& candidateId) { return parseAssetFileElementId(candidateId).has_value(); });
    Rml::Element* assetBrowserWorkspaceElement = ::findAncestorElement(
        targetElement,
        [](const Rml::Element& candidate) { return candidate.GetId() == "scene_asset_browser_workspace"; });
    const Rml::String inspectorGroupElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return startsWith(std::string(candidateId), "scene_inspector_group_"); });
    const Rml::String inspectorFieldElementId = ::findAncestorElementId(
        targetElement,
        [&](const Rml::String& candidateId) { return parseInspectorFieldElementId(candidateId).has_value(); });
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
                requestHierarchyRefresh();
                event.StopPropagation();
                return;
            }

            if (!promptCancelElementId.empty())
            {
                closePendingSceneActionPrompt();
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
            markSceneDirty();
            refreshInspectorValuesPresentation();
        }

        event.StopPropagation();
        return;
    }

    if (eventId == Rml::EventId::Click)
    {
        if (!playButtonElementId.empty())
        {
            if (m_scene != nullptr && m_playbackState == PlaybackState::Stopped)
                m_runtimeSceneSnapshot = scene_serialization::captureScene(*m_scene);
            if (m_scene != nullptr)
                m_scene->setPhysicsSimulationEnabled(true);
            m_playbackState = PlaybackState::Playing;
            refreshPresentation();
            event.StopPropagation();
            return;
        }

        if (!pauseButtonElementId.empty())
        {
            if (m_scene != nullptr)
                m_scene->setPhysicsSimulationEnabled(false);
            m_playbackState = PlaybackState::Paused;
            refreshPresentation();
            event.StopPropagation();
            return;
        }

        if (!stopButtonElementId.empty())
        {
            if (m_scene != nullptr)
            {
                m_scene->setPhysicsSimulationEnabled(false);
                if (m_runtimeSceneSnapshot.has_value())
                {
                    scene_serialization::applySceneSnapshot(*m_scene, *m_runtimeSceneSnapshot);
                    sync(*m_scene);
                    m_runtimeSceneSnapshot.reset();
                }
            }
            m_playbackState = PlaybackState::Stopped;
            refreshPresentation();
            event.StopPropagation();
            return;
        }

        if (!fileMenuButtonElementId.empty())
        {
            m_isFileMenuOpen = !m_isFileMenuOpen;
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
                const UiGOHierarchyNode* node = findHierarchyNodeById(m_hierarchyContextMenuNodeId);
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
                m_addComponentMenuX = static_cast<int>(std::lround(mouseX - m_rightPanel->GetAbsoluteLeft())) - 12;
                m_addComponentMenuY = static_cast<int>(std::lround(mouseY - m_rightPanel->GetAbsoluteTop())) + 8;
            }

            requestHierarchyRefresh();
            event.StopPropagation();
            return;
        }

        if (!addComponentItemElementId.empty())
        {
            const std::string prefix = "scene_add_component_";
            const std::optional<std::string> typeKey = decodeElementToken(std::string(addComponentItemElementId).substr(prefix.size()));
            const component_meta::ComponentDescriptor* descriptor = typeKey.has_value() ? component_meta::findComponentDescriptor(*typeKey) : nullptr;
            const UiGOHierarchyNode* selectedNode = findSelectedHierarchyNode();

            m_addComponentMenuOpen = false;
            if (descriptor != nullptr && descriptor->factory != nullptr && selectedNode != nullptr && selectedNode->gameObject != nullptr && m_scene != nullptr)
            {
                GameObject* gameObject = const_cast<GameObject*>(selectedNode->gameObject);
                if (component::Component* component = descriptor->factory(gameObject))
                {
                    gameObject->addComponent(component);
                    markSceneDirty();
                    sync(*m_scene);
                }
            }

            event.StopPropagation();
            return;
        }

        if (::findAncestorElementId(targetElement, [](const Rml::String& candidateId) { return candidateId == "scene_asset_browser_refresh"; }) == "scene_asset_browser_refresh")
        {
            m_assetBrowserContextMenuOpen = false;
            rescanAssetBrowser();
            requestHierarchyRefresh();
            event.StopPropagation();
            return;
        }

        if (!inspectorGroupElementId.empty())
        {
            toggleInspectorGroup(inspectorGroupElementId);
            refreshInspectorPresentation();
            event.StopPropagation();
            return;
        }

        if (!windowMenuButtonElementId.empty())
        {
            m_isFileMenuOpen = false;
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

        if (const std::optional<std::string> directoryId = parseAssetDirectoryElementId(assetDirectoryElementId))
        {
            m_assetBrowserContextMenuOpen = false;
            m_hierarchyContextMenuOpen = false;
            selectAssetDirectory(*directoryId);

            if (const AssetBrowserDirectoryNode* directory = findAssetDirectoryById(*directoryId))
            {
                if (!directory->children.empty())
                    toggleAssetDirectoryExpansion(*directoryId);
            }

            requestHierarchyRefresh();
            event.StopPropagation();
            return;
        }

        if (const std::optional<std::string> fileId = parseAssetFileElementId(assetFileElementId))
        {
            m_assetBrowserContextMenuOpen = false;
            m_hierarchyContextMenuOpen = false;
            m_selectedAssetFileId = *fileId;
            event.StopPropagation();
            return;
        }

        if (const std::optional<int> hierarchyNodeId = parseHierarchyNodeId(hierarchyNodeElementId))
        {
            m_hierarchyContextMenuOpen = false;
            const UiGOHierarchyNode* hierarchyNode = findHierarchyNodeById(*hierarchyNodeId);
            if (hierarchyNode != nullptr && m_scene != nullptr)
            {
                GameObject* clickedGameObject = const_cast<GameObject*>(hierarchyNode->gameObject);
                m_scene->toggleSelectedGameObject(clickedGameObject);
                m_selectedHierarchyNodeId = (clickedGameObject != nullptr && m_scene->getSelectedGameObject() == clickedGameObject)
                    ? *hierarchyNodeId
                    : m_hierarchyRoot.id;
            }
            else
            {
                m_selectedHierarchyNodeId = *hierarchyNodeId;
            }

            requestHierarchyRefresh();
            event.StopPropagation();
            return;
        }

        if (m_isFileMenuOpen || m_isWindowMenuOpen)
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
            requestHierarchyRefresh();
        }

        if (m_hierarchyContextMenuOpen && deleteHierarchyElementId.empty() && hierarchyNodeElementId.empty())
        {
            m_hierarchyContextMenuOpen = false;
            requestHierarchyRefresh();
        }

        if (m_addComponentMenuOpen && addComponentButtonElementId.empty() && addComponentItemElementId.empty())
        {
            m_addComponentMenuOpen = false;
            requestHierarchyRefresh();
        }
        return;
    }

    if (eventId == Rml::EventId::Dblclick)
    {
        if (const std::optional<std::string> fileId = parseAssetFileElementId(assetFileElementId))
        {
            if (const AssetBrowserFileEntry* file = findAssetFileById(*fileId))
            {
                if (file->fileKind == AssetBrowserFileKind::Scene)
                {
                    beginPendingSceneAction(PendingSceneAction::OpenFile, file->diskPath);
                    event.StopPropagation();
                    return;
                }
            }
        }
    }

    if (eventId == Rml::EventId::Dragstart)
    {
        if (const std::optional<std::string> fileId = parseAssetFileElementId(assetFileElementId))
        {
            if (const AssetBrowserFileEntry* file = findAssetFileById(*fileId))
            {
                m_dragPayloadKind = file->dragPayloadKind;
                m_draggedAssetFileId = *fileId;
                m_draggedAssetRuntimePath = file->runtimePath;
                m_selectedAssetFileId = *fileId;
                event.StopPropagation();
                return;
            }
        }
    }

    if (eventId == Rml::EventId::Dragover)
    {
        const std::optional<InspectorFieldBinding> inspectorField = parseInspectorFieldElementId(inspectorFieldElementId);
        const std::string nextHoveredFieldId = (inspectorField.has_value() && canDropDraggedAssetOnInspectorField(*inspectorField))
            ? std::string(inspectorFieldElementId)
            : std::string();

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
        const std::optional<InspectorFieldBinding> inspectorField = parseInspectorFieldElementId(inspectorFieldElementId);
        if (inspectorField.has_value() && applyDraggedAssetToInspectorField(*inspectorField))
        {
            m_hoveredInspectorFieldId.clear();
            markSceneDirty();
            requestHierarchyRefresh();
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
            m_assetBrowserContextMenuOpen = true;
            m_assetBrowserContextMenuX = static_cast<int>(std::lround(mouseX - assetBrowserWorkspaceElement->GetAbsoluteLeft()));
            m_assetBrowserContextMenuY = static_cast<int>(std::lround(mouseY - assetBrowserWorkspaceElement->GetAbsoluteTop()));
            requestHierarchyRefresh();
            event.StopPropagation();
            return;
        }

        if (mouseButton == 1)
        {
            if (const std::optional<int> hierarchyNodeId = parseHierarchyNodeId(hierarchyNodeElementId))
            {
                m_assetBrowserContextMenuOpen = false;
                m_addComponentMenuOpen = false;
                m_hierarchyContextMenuOpen = true;
                m_hierarchyContextMenuNodeId = *hierarchyNodeId;
                m_hierarchyContextMenuX = static_cast<int>(std::lround(mouseX - m_leftPanel->GetAbsoluteLeft()));
                m_hierarchyContextMenuY = static_cast<int>(std::lround(mouseY - m_leftPanel->GetAbsoluteTop()));

                const UiGOHierarchyNode* hierarchyNode = findHierarchyNodeById(*hierarchyNodeId);
                if (hierarchyNode != nullptr && m_scene != nullptr)
                {
                    m_scene->setSelectedGameObject(const_cast<GameObject*>(hierarchyNode->gameObject));
                    m_selectedHierarchyNodeId = *hierarchyNodeId;
                }

                requestHierarchyRefresh();
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
                requestHierarchyRefresh();
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
        const int totalWidth = std::max(m_windowWidth, 1);
        int leftWidth = static_cast<int>(std::lround(totalWidth * m_leftRatio));
        int rightWidth = static_cast<int>(std::lround(totalWidth * m_rightRatio));
        rightWidth = clampInt(rightWidth, MinColumnWidth, totalWidth - leftWidth - MinCenterWidth - (2 * SplitterThickness));

        if (m_dragTarget == DragTarget::LeftSplitter)
        {
            const int maxLeftWidth = totalWidth - MinCenterWidth - rightWidth - (2 * SplitterThickness);
            leftWidth = clampInt(static_cast<int>(std::lround(mouseX)), MinColumnWidth, maxLeftWidth);
            m_leftRatio = static_cast<float>(leftWidth) / static_cast<float>(totalWidth);
        }
        else
        {
            leftWidth = clampInt(leftWidth, MinColumnWidth, totalWidth - MinCenterWidth - MinColumnWidth - (2 * SplitterThickness));
            rightWidth = clampInt(
                totalWidth - static_cast<int>(std::lround(mouseX)) - SplitterThickness,
                MinColumnWidth,
                totalWidth - leftWidth - MinCenterWidth - (2 * SplitterThickness));
            m_rightRatio = static_cast<float>(rightWidth) / static_cast<float>(totalWidth);
        }

        applyLayout();
        event.StopPropagation();
        return;
    }

    if (m_dragTarget == DragTarget::HorizontalSplitter && m_centerRect.isValid())
    {
        const int localY = static_cast<int>(std::lround(mouseY)) - m_centerRect.y;
        const int maxViewportHeight = m_centerRect.height - MinBottomHeight - SplitterThickness;
        const int viewportHeight = clampInt(localY, MinViewportHeight, maxViewportHeight);
        m_viewportRatio = static_cast<float>(viewportHeight) / static_cast<float>(std::max(m_centerRect.height, 1));
        applyLayout();
        event.StopPropagation();
        return;
    }

    if (m_dragTarget == DragTarget::BottomBrowserSplitter && m_bottomPanel != nullptr)
    {
        const int totalWidth = std::max(static_cast<int>(std::lround(m_bottomPanel->GetClientWidth())), 1);
        const int localX = static_cast<int>(std::lround(mouseX)) - static_cast<int>(std::lround(m_bottomPanel->GetAbsoluteLeft()));
        const int maxFilesWidth = totalWidth - MinAssetBrowserPaneWidth - SplitterThickness;
        const int filesWidth = clampInt(localX, MinAssetBrowserPaneWidth, maxFilesWidth);
        const int treeWidth = totalWidth - filesWidth - SplitterThickness;
        m_bottomBrowserTreeRatio = static_cast<float>(treeWidth) / static_cast<float>(totalWidth);
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
    if (m_root == nullptr)
        return;

    const int totalWidth = std::max(m_windowWidth, 1);
    const int totalHeight = std::max(m_windowHeight, 1);
    const int contentTop = BuilderHeaderHeight;
    const int contentHeight = std::max(1, totalHeight - contentTop);

    int leftWidth = static_cast<int>(std::lround(totalWidth * m_leftRatio));
    int rightWidth = static_cast<int>(std::lround(totalWidth * m_rightRatio));

    leftWidth = clampInt(leftWidth, MinColumnWidth, totalWidth - MinCenterWidth - MinColumnWidth - (2 * SplitterThickness));
    rightWidth = clampInt(rightWidth, MinColumnWidth, totalWidth - leftWidth - MinCenterWidth - (2 * SplitterThickness));

    int centerWidth = totalWidth - leftWidth - rightWidth - (2 * SplitterThickness);
    if (centerWidth < MinCenterWidth)
    {
        const int deficit = MinCenterWidth - centerWidth;
        if (rightWidth - deficit >= MinColumnWidth)
            rightWidth -= deficit;
        else
            leftWidth = std::max(MinColumnWidth, leftWidth - (deficit - (rightWidth - MinColumnWidth)));

        rightWidth = clampInt(rightWidth, MinColumnWidth, totalWidth - leftWidth - MinCenterWidth - (2 * SplitterThickness));
        centerWidth = totalWidth - leftWidth - rightWidth - (2 * SplitterThickness);
    }

    m_leftRatio = static_cast<float>(leftWidth) / static_cast<float>(totalWidth);
    m_rightRatio = static_cast<float>(rightWidth) / static_cast<float>(totalWidth);

    const int centerX = leftWidth + SplitterThickness;
    const int rightSplitterX = centerX + centerWidth;
    const int rightX = rightSplitterX + SplitterThickness;

    m_centerRect = {centerX, contentTop, centerWidth, contentHeight};

    m_root->SetProperty("width", pixels(totalWidth));
    m_root->SetProperty("height", pixels(totalHeight));
    m_builderHeader->SetProperty("display", "block");
    m_builderHeader->SetProperty("left", pixels(0));
    m_builderHeader->SetProperty("top", pixels(0));
    m_builderHeader->SetProperty("width", pixels(totalWidth));
    m_builderHeader->SetProperty("height", pixels(BuilderHeaderHeight));

    m_leftPanel->SetProperty("left", pixels(0));
    m_leftPanel->SetProperty("top", pixels(contentTop));
    m_leftPanel->SetProperty("width", pixels(leftWidth));
    m_leftPanel->SetProperty("height", pixels(contentHeight));

    m_leftSplitter->SetProperty("left", pixels(leftWidth));
    m_leftSplitter->SetProperty("top", pixels(contentTop));
    m_leftSplitter->SetProperty("width", pixels(SplitterThickness));
    m_leftSplitter->SetProperty("height", pixels(contentHeight));

    m_centerPanel->SetProperty("left", pixels(centerX));
    m_centerPanel->SetProperty("top", pixels(contentTop));
    m_centerPanel->SetProperty("width", pixels(centerWidth));
    m_centerPanel->SetProperty("height", pixels(contentHeight));

    m_rightSplitter->SetProperty("left", pixels(rightSplitterX));
    m_rightSplitter->SetProperty("top", pixels(contentTop));
    m_rightSplitter->SetProperty("width", pixels(SplitterThickness));
    m_rightSplitter->SetProperty("height", pixels(contentHeight));

    m_rightPanel->SetProperty("left", pixels(rightX));
    m_rightPanel->SetProperty("top", pixels(contentTop));
    m_rightPanel->SetProperty("width", pixels(rightWidth));
    m_rightPanel->SetProperty("height", pixels(contentHeight));

    const int viewportHeight = clampInt(
        static_cast<int>(std::lround(static_cast<float>(contentHeight) * m_viewportRatio)),
        MinViewportHeight,
        contentHeight - MinBottomHeight - SplitterThickness);
    const int bottomHeight = contentHeight - viewportHeight - SplitterThickness;

    m_viewportPanel->SetProperty("display", "block");
    m_viewportPanel->SetProperty("left", pixels(0));
    m_viewportPanel->SetProperty("top", pixels(0));
    m_viewportPanel->SetProperty("width", pixels(centerWidth));
    m_viewportPanel->SetProperty("height", pixels(viewportHeight));

    m_horizontalSplitter->SetProperty("display", "block");
    m_horizontalSplitter->SetProperty("left", pixels(0));
    m_horizontalSplitter->SetProperty("top", pixels(viewportHeight));
    m_horizontalSplitter->SetProperty("width", pixels(centerWidth));
    m_horizontalSplitter->SetProperty("height", pixels(SplitterThickness));

    m_bottomPanel->SetProperty("display", "block");
    m_bottomPanel->SetProperty("left", pixels(0));
    m_bottomPanel->SetProperty("top", pixels(viewportHeight + SplitterThickness));
    m_bottomPanel->SetProperty("width", pixels(centerWidth));
    m_bottomPanel->SetProperty("height", pixels(bottomHeight));

    if (m_bottomBrowserFilesPane != nullptr && m_bottomBrowserTreePane != nullptr && m_bottomBrowserSplitter != nullptr)
    {
        const int totalBrowserWidth = std::max(centerWidth, 1);
        const int treeWidth = clampInt(
            static_cast<int>(std::lround(static_cast<float>(totalBrowserWidth) * m_bottomBrowserTreeRatio)),
            MinAssetBrowserPaneWidth,
            totalBrowserWidth - MinAssetBrowserPaneWidth - SplitterThickness);
        const int filesWidth = totalBrowserWidth - treeWidth - SplitterThickness;

        m_bottomBrowserFilesPane->SetProperty("width", pixels(filesWidth));
        m_bottomBrowserTreePane->SetProperty("width", pixels(treeWidth));
        m_bottomBrowserSplitter->SetProperty("width", pixels(SplitterThickness));
    }
}

void SceneEditorController::refreshPresentation()
{
    if (m_builderHeader == nullptr || m_leftPanel == nullptr || m_rightPanel == nullptr || m_bottomPanel == nullptr)
        return;

    std::ostringstream headerStream;
    headerStream << "<div id='scene_editor_menu_bar' class='builder_menu_bar'>";
    headerStream << "<div id='scene_menu_file' class='builder_menu'><div id='scene_menu_file_button' class='builder_menu_button'>File</div>";
    headerStream << "<div id='scene_menu_file_dropdown' class='builder_menu_dropdown' style='display: " << (m_isFileMenuOpen ? "block" : "none") << ";'>";
    headerStream << "<div id='scene_menu_save_as' class='builder_menu_item'>Save Scene As</div>";
    headerStream << "<div id='scene_menu_load_save' class='builder_menu_item'>Load Save</div>";
    headerStream << "</div></div>";
    headerStream << "<div id='builder_menu_window' class='builder_menu'><div id='builder_menu_window_button' class='builder_menu_button'>Window</div>";
    headerStream << "<div id='builder_menu_window_dropdown' class='builder_menu_dropdown' style='display: " << (m_isWindowMenuOpen ? "block" : "none") << ";'>";
    headerStream << "<div id='builder_menu_open_ui_builder' class='builder_menu_item'>UI Builder</div>";
    headerStream << "</div></div></div>";
    m_builderHeader->SetInnerRML(headerStream.str());

    m_leftPanel->SetInnerRML(
        buildHierarchyMarkup()
    );
    m_viewportPanel->SetInnerRML(buildViewportMarkup());
    m_viewportSurface = m_document->GetElementById("scene_viewport_surface");
    m_rightPanel->SetInnerRML(
        buildInspectorMarkup()
    );
    m_bottomPanel->SetInnerRML(buildAssetBrowserMarkup());
    m_bottomBrowserFilesPane = m_document->GetElementById("scene_asset_browser_files_pane");
    m_bottomBrowserSplitter = m_document->GetElementById("scene_asset_browser_splitter");
    m_bottomBrowserTreePane = m_document->GetElementById("scene_asset_browser_tree_pane");
}

void SceneEditorController::refreshInspectorPresentation()
{
    if (m_rightPanel == nullptr)
        return;

    m_rightPanel->SetInnerRML(buildInspectorMarkup());
}

void SceneEditorController::refreshInspectorValuesPresentation()
{
    const UiGOHierarchyNode* selectedNode = findSelectedHierarchyNode();
    if (m_document == nullptr || selectedNode == nullptr || selectedNode->gameObject == nullptr)
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
        childrenValue->SetInnerRML(std::to_string(selectedNode->children.size()));

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
        }
    }
}

void SceneEditorController::refreshCachedRects()
{
    if (m_viewportPanel == nullptr || m_centerPanel == nullptr)
        return;

    Rml::Element* viewportTarget = m_viewportSurface != nullptr ? m_viewportSurface : m_viewportPanel;
    m_viewportRect.x = static_cast<int>(std::lround(viewportTarget->GetAbsoluteLeft() + viewportTarget->GetClientLeft()));
    m_viewportRect.y = static_cast<int>(std::lround(viewportTarget->GetAbsoluteTop() + viewportTarget->GetClientTop()));
    m_viewportRect.width = static_cast<int>(std::lround(viewportTarget->GetClientWidth()));
    m_viewportRect.height = static_cast<int>(std::lround(viewportTarget->GetClientHeight()));

    m_centerRect.x = static_cast<int>(std::lround(m_centerPanel->GetAbsoluteLeft() + m_centerPanel->GetClientLeft()));
    m_centerRect.y = static_cast<int>(std::lround(m_centerPanel->GetAbsoluteTop() + m_centerPanel->GetClientTop()));
    m_centerRect.width = static_cast<int>(std::lround(m_centerPanel->GetClientWidth()));
    m_centerRect.height = static_cast<int>(std::lround(m_centerPanel->GetClientHeight()));
}


std::string SceneEditorController::buildHierarchyMarkup() const
{
    std::ostringstream stream;
    stream << "<div class='panel_shell hierarchy_shell'><div class='panel_header panel_header_with_action'><div>Scene</div><div id='scene_hierarchy_add' class='panel_header_action'>+</div></div><div class='panel_body hierarchy_body'>";

    if (m_hierarchyRoot.children.empty())
    {
        stream << "<div class='placeholder_block'><div class='placeholder_title'>Scene hierarchy</div><div class='placeholder_text'>No game objects found in the scene.</div></div>";
    }
    else
    {
        for (const UiGOHierarchyNode& child : m_hierarchyRoot.children)
            stream << buildHierarchyNodeMarkup(child, 0);
    }

    stream << buildHierarchyContextMenuMarkup();
    stream << "</div></div>";
    return stream.str();
}

std::string SceneEditorController::buildHierarchyContextMenuMarkup() const
{
    if (!m_hierarchyContextMenuOpen || m_hierarchyContextMenuNodeId == m_hierarchyRoot.id)
        return "";

    std::ostringstream stream;
    stream << "<div class='hierarchy_context_menu' style='left: " << m_hierarchyContextMenuX << "px; top: " << m_hierarchyContextMenuY << "px;'>";
    stream << "<div id='scene_hierarchy_delete' class='hierarchy_context_item danger'>Delete</div>";
    stream << "</div>";
    return stream.str();
}

std::string SceneEditorController::buildHierarchyNodeMarkup(const UiGOHierarchyNode& node, int depth) const
{
    const bool isSelected = node.id == m_selectedHierarchyNodeId;

    std::ostringstream stream;
    stream << "<div class='hierarchy_node depth_" << depth << "'>";
    stream << "<div id='" << makeHierarchyNodeElementId(node.id) << "' class='hierarchy_row scene_hierarchy_row";
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
    const UiGOHierarchyNode* selectedNode = findSelectedHierarchyNode();
    if (selectedNode == nullptr || selectedNode->gameObject == nullptr)
    {
        return R"RML(<div class='panel_shell'><div class='panel_header'>Inspector</div><div class='panel_body'><div class='placeholder_block'><div class='placeholder_title'>Inspector</div><div class='placeholder_text'>Select a game object in the scene hierarchy.</div></div></div></div>)RML";
    }

    const glm::vec3& position = selectedNode->gameObject->transform.getPosition();
    const glm::vec3& rotation = selectedNode->gameObject->transform.getRotation();
    const glm::vec3& scale = selectedNode->gameObject->transform.getScale();

    std::ostringstream stream;
    stream << "<div class='panel_shell'><div class='panel_header'>Inspector</div><div class='panel_body inspector_panel_body'>";
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
                stream << buildInspectorFieldMarkup(selectedNode->id, componentIndex, field, field.read(*component), fieldId == m_hoveredInspectorFieldId);
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
    stream << buildInspectorAddComponentMenuMarkup();

    stream << "</div></div>";
    return stream.str();
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
        stream << "<div id='scene_add_component_" << encodeElementToken(descriptor->typeKey) << "' class='hierarchy_context_item'>";
        stream << escapeRmlText(descriptor->displayName);
        stream << "</div>";
    }
    stream << "</div>";
    return stream.str();
}

std::string SceneEditorController::buildAssetBrowserMarkup() const
{
    const AssetBrowserDirectoryNode* selectedDirectory = findSelectedAssetDirectory();

    std::ostringstream stream;
    stream << "<div class='panel_shell'><div class='panel_header'>Asset Browser</div><div class='panel_body panel_body_no_padding'>";
    stream << "<div id='scene_asset_browser_workspace' class='asset_browser_workspace'>";

    stream << "<div id='scene_asset_browser_files_pane' class='asset_browser_pane asset_browser_files_pane'>";
    stream << "<div class='asset_browser_section_header'>Files";
    if (selectedDirectory != nullptr)
        stream << "<span class='asset_browser_section_path'>" << escapeRmlText(selectedDirectory->runtimePath) << "</span>";
    stream << "</div><div class='asset_browser_section_body asset_browser_files_body'>";
    stream << buildAssetBrowserFileGridMarkup(selectedDirectory);
    stream << "</div></div>";

    stream << "<div id='scene_asset_browser_splitter' class='splitter splitter_vertical_nested'></div>";

    stream << "<div id='scene_asset_browser_tree_pane' class='asset_browser_pane asset_browser_tree_pane'>";
    stream << "<div class='asset_browser_section_header'>Folders</div><div class='asset_browser_section_body asset_browser_tree_body'>";
    for (const AssetBrowserDirectoryNode& root : m_assetBrowserRoots)
        stream << buildAssetBrowserDirectoryMarkup(root, 0);
    stream << "</div></div>";

    stream << buildAssetBrowserContextMenuMarkup();
    stream << "</div></div></div>";
    return stream.str();
}

std::string SceneEditorController::buildAssetBrowserDirectoryMarkup(const AssetBrowserDirectoryNode& node, int depth) const
{
    const bool expanded = isAssetDirectoryExpanded(node);
    const bool selected = node.id == m_selectedAssetDirectoryId;

    std::ostringstream stream;
    stream << "<div class='asset_browser_tree_node depth_" << depth << "'>";
    stream << "<div id='" << makeAssetDirectoryElementId(node.id) << "' class='asset_browser_tree_row ";
    stream << assetBrowserRootClass(node.rootKind);
    if (selected)
        stream << " selected";
    if (expanded)
        stream << " expanded";
    stream << "'>";
    stream << "<div class='asset_browser_tree_toggle'>" << (node.children.empty() ? "-" : (expanded ? "v" : ">")) << "</div>";
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
        stream << "<div id='" << makeAssetFileElementId(file.id) << "' class='asset_browser_file_card ";
        stream << assetBrowserRootClass(file.rootKind) << " " << assetBrowserFileKindClass(file.fileKind);
        if (file.id == m_selectedAssetFileId)
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
    const bool physicsRunning = m_playbackState == PlaybackState::Playing;

    std::ostringstream stream;
    stream << "<div class='scene_viewport_shell'>";
    stream << "<div class='scene_viewport_toolbar'><div class='preview_toolbar_group'>";

    if (m_playbackState == PlaybackState::Playing)
    {
        stream << "<div id='scene_pause_button' class='preview_toolbar_button'>Pause</div>";
        stream << "<div id='scene_stop_button' class='preview_toolbar_button'>Stop</div>";
    }
    else if (m_playbackState == PlaybackState::Paused)
    {
        stream << "<div id='scene_play_button' class='preview_toolbar_button'>Resume</div>";
        stream << "<div id='scene_stop_button' class='preview_toolbar_button'>Stop</div>";
    }
    else
    {
        stream << "<div id='scene_play_button' class='preview_toolbar_button'>Play</div>";
    }

    stream << "</div><div class='preview_toolbar_group'>";
    stream << "<div class='scene_document_status'>";
    if (m_currentSceneFilePath.empty())
        stream << "Untitled scene";
    else
        stream << escapeRmlText(m_currentSceneFilePath);
    if (m_sceneDirty)
        stream << " *";
    stream << "</div>";
    if (m_playbackState == PlaybackState::Playing)
        stream << "<div class='scene_playback_status'>Physics running</div>";
    else if (m_playbackState == PlaybackState::Paused)
        stream << "<div class='scene_playback_status'>Physics paused</div>";
    else
        stream << "<div class='scene_playback_status'>Physics stopped</div>";
    stream << "</div></div>";
    stream << "<div id='scene_viewport_surface' class='scene_viewport_surface'></div>";
    stream << buildSceneDirtyPromptMarkup();
    stream << "</div>";
    return stream.str();
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

std::string SceneEditorController::makeHierarchyNodeElementId(int nodeId)
{
    return "hierarchy_node_" + std::to_string(nodeId);
}

std::string SceneEditorController::makeAssetDirectoryElementId(const std::string& directoryId)
{
    return "asset_directory_" + encodeElementToken(directoryId);
}

std::string SceneEditorController::makeAssetFileElementId(const std::string& fileId)
{
    return "asset_file_" + encodeElementToken(fileId);
}

std::string SceneEditorController::makeTransformFieldElementId(int nodeId, const std::string& fieldKey)
{
    return makeSceneTransformFieldElementId(nodeId, fieldKey);
}

std::string SceneEditorController::makeInspectorFieldElementId(int nodeId, size_t componentIndex, const std::string& fieldKey)
{
    return makeSceneInspectorFieldElementId(nodeId, componentIndex, fieldKey);
}

std::string SceneEditorController::makeInspectorGroupElementId(int nodeId, size_t componentIndex)
{
    return makeSceneInspectorGroupElementId(nodeId, componentIndex);
}

std::optional<int> SceneEditorController::parseHierarchyNodeId(const Rml::String& elementId)
{
    const std::string value = elementId;
    const std::string prefix = "hierarchy_node_";
    if (!startsWith(value, prefix))
        return std::nullopt;

    return std::stoi(value.substr(prefix.size()));
}

std::optional<std::string> SceneEditorController::parseAssetDirectoryElementId(const Rml::String& elementId)
{
    return parseEncodedElementId(elementId, "asset_directory_", [](const std::string& value) { return !value.empty(); });
}

std::optional<std::string> SceneEditorController::parseAssetFileElementId(const Rml::String& elementId)
{
    return parseEncodedElementId(elementId, "asset_file_", [](const std::string& value) { return !value.empty(); });
}

std::optional<SceneEditorController::InspectorFieldBinding> SceneEditorController::parseInspectorFieldElementId(const Rml::String& elementId)
{
    const std::string value = elementId;
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

SceneEditorController::UiGOHierarchyNode* SceneEditorController::findHierarchyNodeById(int nodeId)
{
    if (m_hierarchyRoot.id == nodeId)
        return &m_hierarchyRoot;

    std::function<UiGOHierarchyNode*(UiGOHierarchyNode&)> findInChildren = [&](UiGOHierarchyNode& node) -> UiGOHierarchyNode*
    {
        for (UiGOHierarchyNode& child : node.children)
        {
            if (child.id == nodeId)
                return &child;
            if (UiGOHierarchyNode* found = findInChildren(child))
                return found;
        }
        return nullptr;
    };

    return findInChildren(m_hierarchyRoot);
}

const SceneEditorController::UiGOHierarchyNode* SceneEditorController::findHierarchyNodeById(int nodeId) const
{
    return const_cast<SceneEditorController*>(this)->findHierarchyNodeById(nodeId);
}

SceneEditorController::UiGOHierarchyNode* SceneEditorController::findHierarchyNodeByGameObject(const GameObject* gameObject)
{
    if (gameObject == nullptr)
        return nullptr;

    std::function<UiGOHierarchyNode*(UiGOHierarchyNode&)> findInChildren = [&](UiGOHierarchyNode& node) -> UiGOHierarchyNode*
    {
        for (UiGOHierarchyNode& child : node.children)
        {
            if (child.gameObject == gameObject)
                return &child;
            if (UiGOHierarchyNode* found = findInChildren(child))
                return found;
        }
        return nullptr;
    };

    return findInChildren(m_hierarchyRoot);
}

const SceneEditorController::UiGOHierarchyNode* SceneEditorController::findHierarchyNodeByGameObject(const GameObject* gameObject) const
{
    return const_cast<SceneEditorController*>(this)->findHierarchyNodeByGameObject(gameObject);
}

const SceneEditorController::UiGOHierarchyNode* SceneEditorController::findSelectedHierarchyNode() const
{
    return findHierarchyNodeById(m_selectedHierarchyNodeId);
}

SceneEditorController::AssetBrowserDirectoryNode* SceneEditorController::findAssetDirectoryById(const std::string& directoryId)
{
    auto findNode = [&](auto&& self, AssetBrowserDirectoryNode& node) -> AssetBrowserDirectoryNode* {
        if (node.id == directoryId)
            return &node;

        for (AssetBrowserDirectoryNode& child : node.children)
        {
            if (AssetBrowserDirectoryNode* found = self(self, child))
                return found;
        }

        return nullptr;
    };

    for (AssetBrowserDirectoryNode& root : m_assetBrowserRoots)
    {
        if (AssetBrowserDirectoryNode* found = findNode(findNode, root))
            return found;
    }

    return nullptr;
}

const SceneEditorController::AssetBrowserDirectoryNode* SceneEditorController::findAssetDirectoryById(const std::string& directoryId) const
{
    return const_cast<SceneEditorController*>(this)->findAssetDirectoryById(directoryId);
}

const SceneEditorController::AssetBrowserFileEntry* SceneEditorController::findAssetFileById(const std::string& fileId) const
{
    auto findInNode = [&](auto&& self, const AssetBrowserDirectoryNode& node) -> const AssetBrowserFileEntry* {
        for (const AssetBrowserFileEntry& file : node.files)
        {
            if (file.id == fileId)
                return &file;
        }

        for (const AssetBrowserDirectoryNode& child : node.children)
        {
            if (const AssetBrowserFileEntry* found = self(self, child))
                return found;
        }

        return nullptr;
    };

    for (const AssetBrowserDirectoryNode& root : m_assetBrowserRoots)
    {
        if (const AssetBrowserFileEntry* found = findInNode(findInNode, root))
            return found;
    }

    return nullptr;
}

const SceneEditorController::AssetBrowserDirectoryNode* SceneEditorController::findSelectedAssetDirectory() const
{
    return findAssetDirectoryById(m_selectedAssetDirectoryId);
}

void SceneEditorController::selectAssetDirectory(const std::string& directoryId)
{
    if (findAssetDirectoryById(directoryId) == nullptr)
        return;

    m_selectedAssetDirectoryId = directoryId;
    m_selectedAssetFileId.clear();
}

void SceneEditorController::toggleAssetDirectoryExpansion(const std::string& directoryId)
{
    if (directoryId.empty())
        return;

    const auto it = m_expandedAssetDirectoryIds.find(directoryId);
    if (it != m_expandedAssetDirectoryIds.end())
        m_expandedAssetDirectoryIds.erase(it);
    else
        m_expandedAssetDirectoryIds.insert(directoryId);
}

bool SceneEditorController::isAssetDirectoryExpanded(const AssetBrowserDirectoryNode& node) const
{
    if (node.id == "Assets" || node.id == "built-in")
        return true;

    return m_expandedAssetDirectoryIds.count(node.id) > 0;
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

    const UiGOHierarchyNode* node = findHierarchyNodeById(binding.nodeId);
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
    UiGOHierarchyNode* node = findHierarchyNodeById(binding.nodeId);
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
            gameObject->transform.setPosition(parsedValue);
            return true;
        }

        if (binding.fieldKey == "rotation")
        {
            gameObject->transform.setRotation(parsedValue);
            return true;
        }

        if (binding.fieldKey == "scale")
        {
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

    return field->write(*component, parsedValue);
}

bool SceneEditorController::canDropDraggedAssetOnInspectorField(const InspectorFieldBinding& binding) const
{
    if (m_dragPayloadKind == DragPayloadKind::None || m_draggedAssetRuntimePath.empty())
        return false;

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

void SceneEditorController::markSceneDirty()
{
    if (!m_sceneDirty)
    {
        m_sceneDirty = true;
        requestHierarchyRefresh();
    }
}

void SceneEditorController::clearSceneDirty()
{
    if (m_sceneDirty)
    {
        m_sceneDirty = false;
        requestHierarchyRefresh();
    }
}

void SceneEditorController::rescanAssetBrowser()
{
    const std::string previousDirectoryId = m_selectedAssetDirectoryId;
    const std::string previousFileId = m_selectedAssetFileId;
    const std::unordered_set<std::string> previousExpandedDirectories = m_expandedAssetDirectoryIds;

    m_assetBrowserRoots.clear();

    const std::filesystem::path runtimeRoot = std::filesystem::current_path();
    struct RootDescriptor
    {
        AssetBrowserRootKind kind;
        std::string label;
        std::filesystem::path diskPath;
        std::string runtimePath;
    };

    const std::vector<RootDescriptor> rootDescriptors = {
        {AssetBrowserRootKind::Assets, "Assets", runtimeRoot / "Assets", "Assets"},
        {AssetBrowserRootKind::BuiltIn, "built-in", runtimeRoot / "built-in", "built-in"},
    };

    auto makeDirectoryNode = [](AssetBrowserRootKind kind, const std::string& label, const std::filesystem::path& diskPath, const std::string& runtimePath) {
        AssetBrowserDirectoryNode node;
        node.id = runtimePath;
        node.label = label;
        node.runtimePath = runtimePath;
        node.diskPath = diskPath.string();
        node.rootKind = kind;
        return node;
    };

    auto scanDirectory = [&](auto&& self, AssetBrowserDirectoryNode& node) -> void {
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

        auto sortEntries = [](auto& entries) {
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
            node.children.push_back(makeDirectoryNode(node.rootKind, childLabel, entry.path(), childRuntimePath));
            self(self, node.children.back());
        }

        for (const std::filesystem::directory_entry& entry : childFiles)
        {
            AssetBrowserFileEntry file;
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
        m_assetBrowserRoots.push_back(makeDirectoryNode(rootDescriptor.kind, rootDescriptor.label, rootDescriptor.diskPath, rootDescriptor.runtimePath));
        scanDirectory(scanDirectory, m_assetBrowserRoots.back());
    }

    m_expandedAssetDirectoryIds.clear();
    for (const std::string& expandedDirectoryId : previousExpandedDirectories)
    {
        if (findAssetDirectoryById(expandedDirectoryId) != nullptr)
            m_expandedAssetDirectoryIds.insert(expandedDirectoryId);
    }
    m_expandedAssetDirectoryIds.insert("Assets");
    m_expandedAssetDirectoryIds.insert("built-in");

    if (findAssetDirectoryById(previousDirectoryId) != nullptr)
        m_selectedAssetDirectoryId = previousDirectoryId;
    else if (!m_assetBrowserRoots.empty())
        m_selectedAssetDirectoryId = m_assetBrowserRoots.front().id;
    else
        m_selectedAssetDirectoryId.clear();

    if (const AssetBrowserFileEntry* file = findAssetFileById(previousFileId))
    {
        const AssetBrowserDirectoryNode* selectedDirectory = findSelectedAssetDirectory();
        if (selectedDirectory != nullptr && startsWith(file->runtimePath, selectedDirectory->runtimePath + "/"))
            m_selectedAssetFileId = previousFileId;
        else
            m_selectedAssetFileId.clear();
    }
    else
    {
        m_selectedAssetFileId.clear();
    }
}

void SceneEditorController::closeHeaderMenus()
{
    m_isFileMenuOpen = false;
    m_isWindowMenuOpen = false;
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

bool SceneEditorController::hierarchyNodesEqual(const UiGOHierarchyNode& lhs, const UiGOHierarchyNode& rhs)
{
    if (lhs.id != rhs.id ||
        lhs.label != rhs.label ||
        lhs.tagName != rhs.tagName ||
        lhs.gameObject != rhs.gameObject ||
        lhs.children.size() != rhs.children.size())
    {
        return false;
    }

    for (size_t index = 0; index < lhs.children.size(); ++index)
    {
        if (!hierarchyNodesEqual(lhs.children[index], rhs.children[index]))
            return false;
    }

    return true;
}

void SceneEditorController::rebuildHierarchyFromScene(const Scene& scene)
{
    m_hierarchyRoot.children.clear();
    m_nextHierarchyNodeId = m_hierarchyRoot.id + 1;

    std::unordered_set<const GameObject*> childObjects;
    for (size_t index = 0; index < scene.getGameObjectCount(); ++index)
    {
        GameObject* gameObject = scene.getGameObject(index);
        if (gameObject == nullptr)
            continue;

        for (size_t childIndex = 0; childIndex < gameObject->transform.getChildCount(); ++childIndex)
        {
            if (GameObject* child = gameObject->transform.getChild(childIndex))
                childObjects.insert(child);
        }
    }

    for (size_t index = 0; index < scene.getGameObjectCount(); ++index)
    {
        GameObject* gameObject = scene.getGameObject(index);
        if (gameObject == nullptr || childObjects.count(gameObject) > 0)
            continue;

        appendHierarchyNodeFromGameObject(m_hierarchyRoot, *gameObject);
    }
}

void SceneEditorController::appendHierarchyNodeFromGameObject(UiGOHierarchyNode& parentNode, const GameObject& gameObject)
{
    parentNode.children.push_back({m_nextHierarchyNodeId++, gameObject.getName(), "gameobject", &gameObject, {}});
    UiGOHierarchyNode& newNode = parentNode.children.back();

    for (size_t childIndex = 0; childIndex < gameObject.transform.getChildCount(); ++childIndex)
    {
        const GameObject* child = gameObject.transform.getChild(childIndex);
        if (child != nullptr)
            appendHierarchyNodeFromGameObject(newNode, *child);
    }
}

void SceneEditorController::requestHierarchyRefresh()
{
    m_hierarchyRefreshPending = true;
}