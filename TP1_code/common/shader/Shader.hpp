#ifndef MSHADER_HPP // port from glutony
#define MSHADER_HPP


#include <string.h>
#include <vector>
#include <iostream>
#include <fstream>
#include "Uniforms.hpp"


#include <GL/glut.h>
#include <GL/glew.h>

class Shader
{
private:
    const char * vPath;
    const char * fPath;
    GLuint programID;
    bool loaded = false;
    
public:
    Shader(const char * vertex_path, const char * fragment_path){vPath = vertex_path; fPath = fragment_path;}
    void recompile()
    {
        if (loaded)
            glDeleteProgram(programID);
        programID = LoadShader(vPath, fPath);
        loaded = true;
    }

    void const setActive()
    {
        if (!loaded)
            recompile();
        glUseProgram(programID);
        //UPLOAD UNIFORMS
        
    }

    void Upload(IUniform * u)
    {
        u -> upload(programID);
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

    



    GLuint LoadShader(const char* vertPath, const char* fragPath) { 
        //generate shader names
        GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

        //get shader src
        std::string vertShaderStr = ReadFile(vertPath);
        std::string fragShaderStr = ReadFile(fragPath);

        const char* vertShaderSrc = vertShaderStr.c_str();
        const char* fragShaderSrc = fragShaderStr.c_str();

        GLint result = GL_FALSE;
        int logLength;

        std::cout << "Compiling vertex shader : " << vertPath << " - " << std::flush;
        //compile vertex shader
        glShaderSource(vertShader, 1, &vertShaderSrc, NULL);
        glCompileShader(vertShader);
        glGetShaderiv(vertShader, GL_COMPILE_STATUS, &result);
        glGetShaderiv(vertShader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> vertShaderError(logLength);
        glGetShaderInfoLog(vertShader, logLength, NULL, &vertShaderError[0]);
        if (logLength > 1)
            std::cout << std::endl << &vertShaderError[0] << std::endl;
        else
            std::cout << "Done!" << std::endl;
        std::cout << "Compiling fragment shader : " << fragPath << " - " << std::flush;
        //compile fragment shader
        glShaderSource(fragShader, 1, &fragShaderSrc, NULL);
        glCompileShader(fragShader);
        glGetShaderiv(fragShader, GL_COMPILE_STATUS, &result);
        glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> fragShaderError(logLength);
        glGetShaderInfoLog(fragShader, logLength, NULL, &fragShaderError[0]);
        if (logLength > 1)
            std::cout << std::endl << &fragShaderError[0] << std::endl; 
        else
            std::cout << "Done!" << std::endl;
        std::cout << "Linking program - " << std::flush;
        //link the program
        GLuint program = glCreateProgram();
        glAttachShader(program, vertShader);
        glAttachShader(program, fragShader);
        glLinkProgram(program);

        glGetProgramiv(program, GL_LINK_STATUS, &result);
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> programError(logLength > 1 ? logLength : 1);
        glGetProgramInfoLog(program, logLength, NULL, &programError[0]);
        if (logLength > 1)
            std::cout << std::endl << &programError[0] << std::endl;
        else
            std::cout << "Done!" << std::endl;

        glDeleteShader(vertShader);
        glDeleteShader(fragShader);

        return program;
    }
};



#endif