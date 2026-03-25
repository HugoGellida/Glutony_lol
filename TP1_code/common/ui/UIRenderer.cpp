#include "UIRenderer.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

namespace
{
Rml::Element* findFirstElementByTagName(Rml::Element* root, const Rml::String& tagName)
{
    if (root == nullptr)
        return nullptr;

    if (root->GetTagName() == tagName)
        return root;

    for (int childIndex = 0; childIndex < root->GetNumChildren(true); ++childIndex)
    {
        if (Rml::Element* found = findFirstElementByTagName(root->GetChild(childIndex), tagName))
            return found;
    }

    return nullptr;
}

Rml::String pixels(int value)
{
    return Rml::String(std::to_string(value) + "px");
}
}

bool UIRenderer::initialize(Rml::Context* context)
{
    if (context == nullptr)
        return false;

    if (m_context == context)
        return true;

    shutdown();
    m_context = context;
    if (hasSource())
        return reload();
    return true;
}

void UIRenderer::shutdown()
{
    unload();
    m_context = nullptr;
}

void UIRenderer::setBindCallback(BindCallback callback)
{
    m_bindCallback = std::move(callback);
}

void UIRenderer::setTickCallback(TickCallback callback)
{
    m_tickCallback = std::move(callback);
}

bool UIRenderer::loadFromMemory(const std::string& source, const std::string& sourceUrl)
{
    m_source = source;
    m_sourcePath.clear();
    m_sourceUrl = sourceUrl.empty() ? "[ui-renderer]" : sourceUrl;
    return reload();
}

bool UIRenderer::loadFromFile(const std::string& filePath)
{
    std::ifstream stream(filePath);
    if (!stream.is_open())
    {
        std::cerr << "Unable to open UI document: " << filePath << std::endl;
        return false;
    }

    std::ostringstream buffer;
    buffer << stream.rdbuf();

    m_source = buffer.str();
    m_sourcePath = filePath;
    m_sourceUrl = filePath;
    return reload();
}

bool UIRenderer::reload()
{
    unload();

    if (m_context == nullptr || m_source.empty())
        return false;

    m_document = m_context->LoadDocumentFromMemory(m_source, m_sourceUrl.empty() ? "[ui-renderer]" : m_sourceUrl);
    if (m_document == nullptr)
    {
        std::cerr << "Failed to load UI document." << std::endl;
        return false;
    }

    m_document->Show();
    if (m_bindCallback)
        m_bindCallback(*this);
    m_document->UpdateDocument();
    return true;
}

void UIRenderer::unload()
{
    if (m_context != nullptr && m_document != nullptr)
        m_context->UnloadDocument(m_document);

    m_document = nullptr;
}

void UIRenderer::update(double deltaTime)
{
    if (m_document == nullptr)
        return;

    if (m_tickCallback && m_tickCallback(*this, deltaTime))
        m_document->UpdateDocument();
}

void UIRenderer::setVisible(bool visible)
{
    if (m_document != nullptr)
        m_document->SetProperty("display", visible ? "block" : "none");
}

void UIRenderer::layoutFullscreen(int left, int top, int width, int height, int zIndex)
{
    if (m_document == nullptr)
        return;

    m_document->SetProperty("display", "block");
    m_document->SetProperty("position", "absolute");
    m_document->SetProperty("left", pixels(left));
    m_document->SetProperty("top", pixels(top));
    m_document->SetProperty("width", pixels(width));
    m_document->SetProperty("height", pixels(height));
    m_document->SetProperty("margin", "0px");
    m_document->SetProperty("overflow", "hidden");
    m_document->SetProperty("background-color", "transparent");
    m_document->SetProperty("z-index", std::to_string(zIndex));
    configureDocumentBody(width, height);
    m_document->UpdateDocument();
}

void UIRenderer::pullToFront()
{
    if (m_document != nullptr)
        m_document->PullToFront();
}

UIDocumentRef UIRenderer::uiDoc() const
{
    return UIDocumentRef(m_document);
}

Rml::Element* UIRenderer::getElem(const std::string& elementName) const
{
    return uiDoc().getElem(elementName);
}

Rml::Context* UIRenderer::context() const
{
    return m_context;
}

Rml::ElementDocument* UIRenderer::document() const
{
    return m_document;
}

const std::string& UIRenderer::source() const
{
    return m_source;
}

const std::string& UIRenderer::sourcePath() const
{
    return m_sourcePath;
}

const std::string& UIRenderer::sourceUrl() const
{
    return m_sourceUrl;
}

bool UIRenderer::hasDocument() const
{
    return m_document != nullptr;
}

bool UIRenderer::hasSource() const
{
    return !m_source.empty();
}

void UIRenderer::configureDocumentBody(int width, int height)
{
    Rml::Element* bodyElement = findFirstElementByTagName(m_document, "body");
    if (bodyElement == nullptr)
        return;

    bodyElement->SetProperty("margin", "0px");
    bodyElement->SetProperty("position", "absolute");
    bodyElement->SetProperty("left", pixels(0));
    bodyElement->SetProperty("top", pixels(0));
    bodyElement->SetProperty("width", pixels(width));
    bodyElement->SetProperty("height", pixels(height));
    bodyElement->SetProperty("overflow", "hidden");
}