#include "SceneEditorController.hpp"

#include "EditorUiCommon.hpp"

#include <common/app/RuntimePreviewSession.hpp>
#include <common/asset/editor/DataAssetEditor.hpp>
#include <common/Scene.hpp>

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
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
    stream << std::fixed << std::setprecision(6) << value;
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
    stream << "<div class='inspector_field_row'><div class='inspector_field_name'>" << editor_ui::escapeRmlText(fieldLabel) << "</div>";

    const std::string formattedValue = formatSerializedValueLocal(value);
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
            if (fieldKind == component_meta::FieldKind::Float)
                stream << " step='any'";
        if (isAssetField)
            stream << " placeholder='" << editor_ui::escapeRmlText(assetReferenceKindLabelLocal(assetReferenceKind)) << "'";
        stream << " />";
    }

    if (isAssetField)
        stream << "<div class='inspector_asset_hint'>" << editor_ui::escapeRmlText(assetReferenceKindLabelLocal(assetReferenceKind)) << "</div>";

    stream << "</div>";
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

std::string encodeElementToken(const std::string& value)
{
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (unsigned char character : value)
        stream << std::setw(2) << static_cast<int>(character);
    return stream.str();
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

bool parseDataAssetFieldValue(component_meta::FieldKind kind, const std::string& rawValue, component_meta::SerializedValue& outValue)
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

bool elementIsFocusedOrContainsFocus(Rml::Context* context, Rml::Element* element)
{
    if (context == nullptr || element == nullptr)
        return false;

    const Rml::Element* focusedElement = context->GetFocusElement();
    for (const Rml::Element* current = focusedElement; current != nullptr; current = current->GetParentNode())
    {
        if (current == element)
            return true;
    }

    return false;
}

void patchDataAssetEditorFieldValue(Rml::ElementDocument* document, Rml::Context* context, const std::string& fieldId, const std::string& formattedValue)
{
    if (document == nullptr)
        return;

    Rml::Element* fieldElement = document->GetElementById(fieldId);
    Rml::ElementFormControl* formControl = dynamic_cast<Rml::ElementFormControl*>(fieldElement);
    if (formControl == nullptr || elementIsFocusedOrContainsFocus(context, fieldElement))
        return;

    if (formControl->GetValue() != formattedValue)
        formControl->SetValue(formattedValue);
}

bool writeRuntimePreviewDataAssetFile(
    const std::filesystem::path& tempPath,
    const std::filesystem::path& targetPath,
    uint64_t sequence,
    const std::string& dataAssetPath)
{
    std::error_code errorCode;
    std::filesystem::create_directories(targetPath.parent_path(), errorCode);
    if (errorCode)
        return false;

    std::ofstream output(tempPath, std::ios::trunc);
    if (!output)
        return false;

    output << sequence << '\n' << dataAssetPath << '\n';
    output.close();

    std::filesystem::rename(tempPath, targetPath, errorCode);
    if (errorCode)
    {
        std::filesystem::remove(tempPath, errorCode);
        return false;
    }

    return true;
}

std::string makeDataAssetNodePath(const std::string& parentPath, size_t childIndex)
{
    if (parentPath.empty())
        return std::to_string(childIndex);
    return parentPath + "/" + std::to_string(childIndex);
}

void appendDataAssetStructureSignature(std::ostringstream& stream, const asset::DataAssetNodeDefinition& node, const std::string& nodePath)
{
    stream << '|' << nodePath
           << ':' << node.name
           << ':' << static_cast<int>(node.kind)
           << ':' << node.children.size();

    for (size_t childIndex = 0; childIndex < node.children.size(); ++childIndex)
        appendDataAssetStructureSignature(stream, node.children[childIndex], makeDataAssetNodePath(nodePath, childIndex));
}

std::string buildDataAssetEditorStructureSignature(const asset::editor::DataAssetEditorModel& editorModel)
{
    std::ostringstream stream;
    stream << editorModel.normalizedAssetPath
           << '|'
           << editorModel.definition.rootName
           << '|'
           << editorModel.definition.children.size();

    for (size_t childIndex = 0; childIndex < editorModel.definition.children.size(); ++childIndex)
        appendDataAssetStructureSignature(stream, editorModel.definition.children[childIndex], makeDataAssetNodePath("", childIndex));

    return stream.str();
}

const asset::DataAssetNodeDefinition* findDataAssetNodeByPath(const asset::DataAssetDefinition& definition, const std::string& nodePath)
{
    if (nodePath.empty())
        return nullptr;

    const std::vector<asset::DataAssetNodeDefinition>* currentChildren = &definition.children;
    const asset::DataAssetNodeDefinition* currentNode = nullptr;
    size_t cursor = 0;

    while (cursor <= nodePath.size())
    {
        const size_t separator = nodePath.find('/', cursor);
        const std::string token = nodePath.substr(cursor, separator == std::string::npos ? std::string::npos : separator - cursor);
        if (token.empty())
            return nullptr;

        size_t childIndex = 0;
        try
        {
            childIndex = static_cast<size_t>(std::stoul(token));
        }
        catch (const std::exception&)
        {
            return nullptr;
        }

        if (childIndex >= currentChildren->size())
            return nullptr;

        currentNode = &(*currentChildren)[childIndex];
        currentChildren = &currentNode->children;
        if (separator == std::string::npos)
            break;
        cursor = separator + 1;
    }

    return currentNode;
}

asset::DataAssetNodeDefinition* findDataAssetNodeByPath(asset::DataAssetDefinition& definition, const std::string& nodePath)
{
    return const_cast<asset::DataAssetNodeDefinition*>(findDataAssetNodeByPath(static_cast<const asset::DataAssetDefinition&>(definition), nodePath));
}

const char* dataAssetNodeKindLabel(asset::DataAssetValueKind kind)
{
    switch (kind)
    {
    case asset::DataAssetValueKind::Bool:
        return "bool";
    case asset::DataAssetValueKind::Int:
        return "int";
    case asset::DataAssetValueKind::Float:
        return "float";
    case asset::DataAssetValueKind::Vec3:
        return "vec3";
    case asset::DataAssetValueKind::String:
        return "string";
    case asset::DataAssetValueKind::Group:
    default:
        return "group";
    }
}
}

std::string SceneEditorController::makeDataAssetEditorBindingToken(const InspectorFieldBinding& binding)
{
    switch (binding.target)
    {
    case InspectorFieldBinding::Target::Scene:
        return encodeElementToken(makeSceneFieldElementId(binding.fieldKey));
    case InspectorFieldBinding::Target::Transform:
        return encodeElementToken(makeTransformFieldElementId(binding.nodeId, binding.fieldKey));
    case InspectorFieldBinding::Target::Component:
    default:
        return encodeElementToken(makeInspectorFieldElementId(binding.nodeId, binding.componentIndex, binding.fieldKey));
    }
}

std::optional<SceneEditorController::InspectorFieldBinding> SceneEditorController::parseDataAssetEditorBindingToken(const std::string& token)
{
    const std::optional<std::string> decodedFieldId = decodeElementToken(token);
    if (!decodedFieldId.has_value())
        return std::nullopt;

    return parseInspectorFieldElementId(*decodedFieldId);
}

std::string SceneEditorController::makeDataAssetEditorGroupElementId(const InspectorFieldBinding& binding)
{
    return "scene_data_asset_group__" + makeDataAssetEditorBindingToken(binding);
}

std::string SceneEditorController::makeDataAssetEditorIconElementId(const InspectorFieldBinding& binding)
{
    return "scene_data_asset_icon__" + makeDataAssetEditorBindingToken(binding);
}

std::string SceneEditorController::makeDataAssetEditorBodyElementId(const InspectorFieldBinding& binding)
{
    return "scene_data_asset_body__" + makeDataAssetEditorBindingToken(binding);
}

std::string SceneEditorController::makeDataAssetEditorNodeGroupElementId(const InspectorFieldBinding& binding, const std::string& nodePath)
{
    return "scene_data_asset_node_group__" + makeDataAssetEditorBindingToken(binding) + "__" + encodeElementToken(nodePath);
}

std::string SceneEditorController::makeDataAssetEditorFieldElementId(const InspectorFieldBinding& binding, const std::string& nodePath)
{
    return "scene_data_asset_field__" + makeDataAssetEditorBindingToken(binding) + "__" + encodeElementToken(nodePath);
}

std::optional<SceneEditorController::InspectorFieldBinding> SceneEditorController::parseDataAssetEditorGroupElementId(const Rml::String& elementId)
{
    const std::string value = elementId;
    const std::string prefix = "scene_data_asset_group__";
    if (!editor_ui::startsWith(value, prefix))
        return std::nullopt;

    return parseDataAssetEditorBindingToken(value.substr(prefix.size()));
}

std::optional<SceneEditorController::DataAssetEditorBinding> SceneEditorController::parseDataAssetEditorNodeGroupElementId(const Rml::String& elementId)
{
    const std::string value = elementId;
    const std::string prefix = "scene_data_asset_node_group__";
    if (!editor_ui::startsWith(value, prefix))
        return std::nullopt;

    const size_t separator = value.find("__", prefix.size());
    if (separator == std::string::npos)
        return std::nullopt;

    const std::optional<InspectorFieldBinding> parentBinding = parseDataAssetEditorBindingToken(value.substr(prefix.size(), separator - prefix.size()));
    if (!parentBinding.has_value())
        return std::nullopt;

    const std::optional<std::string> decodedNodePath = decodeElementToken(value.substr(separator + 2));
    if (!decodedNodePath.has_value())
        return std::nullopt;

    DataAssetEditorBinding binding;
    binding.parentField = *parentBinding;
    binding.nodePath = *decodedNodePath;
    return binding;
}

std::optional<SceneEditorController::DataAssetEditorBinding> SceneEditorController::parseDataAssetEditorFieldElementId(const Rml::String& elementId)
{
    const std::string value = elementId;
    const std::string prefix = "scene_data_asset_field__";
    if (!editor_ui::startsWith(value, prefix))
        return std::nullopt;

    const size_t separator = value.find("__", prefix.size());
    if (separator == std::string::npos)
        return std::nullopt;

    const std::optional<InspectorFieldBinding> parentBinding = parseDataAssetEditorBindingToken(value.substr(prefix.size(), separator - prefix.size()));
    if (!parentBinding.has_value())
        return std::nullopt;

    const std::optional<std::string> decodedNodePath = decodeElementToken(value.substr(separator + 2));
    if (!decodedNodePath.has_value())
        return std::nullopt;

    DataAssetEditorBinding binding;
    binding.parentField = *parentBinding;
    binding.nodePath = *decodedNodePath;
    return binding;
}

bool SceneEditorController::isDataAssetInspectorField(const InspectorFieldBinding& binding) const
{
    if (binding.target == InspectorFieldBinding::Target::Scene)
        return binding.fieldKey == "dataAsset";

    const component_meta::ComponentFieldDescriptor* field = findInspectorFieldDescriptor(binding);
    return field != nullptr && field->assetReferenceKind == component_meta::AssetReferenceKind::Data;
}

std::optional<std::string> SceneEditorController::getDataAssetInspectorAssetPath(const InspectorFieldBinding& binding) const
{
    if (!isDataAssetInspectorField(binding))
        return std::nullopt;

    if (binding.target == InspectorFieldBinding::Target::Scene)
    {
        if (m_scene == nullptr)
            return std::nullopt;

        return asset::AssetManager::normalizeRelativePath(m_scene->getDataAssetPath());
    }

    const UiGOHierarchyNode* node = m_hierarchyModel.findNodeById(binding.nodeId);
    if (node == nullptr || node->gameObject == nullptr)
        return std::nullopt;

    const component::Component* component = node->gameObject->getComponentAt(binding.componentIndex);
    const component_meta::ComponentFieldDescriptor* field = findInspectorFieldDescriptor(binding);
    if (component == nullptr || field == nullptr || !field->read)
        return std::nullopt;

    const component_meta::SerializedValue value = field->read(*component);
    const std::string* assetPath = std::get_if<std::string>(&value);
    if (assetPath == nullptr)
        return std::nullopt;

    return asset::AssetManager::normalizeRelativePath(*assetPath);
}

std::vector<SceneEditorController::InspectorFieldBinding> SceneEditorController::collectVisibleDataAssetInspectorBindings() const
{
    std::vector<InspectorFieldBinding> bindings;
    const UiGOHierarchyNode* selectedNode = m_hierarchyModel.findSelectedNode();
    if (selectedNode == nullptr)
        return bindings;

    if (selectedNode->gameObject == nullptr)
    {
        InspectorFieldBinding binding;
        binding.target = InspectorFieldBinding::Target::Scene;
        binding.nodeId = 0;
        binding.fieldKey = "dataAsset";
        bindings.push_back(binding);
        return bindings;
    }

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
            if (field.assetReferenceKind != component_meta::AssetReferenceKind::Data)
                continue;

            InspectorFieldBinding binding;
            binding.target = InspectorFieldBinding::Target::Component;
            binding.nodeId = selectedNode->id;
            binding.componentIndex = componentIndex;
            binding.fieldKey = field.key;
            bindings.push_back(binding);
        }
    }

    return bindings;
}

void SceneEditorController::refreshVisibleDataAssetEditorsForAsset(const std::string& normalizedAssetPath)
{
    if (normalizedAssetPath.empty())
        return;

    for (const InspectorFieldBinding& binding : collectVisibleDataAssetInspectorBindings())
    {
        const std::optional<std::string> bindingAssetPath = getDataAssetInspectorAssetPath(binding);
        if (!bindingAssetPath.has_value() || *bindingAssetPath != normalizedAssetPath)
            continue;

        refreshDataAssetEditorPresentation(binding);
    }
}

bool SceneEditorController::applyDataAssetEditorFieldValue(const DataAssetEditorBinding& binding, const std::string& value)
{
    if (m_scene == nullptr || !isDataAssetInspectorField(binding.parentField))
        return false;

    const std::optional<std::string> assetPath = getDataAssetInspectorAssetPath(binding.parentField);
    if (!assetPath.has_value())
        return false;

    asset::editor::DataAssetEditorModel editorModel = asset::editor::loadDataAssetEditorModel(*assetPath);
    if (!editorModel.valid)
        return false;

    const asset::DataAssetDefinition previousDefinition = editorModel.definition;
    asset::DataAssetNodeDefinition* node = findDataAssetNodeByPath(editorModel.definition, binding.nodePath);
    if (node == nullptr || node->kind == asset::DataAssetValueKind::Group)
        return false;

    component_meta::SerializedValue parsedValue;
    if (!parseDataAssetFieldValue(asset::editor::fieldKindFromDataAssetValueKind(node->kind), value, parsedValue))
        return false;
    if (formatSerializedValueLocal(asset::editor::serializedValueFromDataAssetNode(*node)) == formatSerializedValueLocal(parsedValue))
        return false;

    switch (node->kind)
    {
    case asset::DataAssetValueKind::Bool:
    {
        const bool* parsedBool = std::get_if<bool>(&parsedValue);
        if (parsedBool == nullptr)
            return false;
        node->boolValue = *parsedBool;
        break;
    }
    case asset::DataAssetValueKind::Int:
    {
        const int* parsedInt = std::get_if<int>(&parsedValue);
        if (parsedInt == nullptr)
            return false;
        node->intValue = *parsedInt;
        break;
    }
    case asset::DataAssetValueKind::Float:
    {
        const float* parsedFloat = std::get_if<float>(&parsedValue);
        if (parsedFloat == nullptr)
            return false;
        node->floatValue = *parsedFloat;
        break;
    }
    case asset::DataAssetValueKind::Vec3:
    {
        const glm::vec3* parsedVec3 = std::get_if<glm::vec3>(&parsedValue);
        if (parsedVec3 == nullptr)
            return false;
        node->vec3Value = *parsedVec3;
        break;
    }
    case asset::DataAssetValueKind::String:
    {
        const std::string* parsedString = std::get_if<std::string>(&parsedValue);
        if (parsedString == nullptr)
            return false;
        node->stringValue = *parsedString;
        break;
    }
    case asset::DataAssetValueKind::Group:
        return false;
    }

    const std::string diskPath = asset::AssetManager::runtimePath(editorModel.normalizedAssetPath);
    if (!asset::DataAssetIO::saveDefinition(diskPath, editorModel.definition))
    {
        appendConsoleSystemMessage("[asset] Failed to save data asset: " + editorModel.normalizedAssetPath, "console_line_error");
        return false;
    }

    asset::DataAssetDefinition* reloadedDefinition = nullptr;
    if (!asset::AssetManager::instance().reloadDataAssetDefinition(editorModel.normalizedAssetPath, reloadedDefinition))
    {
        asset::DataAssetIO::saveDefinition(diskPath, previousDefinition);
        asset::AssetManager::instance().reloadDataAssetDefinition(editorModel.normalizedAssetPath, reloadedDefinition);
        appendConsoleSystemMessage("[asset] Failed to reload data asset: " + editorModel.normalizedAssetPath, "console_line_error");
        return false;
    }

    if (isExternalPreviewActive())
    {
        if (!writeRuntimePreviewDataAssetFile(
                runtime_preview::dataAssetMetadataTempPath(),
                runtime_preview::dataAssetMetadataPath(),
                m_runtimeDataAssetSyncSequence + 1,
                editorModel.normalizedAssetPath))
        {
            appendConsoleSystemMessage("[play] Failed to sync data asset to preview: " + editorModel.normalizedAssetPath, "console_line_error");
        }
        else
        {
            ++m_runtimeDataAssetSyncSequence;
        }
    }

    return true;
}

void SceneEditorController::toggleDataAssetEditor(const InspectorFieldBinding& binding)
{
    const std::string editorId = makeDataAssetEditorGroupElementId(binding);
    const auto it = m_collapsedDataAssetEditors.find(editorId);
    if (it != m_collapsedDataAssetEditors.end())
        m_collapsedDataAssetEditors.erase(it);
    else
        m_collapsedDataAssetEditors.insert(editorId);
}

bool SceneEditorController::isDataAssetEditorCollapsed(const InspectorFieldBinding& binding) const
{
    return m_collapsedDataAssetEditors.count(makeDataAssetEditorGroupElementId(binding)) > 0;
}

void SceneEditorController::toggleDataAssetEditorNode(const DataAssetEditorBinding& binding)
{
    const std::string groupId = makeDataAssetEditorNodeGroupElementId(binding.parentField, binding.nodePath);
    const auto it = m_collapsedDataAssetEditorNodes.find(groupId);
    if (it != m_collapsedDataAssetEditorNodes.end())
        m_collapsedDataAssetEditorNodes.erase(it);
    else
        m_collapsedDataAssetEditorNodes.insert(groupId);
}

bool SceneEditorController::isDataAssetEditorNodeCollapsed(const DataAssetEditorBinding& binding) const
{
    return m_collapsedDataAssetEditorNodes.count(makeDataAssetEditorNodeGroupElementId(binding.parentField, binding.nodePath)) > 0;
}

std::string SceneEditorController::buildDataAssetEditorMarkup(const InspectorFieldBinding& binding, const std::string& assetPath) const
{
    const bool collapsed = isDataAssetEditorCollapsed(binding);
    const std::string groupId = makeDataAssetEditorGroupElementId(binding);
    const std::string iconId = makeDataAssetEditorIconElementId(binding);
    const std::string bodyId = makeDataAssetEditorBodyElementId(binding);

    std::ostringstream stream;
    stream << "<div class='inspector_foldout inspector_asset_editor_foldout'>";
    stream << "<div id='" << groupId << "' class='inspector_foldout_header inspector_asset_editor_header'>";
    stream << "<div id='" << iconId << "' class='inspector_foldout_icon'>" << (collapsed ? ">" : "v") << "</div>";
    stream << "<div class='inspector_foldout_title'>Data Asset</div>";
    stream << "</div>";
    stream << "<div id='" << bodyId << "' class='inspector_foldout_body inspector_asset_editor_body'>";
    if (!collapsed)
        stream << buildDataAssetEditorBodyMarkup(binding, assetPath);
    stream << "</div></div>";
    return stream.str();
}

std::string SceneEditorController::buildDataAssetEditorBodyMarkup(const InspectorFieldBinding& binding, const std::string& assetPath) const
{
    const asset::editor::DataAssetEditorModel editorModel = asset::editor::loadDataAssetEditorModel(assetPath);
    if (!editorModel.available)
        return "<div class='inspector_asset_hint'>Assign a .data asset to edit its values.</div>";
    if (!editorModel.valid)
        return "<div class='inspector_asset_hint'>" + editor_ui::escapeRmlText(editorModel.errorMessage) + "</div>";

    std::ostringstream stream;
    stream << "<div class='inspector_summary_text'>" << editor_ui::escapeRmlText(editorModel.normalizedAssetPath) << "</div>";
    stream << "<div class='inspector_summary_text'>root: " << editor_ui::escapeRmlText(editorModel.definition.rootName) << "</div>";

    if (editorModel.definition.children.empty())
    {
        stream << "<div class='inspector_asset_hint'>This data asset has no nodes yet.</div>";
        return stream.str();
    }

    for (size_t childIndex = 0; childIndex < editorModel.definition.children.size(); ++childIndex)
        stream << buildDataAssetEditorNodeMarkup(binding, editorModel.definition.children[childIndex], makeDataAssetNodePath("", childIndex), 0);

    return stream.str();
}

std::string SceneEditorController::buildDataAssetEditorNodeMarkup(const InspectorFieldBinding& binding, const asset::DataAssetNodeDefinition& node, const std::string& nodePath, int depth) const
{
    std::ostringstream stream;
    const int indentPixels = depth * 14;

    if (node.kind == asset::DataAssetValueKind::Group)
    {
        DataAssetEditorBinding nodeBinding;
        nodeBinding.parentField = binding;
        nodeBinding.nodePath = nodePath;
        const bool collapsed = isDataAssetEditorNodeCollapsed(nodeBinding);
        const std::string groupId = makeDataAssetEditorNodeGroupElementId(binding, nodePath);

        stream << "<div class='inspector_foldout inspector_asset_editor_foldout' style='margin-left: " << indentPixels << "px;'>";
        stream << "<div id='" << groupId << "' class='inspector_foldout_header inspector_asset_editor_header'>";
        stream << "<div class='inspector_foldout_icon'>" << (collapsed ? ">" : "v") << "</div>";
        stream << "<div class='inspector_foldout_title'>" << editor_ui::escapeRmlText(node.name) << "</div>";
        stream << "</div>";
        if (!collapsed)
        {
            stream << "<div class='inspector_foldout_body inspector_asset_editor_body'>";
            if (node.children.empty())
            {
                stream << "<div class='inspector_asset_hint'>Empty group.</div>";
            }
            else
            {
                for (size_t childIndex = 0; childIndex < node.children.size(); ++childIndex)
                    stream << buildDataAssetEditorNodeMarkup(binding, node.children[childIndex], makeDataAssetNodePath(nodePath, childIndex), depth + 1);
            }
            stream << "</div>";
        }
        stream << "</div>";
        return stream.str();
    }

    stream << "<div style='margin-left: " << indentPixels << "px;'>";
    stream << buildInspectorFieldMarkup(
        makeDataAssetEditorFieldElementId(binding, nodePath),
        node.name + " (" + dataAssetNodeKindLabel(node.kind) + ")",
        asset::editor::fieldKindFromDataAssetValueKind(node.kind),
        asset::editor::serializedValueFromDataAssetNode(node),
        {},
        component_meta::AssetReferenceKind::None,
        false);
    stream << "</div>";
    return stream.str();
}

void SceneEditorController::refreshDataAssetEditorPresentation(const InspectorFieldBinding& binding)
{
    if (m_document == nullptr || !isDataAssetInspectorField(binding))
        return;

    const std::optional<std::string> currentAssetPath = getDataAssetInspectorAssetPath(binding);
    if (!currentAssetPath.has_value())
        return;

    if (Rml::Element* icon = m_document->GetElementById(makeDataAssetEditorIconElementId(binding)))
        icon->SetInnerRML(isDataAssetEditorCollapsed(binding) ? ">" : "v");

    if (Rml::Element* body = m_document->GetElementById(makeDataAssetEditorBodyElementId(binding)))
    {
        if (isDataAssetEditorCollapsed(binding))
        {
            body->SetInnerRML("");
            body->SetAttribute("data-data-structure", "");
            return;
        }

        const asset::editor::DataAssetEditorModel editorModel = asset::editor::loadDataAssetEditorModel(*currentAssetPath);
        const std::string nextStructureSignature = buildDataAssetEditorStructureSignature(editorModel);
        const std::string currentStructureSignature = body->GetAttribute<Rml::String>("data-data-structure", "").c_str();

        if (currentStructureSignature != nextStructureSignature)
        {
            if (elementIsFocusedOrContainsFocus(m_context, body))
                return;

            body->SetInnerRML(buildDataAssetEditorBodyMarkup(binding, *currentAssetPath));
            body->SetAttribute("data-data-structure", nextStructureSignature);
            return;
        }

        if (!editorModel.valid)
            return;

        const auto patchNodeValues = [&](const auto& self, const asset::DataAssetNodeDefinition& node, const std::string& nodePath) -> void {
            if (node.kind == asset::DataAssetValueKind::Group)
            {
                for (size_t childIndex = 0; childIndex < node.children.size(); ++childIndex)
                    self(self, node.children[childIndex], makeDataAssetNodePath(nodePath, childIndex));
                return;
            }

            patchDataAssetEditorFieldValue(
                m_document,
                m_context,
                makeDataAssetEditorFieldElementId(binding, nodePath),
                formatSerializedValueLocal(asset::editor::serializedValueFromDataAssetNode(node)));
        };

        for (size_t childIndex = 0; childIndex < editorModel.definition.children.size(); ++childIndex)
            patchNodeValues(patchNodeValues, editorModel.definition.children[childIndex], makeDataAssetNodePath("", childIndex));
    }
}

void SceneEditorController::pollRuntimePreviewDataAssetState()
{
    if (m_activeProcessKind != ActiveProcessKind::Player || m_scene == nullptr)
        return;

    std::ifstream input(runtime_preview::dataAssetStatePath());
    if (!input)
        return;

    uint64_t nextSequence = 0;
    if (!(input >> nextSequence) || nextSequence <= m_runtimeDataAssetStateSequence)
        return;

    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::string dataAssetPath;
    if (!std::getline(input, dataAssetPath) || dataAssetPath.empty())
        return;

    std::ostringstream definitionStream;
    definitionStream << input.rdbuf();

    asset::DataAssetDefinition definition;
    if (!asset::DataAssetIO::loadDefinitionFromContent(definitionStream.str(), definition))
        return;

    const std::string normalizedDataAssetPath = asset::AssetManager::normalizeRelativePath(dataAssetPath);
    if (normalizedDataAssetPath.empty())
        return;

    const std::string diskPath = asset::AssetManager::runtimePath(normalizedDataAssetPath);
    if (!asset::DataAssetIO::saveDefinition(diskPath, definition))
    {
        appendConsoleSystemMessage("[play] Failed to persist preview data asset state: " + normalizedDataAssetPath, "console_line_error");
        return;
    }

    asset::DataAssetDefinition* reloadedDefinition = nullptr;
    if (!asset::AssetManager::instance().reloadDataAssetDefinition(normalizedDataAssetPath, reloadedDefinition))
    {
        appendConsoleSystemMessage("[play] Failed to reload preview data asset state: " + normalizedDataAssetPath, "console_line_error");
        return;
    }

    refreshVisibleDataAssetEditorsForAsset(normalizedDataAssetPath);

    m_runtimeDataAssetStateSequence = nextSequence;
}