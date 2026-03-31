#include "SceneEditorController.hpp"

#include "EditorUiDocuments.hpp"

#include <common/Scene.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <iomanip>
#include <limits>
#include <sstream>
#include <unordered_set>

using namespace editor_ui;

namespace
{
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

std::string buildInspectorFieldMarkup(
    const std::string& fieldId,
    const std::string& fieldLabel,
    component_meta::FieldKind fieldKind,
    const component_meta::SerializedValue& value,
    const std::vector<component_meta::EnumOption>& enumOptions)
{
    std::ostringstream stream;
    stream << "<div class='inspector_field_row'><div class='inspector_field_name'>" << escapeRmlText(fieldLabel) << "</div>";

    const std::string formattedValue = formatSerializedValue(value);

    if (fieldKind == component_meta::FieldKind::Bool)
    {
        stream << "<select id='" << fieldId << "' class='inspector_field_input'>";
        stream << "<option value='true'" << (formattedValue == "true" ? " selected='selected'" : "") << ">True</option>";
        stream << "<option value='false'" << (formattedValue == "false" ? " selected='selected'" : "") << ">False</option>";
        stream << "</select>";
    }
    else if (fieldKind == component_meta::FieldKind::Enum)
    {
        stream << "<select id='" << fieldId << "' class='inspector_field_input'>";
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
        stream << "<input id='" << fieldId << "' class='inspector_field_input' type='";
        stream << ((fieldKind == component_meta::FieldKind::Float || fieldKind == component_meta::FieldKind::Int) ? "number" : "text");
        stream << "' value='" << escapeRmlText(formattedValue) << "' />";
    }

    stream << "</div>";
    return stream.str();
}

std::string buildInspectorFieldMarkup(
    int nodeId,
    size_t componentIndex,
    const component_meta::ComponentFieldDescriptor& field,
    const component_meta::SerializedValue& value)
{
    const std::string fieldId = makeSceneInspectorFieldElementId(nodeId, componentIndex, field.key);
    return buildInspectorFieldMarkup(fieldId, field.label, field.kind, value, field.enumOptions);
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
}

bool SceneEditorController::initialize(Rml::Context* context)
{
    m_context = context;
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
    m_rightSplitter = nullptr;
    m_rightPanel = nullptr;
    m_isWindowMenuOpen = false;
    m_scene = nullptr;
    m_dragTarget = DragTarget::None;
    m_viewportRect = {};
    m_centerRect = {};
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
    const Rml::String windowMenuButtonElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "builder_menu_window_button"; });
    const Rml::String openUiBuilderElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return candidateId == "builder_menu_open_ui_builder"; });
    const Rml::String hierarchyNodeElementId = ::findAncestorElementId(
        targetElement,
        [&](const Rml::String& candidateId) { return parseHierarchyNodeId(candidateId).has_value(); });
    const Rml::String inspectorGroupElementId = ::findAncestorElementId(
        targetElement,
        [](const Rml::String& candidateId) { return startsWith(std::string(candidateId), "scene_inspector_group_"); });

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
            refreshInspectorValuesPresentation();

        event.StopPropagation();
        return;
    }

    if (eventId == Rml::EventId::Click)
    {
        if (!playButtonElementId.empty())
        {
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
                m_scene->setPhysicsSimulationEnabled(false);
            m_playbackState = PlaybackState::Stopped;
            refreshPresentation();
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
            m_isWindowMenuOpen = !m_isWindowMenuOpen;
            refreshPresentation();
            event.StopPropagation();
            return;
        }

        if (!openUiBuilderElementId.empty())
        {
            m_isWindowMenuOpen = false;
            refreshPresentation();
            if (m_modeChangeCallback)
                m_modeChangeCallback(EditorMode::UiBuilder);
            event.StopPropagation();
            return;
        }

        if (const std::optional<int> hierarchyNodeId = parseHierarchyNodeId(hierarchyNodeElementId))
        {
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

        if (m_isWindowMenuOpen)
        {
            const Rml::String menuHit = ::findAncestorElementId(targetElement, [](const Rml::String& candidateId) {
                return candidateId == "builder_menu_window";
            });
            if (menuHit.empty())
            {
                m_isWindowMenuOpen = false;
                refreshPresentation();
            }
        }
        return;
    }

    if (eventId == Rml::EventId::Mousedown)
    {
        if (elementId == "left_splitter")
            m_dragTarget = DragTarget::LeftSplitter;
        else if (elementId == "right_splitter")
            m_dragTarget = DragTarget::RightSplitter;
        else if (elementId == "horizontal_splitter")
            m_dragTarget = DragTarget::HorizontalSplitter;

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
    }
}

void SceneEditorController::attachListeners()
{
    if (m_document == nullptr)
        return;

    m_document->AddEventListener(Rml::EventId::Change, this);
    m_document->AddEventListener(Rml::EventId::Blur, this, true);
    m_document->AddEventListener(Rml::EventId::Click, this);
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
}

void SceneEditorController::refreshPresentation()
{
    if (m_builderHeader == nullptr || m_leftPanel == nullptr || m_rightPanel == nullptr || m_bottomPanel == nullptr)
        return;

    m_builderHeader->SetInnerRML(
        std::string(
            R"RML(<div class='builder_menu_bar'><div id='builder_menu_window' class='builder_menu'><div id='builder_menu_window_button' class='builder_menu_button'>Window</div><div id='builder_menu_window_dropdown' class='builder_menu_dropdown' style='display: )RML") +
        (m_isWindowMenuOpen ? "block" : "none") +
        R"RML(;'><div id='builder_menu_open_ui_builder' class='builder_menu_item'>UI Builder</div></div></div></div>)RML"
    );

    m_leftPanel->SetInnerRML(
        buildHierarchyMarkup()
    );
    m_viewportPanel->SetInnerRML(buildViewportMarkup());
    m_viewportSurface = m_document->GetElementById("scene_viewport_surface");
    m_rightPanel->SetInnerRML(
        buildInspectorMarkup()
    );
    m_bottomPanel->SetInnerRML(
        R"RML(<div class='panel_shell'><div class='panel_header'>Bottom Panel</div><div class='panel_body'><div class='placeholder_block'><div class='placeholder_title'>Logs / Assets / Timeline</div><div class='placeholder_text'>This region stays available for the classic editor layout.</div></div></div></div>)RML"
    );
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
    stream << "<div class='panel_shell hierarchy_shell'><div class='panel_header'>Scene</div><div class='panel_body hierarchy_body'>";

    if (m_hierarchyRoot.children.empty())
    {
        stream << "<div class='placeholder_block'><div class='placeholder_title'>Scene hierarchy</div><div class='placeholder_text'>No game objects found in the scene.</div></div>";
    }
    else
    {
        for (const UiGOHierarchyNode& child : m_hierarchyRoot.children)
            stream << buildHierarchyNodeMarkup(child, 0);
    }

    stream << "</div></div>";
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
        {}
    );
    stream << buildInspectorFieldMarkup(
        makeTransformFieldElementId(selectedNode->id, "rotation"),
        "Rotation",
        component_meta::FieldKind::Vec3,
        rotation,
        {}
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

                stream << buildInspectorFieldMarkup(selectedNode->id, componentIndex, field, field.read(*component));
            }
            stream << "</div>";
        }
        stream << "</div>";
    }

    if (serializableComponentCount == 0)
    {
        stream << "<div class='placeholder_block'><div class='placeholder_title'>Serializable components</div><div class='placeholder_text'>No serializable component descriptor is registered on this game object yet.</div></div>";
    }

    stream << "</div></div>";
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
    if (m_playbackState == PlaybackState::Playing)
        stream << "<div class='scene_playback_status'>Physics running</div>";
    else if (m_playbackState == PlaybackState::Paused)
        stream << "<div class='scene_playback_status'>Physics paused</div>";
    else
        stream << "<div class='scene_playback_status'>Physics stopped</div>";
    stream << "</div></div>";
    stream << "<div id='scene_viewport_surface' class='scene_viewport_surface'></div>";
    stream << "</div>";
    return stream.str();
}

std::string SceneEditorController::makeHierarchyNodeElementId(int nodeId)
{
    return "hierarchy_node_" + std::to_string(nodeId);
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