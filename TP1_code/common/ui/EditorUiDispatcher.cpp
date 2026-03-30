#include "EditorUiDispatcher.hpp"

#include "EditorUiModeController.hpp"
#include "SceneEditorController.hpp"
#include "UiBuilderController.hpp"

EditorUiDispatcher::EditorUiDispatcher()
    : m_sceneEditor(std::make_unique<SceneEditorController>())
    , m_uiBuilder(std::make_unique<UiBuilderController>())
{
}

EditorUiDispatcher::~EditorUiDispatcher() = default;

bool EditorUiDispatcher::initialize(Rml::Context* context)
{
    if (context == nullptr)
        return false;

    m_context = context;
    m_sceneEditor->setModeChangeCallback([this](editor_ui::EditorMode mode) { setMode(mode); });
    m_uiBuilder->setModeChangeCallback([this](editor_ui::EditorMode mode) { setMode(mode); });

    if (!m_sceneEditor->initialize(context))
        return false;

    m_sceneEditor->activate();
    m_mode = editor_ui::EditorMode::SceneEditor;
    return true;
}

void EditorUiDispatcher::sync(Scene& scene)
{
    if (m_sceneEditor != nullptr)
        m_sceneEditor->sync(scene);
}

void EditorUiDispatcher::shutdown()
{
    if (m_sceneEditor != nullptr)
        m_sceneEditor->shutdown();
    if (m_uiBuilder != nullptr)
        m_uiBuilder->shutdown();
    m_context = nullptr;
}

void EditorUiDispatcher::syncToWindow(int width, int height)
{
    if (EditorUiModeController* controller = activeController())
        controller->syncToWindow(width, height);
}

void EditorUiDispatcher::setUiBuilderEnabled(bool enabled)
{
    setMode(enabled ? editor_ui::EditorMode::UiBuilder : editor_ui::EditorMode::SceneEditor);
}

bool EditorUiDispatcher::isUiBuilderEnabled() const
{
    return m_mode == editor_ui::EditorMode::UiBuilder;
}

void EditorUiDispatcher::setUiBuilderShowStylePanel(bool showStylePanel)
{
    if (m_uiBuilder != nullptr)
        m_uiBuilder->setShowStylePanel(showStylePanel);
}

void EditorUiDispatcher::update()
{
    if (EditorUiModeController* controller = activeController())
        controller->update();
}

void EditorUiDispatcher::render()
{
    if (EditorUiModeController* controller = activeController())
        controller->render();
}

UiRect EditorUiDispatcher::getViewportRect() const
{
    if (const EditorUiModeController* controller = activeController())
        return controller->getViewportRect();
    return {};
}

bool EditorUiDispatcher::isViewportHovered(double mouseX, double mouseY) const
{
    if (const EditorUiModeController* controller = activeController())
        return controller->isViewportHovered(mouseX, mouseY);
    return false;
}

bool EditorUiDispatcher::isDragging() const
{
    if (const EditorUiModeController* controller = activeController())
        return controller->isDragging();
    return false;
}

void EditorUiDispatcher::ProcessEvent(Rml::Event& event)
{
    if (EditorUiModeController* controller = activeController())
        controller->ProcessEvent(event);
}

EditorUiModeController* EditorUiDispatcher::activeController()
{
    return m_mode == editor_ui::EditorMode::UiBuilder
        ? static_cast<EditorUiModeController*>(m_uiBuilder.get())
        : static_cast<EditorUiModeController*>(m_sceneEditor.get());
}

const EditorUiModeController* EditorUiDispatcher::activeController() const
{
    return m_mode == editor_ui::EditorMode::UiBuilder
        ? static_cast<const EditorUiModeController*>(m_uiBuilder.get())
        : static_cast<const EditorUiModeController*>(m_sceneEditor.get());
}

void EditorUiDispatcher::setMode(editor_ui::EditorMode mode)
{
    if (m_context == nullptr || m_mode == mode)
        return;

    if (m_mode == editor_ui::EditorMode::UiBuilder)
        m_uiBuilder->shutdown();
    else
        m_sceneEditor->deactivate();

    m_mode = mode;

    if (m_mode == editor_ui::EditorMode::UiBuilder)
    {
        if (m_uiBuilder->initialize(m_context))
            m_uiBuilder->activate();
    }
    else
    {
        if (m_sceneEditor->initialize(m_context))
            m_sceneEditor->activate();
    }
}