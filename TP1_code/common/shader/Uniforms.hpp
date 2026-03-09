#pragma once


#include <GL/glew.h>
#include <string>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include "Texture2D.hpp"

class IUniform
{
public:
    virtual void upload(GLint progID) = 0;
    virtual ~IUniform() {};
};


template<typename T>
class Uniform : public IUniform
{
protected:
    T value;
    std::string loc;
protected:
    Uniform(std::string loc, T value) : IUniform()
    {
        this -> loc = loc;
        this -> value = value;
    }
};

class Uniform1f : public Uniform<float>
{
public:
    Uniform1f(std::string loc, float value) : Uniform(loc, value) {}
    void upload(GLint progID) override
    {
        glUniform1f(glGetUniformLocation(progID, loc.c_str()), value);
    }
};


class UniformTex2D : public Uniform<Texture2D>
{
public:
    UniformTex2D() : Uniform("", Texture2D())
    {
        // FOR UNINITIALIZED VALUES, NO TRUE USAGE.
    }
    UniformTex2D(std::string loc, Texture2D value) : Uniform(loc, value) {}
    void upload(GLint progID) override
    {
        value.bind(progID, loc.c_str());
    }
};


class UniformVec3f : public Uniform<glm::vec3 *>
{
public:
    UniformVec3f(std::string loc, glm::vec3 *& value) : Uniform(loc, value){}
    void upload(GLint progID) override
    {
        glUniform3f(glGetUniformLocation(progID, loc.c_str()), value -> x, value -> y, value -> z);
    }
};

class UniformMat4x4f : public Uniform<glm::mat4x4 *>
{
public:
    UniformMat4x4f(std::string loc, glm::mat4x4 *& value) : Uniform(loc, value){}
    void upload(GLint progID) override
    {
        GLint uLoc = glGetUniformLocation(progID, loc.c_str());
        if (uLoc != -1)
            glUniformMatrix4fv(uLoc, 1, GL_FALSE, &((value->operator[](0))[0]));
    }
};