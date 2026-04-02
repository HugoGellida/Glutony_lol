#include "EditorUi.hpp"

#include "EditorUiDispatcher.hpp"

EditorUiController::EditorUiController()
    : m_dispatcher(std::make_unique<EditorUiDispatcher>())
{
}

EditorUiController::~EditorUiController() = default;

bool EditorUiController::initialize(Rml::Context* context)
{
    return m_dispatcher != nullptr && m_dispatcher->initialize(context);
}

void EditorUiController::sync(Scene& scene)
{
    if (m_dispatcher != nullptr)
        m_dispatcher->sync(scene);
}

void EditorUiController::shutdown()
{
    if (m_dispatcher != nullptr)
        m_dispatcher->shutdown();
}

void EditorUiController::syncToWindow(int width, int height)
{
    if (m_dispatcher != nullptr)
        m_dispatcher->syncToWindow(width, height);
}

void EditorUiController::setUiBuilderEnabled(bool enabled)
{
    if (m_dispatcher != nullptr)
        m_dispatcher->setUiBuilderEnabled(enabled);
}

bool EditorUiController::isUiBuilderEnabled() const
{
    return m_dispatcher != nullptr && m_dispatcher->isUiBuilderEnabled();
}

void EditorUiController::setUiBuilderShowStylePanel(bool showStylePanel)
{
    if (m_dispatcher != nullptr)
        m_dispatcher->setUiBuilderShowStylePanel(showStylePanel);
}

void EditorUiController::update()
{
    if (m_dispatcher != nullptr)
        m_dispatcher->update();
}

void EditorUiController::render()
{
    if (m_dispatcher != nullptr)
        m_dispatcher->render();
}

UiRect EditorUiController::getViewportRect() const
{
    return m_dispatcher != nullptr ? m_dispatcher->getViewportRect() : UiRect{};
}

bool EditorUiController::isViewportHovered(double mouseX, double mouseY) const
{
    return m_dispatcher != nullptr && m_dispatcher->isViewportHovered(mouseX, mouseY);
}

bool EditorUiController::isDragging() const
{
    return m_dispatcher != nullptr && m_dispatcher->isDragging();
}

bool EditorUiController::isExternalPreviewActive() const
{
    return m_dispatcher != nullptr && m_dispatcher->isExternalPreviewActive();
}

void EditorUiController::ProcessEvent(Rml::Event& event)
{
    if (m_dispatcher != nullptr)
        m_dispatcher->ProcessEvent(event);
}