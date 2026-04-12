#ifndef MSHADER_HPP // port from glutony
#define MSHADER_HPP


#include <string.h>
#include <cstdint>
#include <filesystem>
#include <vector>
#include <iostream>
#include <fstream>
#include "Uniforms.hpp"


#include <GL/glew.h>

class Shader
{
public:
    enum class EditableUniformKind
    {
        Bool,
        Int,
        Float,
        Vec3,
        Texture,
        Unsupported,
    };

    struct UniformDescriptor
    {
        std::string name;
        EditableUniformKind kind = EditableUniformKind::Unsupported;
        GLenum glType = 0;
        GLint size = 1;

        bool supported() const
        {
            return kind != EditableUniformKind::Unsupported && size == 1;
        }
    };

private:
    std::string vPath;
    std::string fPath;
    std::string m_assetPath;
    GLuint programID = 0;
    bool loaded = false;
    bool m_hasObservedSourceWriteTimes = false;
    std::vector<UniformDescriptor> m_uniformDescriptors;
    uint64_t m_reloadGeneration = 0;
    std::filesystem::file_time_type m_vertexWriteTime{};
    std::filesystem::file_time_type m_fragmentWriteTime{};

    static EditableUniformKind classifyUniformKind(GLenum glType)
    {
        switch (glType)
        {
        case GL_BOOL:
            return EditableUniformKind::Bool;
        case GL_INT:
            return EditableUniformKind::Int;
        case GL_FLOAT:
            return EditableUniformKind::Float;
        case GL_FLOAT_VEC3:
            return EditableUniformKind::Vec3;
        case GL_SAMPLER_2D:
            return EditableUniformKind::Texture;
        default:
            return EditableUniformKind::Unsupported;
        }
    }

    static bool isEngineUniform(const std::string& name)
    {
        return name == "MVP" || name == "MVP_ORTHO";
    }

    static std::filesystem::file_time_type safeLastWriteTime(const std::string& path)
    {
        std::error_code errorCode;
        const std::filesystem::file_time_type writeTime = std::filesystem::last_write_time(path, errorCode);
        if (errorCode)
            return std::filesystem::file_time_type::min();
        return writeTime;
    }

    void snapshotSourceWriteTimes(const std::filesystem::file_time_type& vertexWriteTime,
                                  const std::filesystem::file_time_type& fragmentWriteTime)
    {
        m_vertexWriteTime = vertexWriteTime;
        m_fragmentWriteTime = fragmentWriteTime;
        m_hasObservedSourceWriteTimes = true;
    }

    static void reflectUniforms(GLuint programId, std::vector<UniformDescriptor>& descriptors)
    {
        descriptors.clear();

        GLint uniformCount = 0;
        glGetProgramiv(programId, GL_ACTIVE_UNIFORMS, &uniformCount);
        for (GLint uniformIndex = 0; uniformIndex < uniformCount; ++uniformIndex)
        {
            GLchar nameBuffer[256] = {};
            GLsizei nameLength = 0;
            GLint size = 0;
            GLenum type = 0;
            glGetActiveUniform(programId, static_cast<GLuint>(uniformIndex), sizeof(nameBuffer), &nameLength, &size, &type, nameBuffer);
            if (nameLength <= 0)
                continue;

            std::string uniformName(nameBuffer, static_cast<size_t>(nameLength));
            if (uniformName.size() > 3 && uniformName.compare(uniformName.size() - 3, 3, "[0]") == 0)
                uniformName.erase(uniformName.size() - 3);

            if (isEngineUniform(uniformName))
                continue;

            UniformDescriptor descriptor;
            descriptor.name = uniformName;
            descriptor.kind = classifyUniformKind(type);
            descriptor.glType = type;
            descriptor.size = size;
            descriptors.push_back(descriptor);
        }
    }

    bool compileProgram(GLuint& compiledProgram, std::vector<UniformDescriptor>& uniformDescriptors)
    {
        compiledProgram = 0;

        const GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
        const GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

        const std::string vertShaderStr = ReadFile(vPath.c_str());
        const std::string fragShaderStr = ReadFile(fPath.c_str());

        const char* vertShaderSrc = vertShaderStr.c_str();
        const char* fragShaderSrc = fragShaderStr.c_str();

        GLint vertexCompileStatus = GL_FALSE;
        GLint fragmentCompileStatus = GL_FALSE;
        GLint linkStatus = GL_FALSE;
        int logLength = 0;

        std::cout << "Compiling vertex shader : " << vPath << " - " << std::flush;
        glShaderSource(vertShader, 1, &vertShaderSrc, NULL);
        glCompileShader(vertShader);
        glGetShaderiv(vertShader, GL_COMPILE_STATUS, &vertexCompileStatus);
        glGetShaderiv(vertShader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> vertShaderError(logLength > 1 ? logLength : 1, '\0');
        if (logLength > 0)
            glGetShaderInfoLog(vertShader, logLength, NULL, &vertShaderError[0]);
        if (logLength > 1)
            std::cout << std::endl << &vertShaderError[0] << std::endl;
        else
            std::cout << "Done!" << std::endl;

        std::cout << "Compiling fragment shader : " << fPath << " - " << std::flush;
        glShaderSource(fragShader, 1, &fragShaderSrc, NULL);
        glCompileShader(fragShader);
        glGetShaderiv(fragShader, GL_COMPILE_STATUS, &fragmentCompileStatus);
        glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> fragShaderError(logLength > 1 ? logLength : 1, '\0');
        if (logLength > 0)
            glGetShaderInfoLog(fragShader, logLength, NULL, &fragShaderError[0]);
        if (logLength > 1)
            std::cout << std::endl << &fragShaderError[0] << std::endl;
        else
            std::cout << "Done!" << std::endl;

        if (vertexCompileStatus != GL_TRUE || fragmentCompileStatus != GL_TRUE)
        {
            glDeleteShader(vertShader);
            glDeleteShader(fragShader);
            return false;
        }

        std::cout << "Linking program - " << std::flush;
        const GLuint program = glCreateProgram();
        glAttachShader(program, vertShader);
        glAttachShader(program, fragShader);
        glLinkProgram(program);

        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> programError(logLength > 1 ? logLength : 1, '\0');
        if (logLength > 0)
            glGetProgramInfoLog(program, logLength, NULL, &programError[0]);
        if (logLength > 1)
            std::cout << std::endl << &programError[0] << std::endl;
        else
            std::cout << "Done!" << std::endl;

        glDeleteShader(vertShader);
        glDeleteShader(fragShader);

        if (linkStatus != GL_TRUE)
        {
            glDeleteProgram(program);
            return false;
        }

        reflectUniforms(program, uniformDescriptors);
        compiledProgram = program;
        return true;
    }

    bool recompileForWriteTimes(const std::filesystem::file_time_type& vertexWriteTime,
                                const std::filesystem::file_time_type& fragmentWriteTime)
    {
        GLuint compiledProgram = 0;
        std::vector<UniformDescriptor> compiledUniformDescriptors;
        const bool compileSucceeded = compileProgram(compiledProgram, compiledUniformDescriptors);

        snapshotSourceWriteTimes(vertexWriteTime, fragmentWriteTime);

        if (!compileSucceeded)
        {
            if (loaded && programID != 0)
                std::cerr << "Shader reload failed for " << m_assetPath << ". Keeping previous valid program." << std::endl;
            else
                std::cerr << "Shader compile failed for " << m_assetPath << " and no valid fallback is available yet." << std::endl;
            return false;
        }

        if (loaded && programID != 0)
            glDeleteProgram(programID);

        programID = compiledProgram;
        loaded = true;
        m_uniformDescriptors = std::move(compiledUniformDescriptors);
        ++m_reloadGeneration;
        return true;
    }

    bool ensureCompiled()
    {
        if (loaded)
            return true;

        const std::filesystem::file_time_type currentVertexWriteTime = safeLastWriteTime(vPath);
        const std::filesystem::file_time_type currentFragmentWriteTime = safeLastWriteTime(fPath);
        if (m_hasObservedSourceWriteTimes && currentVertexWriteTime == m_vertexWriteTime && currentFragmentWriteTime == m_fragmentWriteTime)
            return false;

        return recompileForWriteTimes(currentVertexWriteTime, currentFragmentWriteTime);
    }
    
public:
    Shader(const char * vertex_path, const char * fragment_path)
        : vPath(vertex_path != nullptr ? vertex_path : ""),
          fPath(fragment_path != nullptr ? fragment_path : "")
    {
    }

    Shader(const std::string& vertex_path, const std::string& fragment_path)
        : vPath(vertex_path), fPath(fragment_path)
    {
    }

    bool refreshIfSourcesChanged()
    {
        const std::filesystem::file_time_type currentVertexWriteTime = safeLastWriteTime(vPath);
        const std::filesystem::file_time_type currentFragmentWriteTime = safeLastWriteTime(fPath);

        if (!m_hasObservedSourceWriteTimes || currentVertexWriteTime != m_vertexWriteTime || currentFragmentWriteTime != m_fragmentWriteTime)
            return recompileForWriteTimes(currentVertexWriteTime, currentFragmentWriteTime);

        return false;
    }

    void const setActive()
    {
        if (!const_cast<Shader*>(this)->ensureCompiled())
            return;
        glUseProgram(programID);
        //UPLOAD UNIFORMS
        
    }

    void Upload(IUniform * u)
    {
        if (!ensureCompiled())
            return;
        u -> upload(programID);
    }

    void setAssetPath(const std::string& assetPath)
    {
        m_assetPath = assetPath;
    }

    const std::string& getAssetPath() const
    {
        return m_assetPath;
    }

    const std::string& getVertexPath() const
    {
        return vPath;
    }

    const std::string& getFragmentPath() const
    {
        return fPath;
    }

    GLuint getProgramId()
    {
        ensureCompiled();
        return programID;
    }

    const std::vector<UniformDescriptor>& getEditableUniforms()
    {
        ensureCompiled();
        return m_uniformDescriptors;
    }

    bool tryGetUniformDescriptor(const std::string& uniformName, UniformDescriptor& descriptor, bool includeEngineUniforms = true)
    {
        if (!ensureCompiled() || programID == 0)
            return false;

        GLint uniformCount = 0;
        glGetProgramiv(programID, GL_ACTIVE_UNIFORMS, &uniformCount);
        for (GLint uniformIndex = 0; uniformIndex < uniformCount; ++uniformIndex)
        {
            GLchar nameBuffer[256] = {};
            GLsizei nameLength = 0;
            GLint size = 0;
            GLenum type = 0;
            glGetActiveUniform(programID, static_cast<GLuint>(uniformIndex), sizeof(nameBuffer), &nameLength, &size, &type, nameBuffer);
            if (nameLength <= 0)
                continue;

            std::string activeUniformName(nameBuffer, static_cast<size_t>(nameLength));
            if (activeUniformName.size() > 3 && activeUniformName.compare(activeUniformName.size() - 3, 3, "[0]") == 0)
                activeUniformName.erase(activeUniformName.size() - 3);
            if (!includeEngineUniforms && isEngineUniform(activeUniformName))
                continue;
            if (activeUniformName != uniformName)
                continue;

            descriptor.name = activeUniformName;
            descriptor.kind = classifyUniformKind(type);
            descriptor.glType = type;
            descriptor.size = size;
            return true;
        }

        return false;
    }

    uint64_t getReloadGeneration()
    {
        ensureCompiled();
        return m_reloadGeneration;
    }


    ~Shader()
    {
        if (loaded)
            glDeleteProgram(programID);
    }


    std::string ReadFile(const char* path) { 
        std::string content;
        std::ifstream fileStream(path, std::ios::in);

        if (!fileStream.is_open()) { 
            std::cerr << "File could not be opened" << std::endl;
            return "";
        }

        std::string line = "";
        while (!fileStream.eof()) { 
            getline(fileStream, line);
            content.append(line + "\n");
        }

        fileStream.close();
        return content;
    }

};



#endif