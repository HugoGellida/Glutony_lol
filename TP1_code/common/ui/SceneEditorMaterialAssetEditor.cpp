#include "SceneEditorController.hpp"

#include "EditorUiCommon.hpp"

#include <common/app/RuntimePreviewSession.hpp>
#include <common/asset/editor/MaterialAssetEditor.hpp>
#include <common/Scene.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <system_error>
#include <type_traits>
#include <variant>

namespace
{
std::string trimCopyLocal(const std::string& value)
{
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0)
        ++start;

    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
        --end;

    return value.substr(start, end - start);
}

std::string lowercaseCopyLocal(const std::string& value)
{
    std::string lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return lowered;
}

bool parseVec3Local(const std::string& rawValue, glm::vec3& outValue)
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

std::string formatFloatLocal(float value)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(3) << value;
    std::string formatted = stream.str();
    while (formatted.size() > 1 && formatted.back() == '0' && formatted[formatted.size() - 2] != '.')
        formatted.pop_back();
    if (!formatted.empty() && formatted.back() == '.')
        formatted.push_back('0');
    return formatted;
}

std::string formatSerializedValueLocal(const component_meta::SerializedValue& value)
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
            return formatFloatLocal(storedValue);
        }
        else if constexpr (std::is_same_v<StoredType, glm::vec3>)
        {
            return formatFloatLocal(storedValue.x) + ", " + formatFloatLocal(storedValue.y) + ", " + formatFloatLocal(storedValue.z);
        }
        else
        {
            return storedValue;
        }
    }, value);
}

bool parseMaterialFieldValue(component_meta::FieldKind kind, const std::string& rawValue, component_meta::SerializedValue& outValue)
{
    const std::string trimmedValue = trimCopyLocal(rawValue);

    try
    {
        switch (kind)
        {
        case component_meta::FieldKind::Bool:
            if (trimmedValue == "true" || trimmedValue == "1")
                return outValue = true, true;
            if (trimmedValue == "false" || trimmedValue == "0")
                return outValue = false, true;
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
            if (!parseVec3Local(trimmedValue, parsedValue))
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

const char* assetReferenceKindLabelLocal(component_meta::AssetReferenceKind kind)
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

std::string buildMaterialEditorFieldMarkup(
    const std::string& fieldId,
    const std::string& fieldLabel,
    component_meta::FieldKind fieldKind,
    const component_meta::SerializedValue& value,
    const std::vector<component_meta::EnumOption>& enumOptions,
    component_meta::AssetReferenceKind assetReferenceKind)
{
    std::ostringstream stream;
    stream << "<div class='inspector_field_row'><div class='inspector_field_name'>" << editor_ui::escapeRmlText(fieldLabel) << "</div>";

    const std::string formattedValue = formatSerializedValueLocal(value);
    const bool isAssetField = assetReferenceKind != component_meta::AssetReferenceKind::None || fieldKind == component_meta::FieldKind::Asset;
    const std::string fieldClass = std::string("inspector_field_input") + (isAssetField ? " inspector_asset_field" : "");

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
            stream << "<option value='" << editor_ui::escapeRmlText(option.value) << "'";
            if (option.value == formattedValue)
                stream << " selected='selected'";
            stream << ">" << editor_ui::escapeRmlText(option.label) << "</option>";
        }
        stream << "</select>";
    }
    else
    {
        stream << "<input id='" << fieldId << "' class='" << fieldClass << "' type='";
        stream << ((fieldKind == component_meta::FieldKind::Float || fieldKind == component_meta::FieldKind::Int) ? "number" : "text");
        stream << "' value='" << editor_ui::escapeRmlText(formattedValue) << "'";
        if (isAssetField)
            stream << " placeholder='" << editor_ui::escapeRmlText(assetReferenceKindLabelLocal(assetReferenceKind)) << "'";
        stream << "' />";
    }

    if (isAssetField)
        stream << "<div class='inspector_asset_hint'>" << editor_ui::escapeRmlText(assetReferenceKindLabelLocal(assetReferenceKind)) << "</div>";

    stream << "</div>";
    return stream.str();
}

bool writeRuntimePreviewMaterialFile(
    const std::filesystem::path& tempPath,
    const std::filesystem::path& targetPath,
    uint64_t sequence,
    const std::string& materialAssetPath)
{
    std::error_code errorCode;
    std::filesystem::create_directories(targetPath.parent_path(), errorCode);
    if (errorCode)
        return false;

    std::ofstream output(tempPath, std::ios::trunc);
    if (!output)
        return false;

    output << sequence << '\n' << materialAssetPath << '\n';
    output.close();

    std::filesystem::rename(tempPath, targetPath, errorCode);
    if (errorCode)
    {
        std::filesystem::remove(tempPath, errorCode);
        return false;
    }

    return true;
}

std::string makeUniformPropertyKey(size_t fieldIndex)
{
    return "uniform_" + std::to_string(fieldIndex);
}

std::optional<size_t> parseUniformPropertyIndex(const std::string& propertyKey)
{
    const std::string prefix = "uniform_";
    if (!editor_ui::startsWith(propertyKey, prefix))
        return std::nullopt;

    try
    {
        return static_cast<size_t>(std::stoul(propertyKey.substr(prefix.size())));
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}
}

std::string SceneEditorController::makeMaterialAssetEditorGroupElementId(int nodeId, size_t componentIndex, const std::string& fieldKey)
{
    return "scene_material_asset_group_" + std::to_string(nodeId) + "__" + std::to_string(componentIndex) + "__" + fieldKey;
}

std::string SceneEditorController::makeMaterialAssetEditorIconElementId(int nodeId, size_t componentIndex, const std::string& fieldKey)
{
    return "scene_material_asset_icon_" + std::to_string(nodeId) + "__" + std::to_string(componentIndex) + "__" + fieldKey;
}

std::string SceneEditorController::makeMaterialAssetEditorBodyElementId(int nodeId, size_t componentIndex, const std::string& fieldKey)
{
    return "scene_material_asset_body_" + std::to_string(nodeId) + "__" + std::to_string(componentIndex) + "__" + fieldKey;
}

std::string SceneEditorController::makeMaterialAssetEditorFieldElementId(int nodeId, size_t componentIndex, const std::string& fieldKey, const std::string& propertyKey)
{
    return "scene_material_asset_field_" + std::to_string(nodeId) + "__" + std::to_string(componentIndex) + "__" + fieldKey + "__" + propertyKey;
}

std::optional<SceneEditorController::InspectorFieldBinding> SceneEditorController::parseMaterialAssetEditorGroupElementId(const Rml::String& elementId)
{
    const std::string value = elementId;
    const std::string prefix = "scene_material_asset_group_";
    if (!editor_ui::startsWith(value, prefix))
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

std::optional<SceneEditorController::MaterialAssetEditorBinding> SceneEditorController::parseMaterialAssetEditorFieldElementId(const Rml::String& elementId)
{
    const std::string value = elementId;
    const std::string prefix = "scene_material_asset_field_";
    if (!editor_ui::startsWith(value, prefix))
        return std::nullopt;

    const size_t firstSeparator = value.find("__", prefix.size());
    if (firstSeparator == std::string::npos)
        return std::nullopt;

    const size_t secondSeparator = value.find("__", firstSeparator + 2);
    if (secondSeparator == std::string::npos)
        return std::nullopt;

    const size_t thirdSeparator = value.find("__", secondSeparator + 2);
    if (thirdSeparator == std::string::npos)
        return std::nullopt;

    MaterialAssetEditorBinding binding;
    binding.parentField.target = InspectorFieldBinding::Target::Component;
    binding.parentField.nodeId = std::stoi(value.substr(prefix.size(), firstSeparator - prefix.size()));
    binding.parentField.componentIndex = static_cast<size_t>(std::stoul(value.substr(firstSeparator + 2, secondSeparator - (firstSeparator + 2))));
    binding.parentField.fieldKey = value.substr(secondSeparator + 2, thirdSeparator - (secondSeparator + 2));
    binding.propertyKey = value.substr(thirdSeparator + 2);
    return binding;
}

bool SceneEditorController::isMaterialAssetInspectorField(const InspectorFieldBinding& binding) const
{
    const component_meta::ComponentFieldDescriptor* field = findInspectorFieldDescriptor(binding);
    return field != nullptr && field->assetReferenceKind == component_meta::AssetReferenceKind::Material;
}

bool SceneEditorController::applyMaterialAssetEditorFieldValue(const MaterialAssetEditorBinding& binding, const std::string& value)
{
    if (m_scene == nullptr)
        return false;

    UiGOHierarchyNode* node = findHierarchyNodeById(binding.parentField.nodeId);
    if (node == nullptr || node->gameObject == nullptr)
        return false;

    component::Component* component = const_cast<GameObject*>(node->gameObject)->getComponentAt(binding.parentField.componentIndex);
    if (component == nullptr)
        return false;

    const component_meta::ComponentFieldDescriptor* field = findInspectorFieldDescriptor(binding.parentField);
    if (field == nullptr || !field->read)
        return false;

    const component_meta::SerializedValue assetPathSerialized = field->read(*component);
    const std::string* assetPathValue = std::get_if<std::string>(&assetPathSerialized);
    if (assetPathValue == nullptr || assetPathValue->empty())
        return false;

    asset::editor::MaterialAssetEditorModel editorModel = asset::editor::loadMaterialAssetEditorModel(*assetPathValue);
    if (!editorModel.valid)
        return false;

    const asset::MaterialAssetDefinition previousDefinition = editorModel.definition;

    if (binding.propertyKey == "type")
    {
        const std::string loweredValue = lowercaseCopyLocal(trimCopyLocal(value));
        if (loweredValue != "lit" && loweredValue != "unlit")
            return false;

        const asset::MaterialAssetKind nextKind = loweredValue == "unlit" ? asset::MaterialAssetKind::Unlit : asset::MaterialAssetKind::Lit;
        if (editorModel.definition.kind == nextKind)
            return false;
        editorModel.definition.kind = nextKind;
    }
    else if (binding.propertyKey == "shader")
    {
        const std::string normalizedShaderPath = asset::AssetManager::normalizeRelativePath(trimCopyLocal(value));
        if (normalizedShaderPath.empty() || editorModel.definition.shaderPath == normalizedShaderPath)
            return false;

        Shader* shader = asset::AssetManager::instance().loadShader(normalizedShaderPath);
        if (shader == nullptr)
            return false;

        editorModel.definition.shaderPath = normalizedShaderPath;
        editorModel.definition = asset::editor::normalizeDefinitionForShader(editorModel.definition, *shader);
    }
    else
    {
        const std::optional<size_t> uniformIndex = parseUniformPropertyIndex(binding.propertyKey);
        if (!uniformIndex.has_value() || *uniformIndex >= editorModel.fields.size())
            return false;

        const asset::editor::MaterialAssetEditorField& editorField = editorModel.fields[*uniformIndex];
        component_meta::SerializedValue parsedValue;
        if (!parseMaterialFieldValue(editorField.fieldKind, value, parsedValue))
            return false;
        if (formatSerializedValueLocal(editorField.value) == formatSerializedValueLocal(parsedValue))
            return false;

        if (*uniformIndex >= editorModel.definition.uniforms.size())
            return false;

        asset::MaterialUniformDefinition* uniform = &editorModel.definition.uniforms[*uniformIndex];

        switch (uniform->kind)
        {
        case asset::MaterialUniformKind::Bool:
        {
            const bool* parsedBool = std::get_if<bool>(&parsedValue);
            if (parsedBool == nullptr)
                return false;
            uniform->boolValue = *parsedBool;
            break;
        }
        case asset::MaterialUniformKind::Int:
        {
            const int* parsedInt = std::get_if<int>(&parsedValue);
            if (parsedInt == nullptr)
                return false;
            uniform->intValue = *parsedInt;
            break;
        }
        case asset::MaterialUniformKind::Float:
        {
            const float* parsedFloat = std::get_if<float>(&parsedValue);
            if (parsedFloat == nullptr)
                return false;
            uniform->floatValue = *parsedFloat;
            break;
        }
        case asset::MaterialUniformKind::Vec3:
        {
            const glm::vec3* parsedVec3 = std::get_if<glm::vec3>(&parsedValue);
            if (parsedVec3 == nullptr)
                return false;
            uniform->vec3Value = *parsedVec3;
            break;
        }
        case asset::MaterialUniformKind::Texture:
        {
            const std::string* parsedString = std::get_if<std::string>(&parsedValue);
            if (parsedString == nullptr)
                return false;
            uniform->textureAssetPath = asset::AssetManager::normalizeRelativePath(*parsedString);
            break;
        }
        }
    }

    const std::string diskPath = asset::AssetManager::runtimePath(editorModel.normalizedAssetPath);
    if (!asset::MaterialAssetIO::saveDefinition(diskPath, editorModel.definition))
    {
        appendConsoleSystemMessage("[asset] Failed to save material asset: " + editorModel.normalizedAssetPath, "console_line_error");
        return false;
    }

    if (!m_scene->refreshMaterialAsset(editorModel.normalizedAssetPath))
    {
        asset::MaterialAssetIO::saveDefinition(diskPath, previousDefinition);
        m_scene->refreshMaterialAsset(editorModel.normalizedAssetPath);
        appendConsoleSystemMessage("[asset] Failed to reload material asset: " + editorModel.normalizedAssetPath, "console_line_error");
        return false;
    }

    if (isExternalPreviewActive())
    {
        if (!writeRuntimePreviewMaterialFile(
                runtime_preview::materialMetadataTempPath(),
                runtime_preview::materialMetadataPath(),
                m_runtimeMaterialSyncSequence + 1,
                editorModel.normalizedAssetPath))
        {
            appendConsoleSystemMessage("[play] Failed to sync material asset to preview: " + editorModel.normalizedAssetPath, "console_line_error");
        }
        else
        {
            ++m_runtimeMaterialSyncSequence;
        }
    }

    return true;
}

void SceneEditorController::toggleMaterialAssetEditor(const InspectorFieldBinding& binding)
{
    const std::string editorId = makeMaterialAssetEditorGroupElementId(binding.nodeId, binding.componentIndex, binding.fieldKey);
    const auto it = m_collapsedMaterialAssetEditors.find(editorId);
    if (it != m_collapsedMaterialAssetEditors.end())
        m_collapsedMaterialAssetEditors.erase(it);
    else
        m_collapsedMaterialAssetEditors.insert(editorId);
}

bool SceneEditorController::isMaterialAssetEditorCollapsed(const InspectorFieldBinding& binding) const
{
    return m_collapsedMaterialAssetEditors.count(makeMaterialAssetEditorGroupElementId(binding.nodeId, binding.componentIndex, binding.fieldKey)) > 0;
}

std::string SceneEditorController::buildMaterialAssetEditorMarkup(const InspectorFieldBinding& binding, const std::string& assetPath) const
{
    const bool collapsed = isMaterialAssetEditorCollapsed(binding);
    const std::string groupId = makeMaterialAssetEditorGroupElementId(binding.nodeId, binding.componentIndex, binding.fieldKey);
    const std::string iconId = makeMaterialAssetEditorIconElementId(binding.nodeId, binding.componentIndex, binding.fieldKey);
    const std::string bodyId = makeMaterialAssetEditorBodyElementId(binding.nodeId, binding.componentIndex, binding.fieldKey);

    std::ostringstream stream;
    stream << "<div class='inspector_foldout inspector_asset_editor_foldout'>";
    stream << "<div id='" << groupId << "' class='inspector_foldout_header inspector_asset_editor_header'>";
    stream << "<div id='" << iconId << "' class='inspector_foldout_icon'>" << (collapsed ? ">" : "v") << "</div>";
    stream << "<div class='inspector_foldout_title'>Material Asset</div>";
    stream << "</div>";
    stream << "<div id='" << bodyId << "' class='inspector_foldout_body inspector_asset_editor_body'>";
    if (!collapsed)
        stream << buildMaterialAssetEditorBodyMarkup(binding, assetPath);
    stream << "</div></div>";
    return stream.str();
}

std::string SceneEditorController::buildMaterialAssetEditorBodyMarkup(const InspectorFieldBinding& binding, const std::string& assetPath) const
{
    const asset::editor::MaterialAssetEditorModel editorModel = asset::editor::loadMaterialAssetEditorModel(assetPath);
    if (!editorModel.available)
        return "<div class='inspector_asset_hint'>Assign a .mat asset to edit its properties.</div>";
    if (!editorModel.valid)
        return "<div class='inspector_asset_hint'>" + editor_ui::escapeRmlText(editorModel.errorMessage) + "</div>";

    static const std::vector<component_meta::EnumOption> materialKindOptions = {
        {"lit", "Lit"},
        {"unlit", "Unlit"},
    };

    std::ostringstream stream;
    stream << "<div class='inspector_summary_text'>" << editor_ui::escapeRmlText(editorModel.normalizedAssetPath) << "</div>";
    stream << buildMaterialEditorFieldMarkup(
        makeMaterialAssetEditorFieldElementId(binding.nodeId, binding.componentIndex, binding.fieldKey, "type"),
        "Type",
        component_meta::FieldKind::Enum,
        std::string(asset::editor::materialAssetKindValue(editorModel.definition.kind)),
        materialKindOptions,
        component_meta::AssetReferenceKind::None);
    stream << buildMaterialEditorFieldMarkup(
        makeMaterialAssetEditorFieldElementId(binding.nodeId, binding.componentIndex, binding.fieldKey, "shader"),
        "Shader",
        component_meta::FieldKind::Asset,
        editorModel.definition.shaderPath,
        {},
        component_meta::AssetReferenceKind::Shader);

    for (size_t fieldIndex = 0; fieldIndex < editorModel.fields.size(); ++fieldIndex)
    {
        const asset::editor::MaterialAssetEditorField& field = editorModel.fields[fieldIndex];
        stream << buildMaterialEditorFieldMarkup(
            makeMaterialAssetEditorFieldElementId(binding.nodeId, binding.componentIndex, binding.fieldKey, makeUniformPropertyKey(fieldIndex)),
            field.label,
            field.fieldKind,
            field.value,
            {},
            field.assetReferenceKind);
    }

    if (!editorModel.unsupportedUniforms.empty())
    {
        stream << "<div class='inspector_asset_hint'>Unsupported uniforms: ";
        for (size_t index = 0; index < editorModel.unsupportedUniforms.size(); ++index)
        {
            if (index > 0)
                stream << ", ";
            stream << editor_ui::escapeRmlText(editorModel.unsupportedUniforms[index]);
        }
        stream << "</div>";
    }

    return stream.str();
}

void SceneEditorController::refreshMaterialAssetEditorPresentation(const InspectorFieldBinding& binding)
{
    if (m_document == nullptr || !isMaterialAssetInspectorField(binding))
        return;

    UiGOHierarchyNode* node = findHierarchyNodeById(binding.nodeId);
    if (node == nullptr || node->gameObject == nullptr)
        return;

    component::Component* component = const_cast<GameObject*>(node->gameObject)->getComponentAt(binding.componentIndex);
    if (component == nullptr)
        return;

    const component_meta::ComponentFieldDescriptor* field = findInspectorFieldDescriptor(binding);
    if (field == nullptr || !field->read)
        return;

    const component_meta::SerializedValue assetPathSerialized = field->read(*component);
    const std::string* assetPath = std::get_if<std::string>(&assetPathSerialized);
    const std::string currentAssetPath = assetPath != nullptr ? *assetPath : std::string();

    if (Rml::Element* icon = m_document->GetElementById(makeMaterialAssetEditorIconElementId(binding.nodeId, binding.componentIndex, binding.fieldKey)))
        icon->SetInnerRML(isMaterialAssetEditorCollapsed(binding) ? ">" : "v");

    if (Rml::Element* body = m_document->GetElementById(makeMaterialAssetEditorBodyElementId(binding.nodeId, binding.componentIndex, binding.fieldKey)))
    {
        const Rml::Element* focusedElement = m_context != nullptr ? m_context->GetFocusElement() : nullptr;
        for (const Rml::Element* element = focusedElement; element != nullptr; element = element->GetParentNode())
        {
            if (element == body)
                return;
        }

        if (isMaterialAssetEditorCollapsed(binding))
            body->SetInnerRML("");
        else
            body->SetInnerRML(buildMaterialAssetEditorBodyMarkup(binding, currentAssetPath));
    }
}
