#pragma once

#include <RmlUi/Core.h>

#include <functional>
#include <string>

class UIDocumentRef
{
public:
    UIDocumentRef() = default;
    explicit UIDocumentRef(Rml::ElementDocument* document) : m_document(document) {}

    Rml::ElementDocument* raw() const
    {
        return m_document;
    }

    Rml::Element* getElem(const std::string& elementName) const
    {
        if (m_document == nullptr)
            return nullptr;

        return m_document->GetElementById(elementName);
    }

    template <typename ElementType>
    ElementType* getElem(const std::string& elementName) const
    {
        return dynamic_cast<ElementType*>(getElem(elementName));
    }

private:
    Rml::ElementDocument* m_document = nullptr;
};

class UIRenderer
{
public:
    using BindCallback = std::function<void(UIRenderer&)>;
    using TickCallback = std::function<bool(UIRenderer&, double)>;

    UIRenderer() = default;
    ~UIRenderer() = default;

    bool initialize(Rml::Context* context);
    void shutdown();

    void setBindCallback(BindCallback callback);
    void setTickCallback(TickCallback callback);

    bool loadFromMemory(const std::string& source, const std::string& sourceUrl = "[ui-renderer]");
    bool loadFromFile(const std::string& filePath);
    bool reload();
    void unload();
    void update(double deltaTime);

    void setVisible(bool visible);
    void layoutFullscreen(int left, int top, int width, int height, int zIndex = 0);
    void pullToFront();

    UIDocumentRef uiDoc() const;
    Rml::Element* getElem(const std::string& elementName) const;
    template <typename ElementType>
    ElementType* getElem(const std::string& elementName) const
    {
        return uiDoc().getElem<ElementType>(elementName);
    }

    Rml::Context* context() const;
    Rml::ElementDocument* document() const;
    const std::string& source() const;
    const std::string& sourcePath() const;
    const std::string& sourceUrl() const;
    bool hasDocument() const;
    bool hasSource() const;

private:
    void configureDocumentBody(int width, int height);

    Rml::Context* m_context = nullptr;
    Rml::ElementDocument* m_document = nullptr;
    BindCallback m_bindCallback;
    TickCallback m_tickCallback;
    std::string m_source;
    std::string m_sourcePath;
    std::string m_sourceUrl;
};