#pragma once

#include <cstring>
#include <string>
#include <vector>

#include "external/stb_image/stb_image.h"

#include <iostream>

class Texture2D
{
private:
    bool m_on_GPU = false;
    bool m_empty = false;
    bool m_ownsGpuResource = false;
    int m_width = 0;
    int m_height = 0;
    int m_nbChannels = 0;
    GLuint m_textureID = 0;
    std::vector<unsigned char> m_data;
    GLuint m_slot = 0;

    void releaseOwnedGpuResource()
    {
        if (!m_on_GPU || m_empty || !m_ownsGpuResource)
            return;

        glDeleteTextures(1, &m_textureID);
        m_textureID = 0;
        m_on_GPU = false;
        m_ownsGpuResource = false;
    }

    void resetToEmptyState()
    {
        m_on_GPU = false;
        m_empty = true;
        m_ownsGpuResource = false;
        m_width = 0;
        m_height = 0;
        m_nbChannels = 0;
        m_textureID = 0;
        m_slot = 0;
        m_data.clear();
    }

    void moveFrom(Texture2D& other)
    {
        m_on_GPU = other.m_on_GPU;
        m_empty = other.m_empty;
        m_ownsGpuResource = other.m_ownsGpuResource;
        m_width = other.m_width;
        m_height = other.m_height;
        m_nbChannels = other.m_nbChannels;
        m_textureID = other.m_textureID;
        m_data = std::move(other.m_data);
        m_slot = other.m_slot;

        other.resetToEmptyState();
    }
public:

    Texture2D()
    {
        m_empty = true;
        m_slot = 0;
    }

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    Texture2D(Texture2D&& other) noexcept
    {
        moveFrom(other);
    }

    Texture2D& operator=(Texture2D&& other) noexcept
    {
        if (this == &other)
            return *this;

        releaseOwnedGpuResource();
        moveFrom(other);
        return *this;
    }

    Texture2D(GLuint textureId, GLuint slot, bool ownsGpuResource = false)
    {
        m_textureID = textureId;
        m_slot = slot;
        m_empty = (textureId == 0);
        m_on_GPU = (textureId != 0);
        m_ownsGpuResource = ownsGpuResource;
    }
    Texture2D(const unsigned int width, const unsigned int height)
    {
        // TODO
        m_slot = 0;
    }

    void sync()
    {

        if (m_empty || m_on_GPU)
            return;
        glGenTextures(1, &m_textureID);
        glBindTexture(GL_TEXTURE_2D, m_textureID);
        
        // tex params
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // send tex to GPU
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_data.data());
        // genMipmaps
        glGenerateMipmap(GL_TEXTURE_2D);
        m_on_GPU = true;
        m_ownsGpuResource = true;
    }

    Texture2D(const std::string path, const GLuint slot)
    {
        m_slot = slot;

        unsigned char * new_data = stbi_load(path.c_str(), &m_width, &m_height, &m_nbChannels, 4);
        if (!new_data)
        {
            m_empty = true;
            std::cerr << "[Texture2D] Failed to load texture from path: "<< path << " (Unable to load data)" << std::endl;
            return;
        }

        m_empty = false;
        m_nbChannels = 4;
        m_data.resize(static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * static_cast<size_t>(m_nbChannels));
        std::memcpy(m_data.data(), new_data, m_data.size());
        stbi_image_free(new_data);

        sync();
    }


    void bind(const GLuint & progID, const char* name) const
    {
        if (m_empty)
            return;
        glActiveTexture(GL_TEXTURE0 + m_slot);
        glUniform1i(glGetUniformLocation(progID, name), m_slot);
        glBindTexture(GL_TEXTURE_2D, m_textureID);
    }

    bool isEmpty() const
    {
        return m_empty;
    }

    GLuint textureId() const
    {
        return m_textureID;
    }



    ~Texture2D()
    {
        releaseOwnedGpuResource();
    }
};