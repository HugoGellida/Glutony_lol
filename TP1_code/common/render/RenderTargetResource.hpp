#pragma once

#include <GL/glew.h>

#include "SceneRenderTargetSettings.hpp"

#include <algorithm>

namespace render
{
class RenderTargetResource
{
private:
    GLuint m_framebuffer = 0;
    GLuint m_colorTexture = 0;
    GLuint m_depthTexture = 0;
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
        default:
            return GL_RGBA;
        }
    }

    static GLenum colorType(RenderTargetFormat format)
    {
        return format == RenderTargetFormat::Float ? GL_FLOAT : GL_UNSIGNED_BYTE;
    }

    void moveFrom(RenderTargetResource& other)
    {
        m_framebuffer = other.m_framebuffer;
        m_colorTexture = other.m_colorTexture;
        m_depthTexture = other.m_depthTexture;
        m_width = other.m_width;
        m_height = other.m_height;
        m_format = other.m_format;

        other.m_framebuffer = 0;
        other.m_colorTexture = 0;
        other.m_depthTexture = 0;
        other.m_width = 0;
        other.m_height = 0;
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
        return true;
    }

    void bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
        glViewport(0, 0, m_width, m_height);
    }

    GLuint colorTextureId() const
    {
        return m_colorTexture;
    }

    GLuint depthTextureId() const
    {
        return m_depthTexture;
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