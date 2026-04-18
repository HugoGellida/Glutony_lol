#pragma once

#include <GL/glew.h>

#include "SceneRenderTargetSettings.hpp"

#include <algorithm>
#include <vector>

namespace render
{
class RenderTargetResource
{
private:
    GLuint m_framebuffer = 0;
    GLuint m_colorTexture = 0;
    GLuint m_depthTexture = 0;
    GLuint m_boundDepthAttachment = 0;
    int m_width = 0;
    int m_height = 0;
    RenderTargetFormat m_format = RenderTargetFormat::Rgba;

    static GLint colorInternalFormat(RenderTargetFormat format)
    {
        switch (format)
        {
        case RenderTargetFormat::Float:
            return GL_R32F;
        case RenderTargetFormat::Rg:
            return GL_RG8;
        case RenderTargetFormat::Rgb:
            return GL_RGB8;
        case RenderTargetFormat::Rgba:
            return GL_RGBA8;
        case RenderTargetFormat::Rgba16f:
            return GL_RGBA16F;
        default:
            return GL_RGBA8;
        }
    }

    static GLenum colorFormat(RenderTargetFormat format)
    {
        switch (format)
        {
        case RenderTargetFormat::Float:
            return GL_RED;
        case RenderTargetFormat::Rg:
            return GL_RG;
        case RenderTargetFormat::Rgb:
            return GL_RGB;
        case RenderTargetFormat::Rgba:
        case RenderTargetFormat::Rgba16f:
        default:
            return GL_RGBA;
        }
    }

    static GLenum colorType(RenderTargetFormat format)
    {
        switch (format)
        {
        case RenderTargetFormat::Float:
        case RenderTargetFormat::Rgba16f:
            return GL_FLOAT;
        case RenderTargetFormat::Rg:
        case RenderTargetFormat::Rgb:
        case RenderTargetFormat::Rgba:
        default:
            return GL_UNSIGNED_BYTE;
        }
    }

    void moveFrom(RenderTargetResource& other)
    {
        m_framebuffer = other.m_framebuffer;
        m_colorTexture = other.m_colorTexture;
        m_depthTexture = other.m_depthTexture;
        m_boundDepthAttachment = other.m_boundDepthAttachment;
        m_width = other.m_width;
        m_height = other.m_height;
        m_format = other.m_format;

        other.m_framebuffer = 0;
        other.m_colorTexture = 0;
        other.m_depthTexture = 0;
        other.m_boundDepthAttachment = 0;
        other.m_width = 0;
        other.m_height = 0;
    }

    void bindInternal(GLuint depthAttachment)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
        glViewport(0, 0, m_width, m_height);

        const GLuint resolvedDepthAttachment = depthAttachment != 0 ? depthAttachment : m_depthTexture;
        if (m_boundDepthAttachment != resolvedDepthAttachment)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, resolvedDepthAttachment, 0);
            m_boundDepthAttachment = resolvedDepthAttachment;
        }
    }

public:
    RenderTargetResource() = default;
    RenderTargetResource(const RenderTargetResource&) = delete;
    RenderTargetResource& operator=(const RenderTargetResource&) = delete;

    RenderTargetResource(RenderTargetResource&& other) noexcept
    {
        moveFrom(other);
    }

    RenderTargetResource& operator=(RenderTargetResource&& other) noexcept
    {
        if (this != &other)
        {
            release();
            moveFrom(other);
        }

        return *this;
    }

    bool ensure(int width, int height, RenderTargetFormat format)
    {
        width = std::max(width, 1);
        height = std::max(height, 1);
        if (m_framebuffer != 0 && m_width == width && m_height == height && m_format == format)
            return true;

        release();

        glGenFramebuffers(1, &m_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

        glGenTextures(1, &m_colorTexture);
        glBindTexture(GL_TEXTURE_2D, m_colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, colorInternalFormat(format), width, height, 0, colorFormat(format), colorType(format), nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);

        glGenTextures(1, &m_depthTexture);
        glBindTexture(GL_TEXTURE_2D, m_depthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);

        const GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
        {
            release();
            return false;
        }

        m_width = width;
        m_height = height;
        m_format = format;
        m_boundDepthAttachment = m_depthTexture;
        return true;
    }

    void bind() const
    {
        const_cast<RenderTargetResource*>(this)->bindInternal(m_depthTexture);
    }

    void bindWithDepthOverride(GLuint depthTextureId) const
    {
        const_cast<RenderTargetResource*>(this)->bindInternal(depthTextureId);
    }

    GLuint colorTextureId() const
    {
        return m_colorTexture;
    }

    GLuint depthTextureId() const
    {
        return m_depthTexture;
    }

    int width() const
    {
        return m_width;
    }

    int height() const
    {
        return m_height;
    }

    RenderTargetFormat format() const
    {
        return m_format;
    }

    bool readColorRgba8(std::vector<unsigned char>& pixelsOut) const
    {
        if (m_framebuffer == 0 || m_width <= 0 || m_height <= 0)
            return false;

        pixelsOut.assign(static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * 4U, 0);

        GLint previousFramebuffer = 0;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousFramebuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, pixelsOut.data());
        glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));

        return glGetError() == GL_NO_ERROR;
    }

    bool readColorFloat(std::vector<float>& pixelsOut) const
    {
        if (m_framebuffer == 0 || m_width <= 0 || m_height <= 0 || m_format != RenderTargetFormat::Float)
            return false;

        pixelsOut.assign(static_cast<size_t>(m_width) * static_cast<size_t>(m_height), 0.0f);

        GLint previousFramebuffer = 0;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousFramebuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, m_width, m_height, GL_RED, GL_FLOAT, pixelsOut.data());
        glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));

        return glGetError() == GL_NO_ERROR;
    }

    void release()
    {
        if (m_depthTexture != 0)
            glDeleteTextures(1, &m_depthTexture);
        if (m_colorTexture != 0)
            glDeleteTextures(1, &m_colorTexture);
        if (m_framebuffer != 0)
            glDeleteFramebuffers(1, &m_framebuffer);

        m_depthTexture = 0;
        m_colorTexture = 0;
        m_boundDepthAttachment = 0;
        m_framebuffer = 0;
        m_width = 0;
        m_height = 0;
    }

    ~RenderTargetResource()
    {
        release();
    }
};
}
