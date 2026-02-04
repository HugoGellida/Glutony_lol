#pragma once

#include <string>
#include "external/stb_image/stb_image.h"
#include <iostream>

class Texture2D
{
private:
    bool m_on_GPU = false;
    bool m_empty = false;
    int m_width = 0;
    int m_height = 0;
    int m_nbChannels = 0;
    GLuint m_textureID;
    std::vector<unsigned char> m_data;
    GLuint m_slot;
public:
      
    Texture2D(){m_empty = true; m_slot = 0;} // ONLY FOR UNINITIALIZED TEXTURES - FORBIDDEN USE ON SHADERS
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

        // send tex to GPU
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, m_data.data());
        // genMipmaps
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    Texture2D(const std::string path, const GLuint slot)
    {
        unsigned char * new_data = stbi_load(path.c_str(), &m_width, &m_height, &m_nbChannels, 3); // force text to be 3 channels
        if (!new_data)
        {
            std::cerr << "[Texture2D] Failed to load texture from path: "<< path << " (Unable to load data)" << std::endl;
            return;
        }

        m_slot = slot;

        m_data.resize(m_width * m_height * m_nbChannels);
        memcpy(m_data.data(), new_data, m_width * m_height * 3);
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



    ~Texture2D()
    {
        // TODO
        if (!m_on_GPU || m_empty)
            return;
        glDeleteTextures(1, &m_textureID);

        m_on_GPU = false;
    }
};