#pragma once
#include "Shader.hpp"
#include "../Camera.hpp"
#include <common/gameobject/Transform.hpp>


namespace dataStruct
{
    // AHAH ENCORE DU POLY DANS UNE LISTE, gud luk, future me!
    class AMaterial
    {
    protected:
        Shader * m_shader;
        template <typename T> static void push_back(T ** & arr, T * val, uint & capacity)
        {
            T ** n_arr = new T*[++capacity];
            for (uint i = 0; i < capacity - 1; i++)
                n_arr[i] = arr[i];
            n_arr[capacity - 1] = val;
            delete[] arr;
            arr = n_arr;
        }
    public:
        AMaterial(Shader * shader)
        {
            m_shader = shader;
        }
        // used to declare uniforms
        virtual void init() = 0;
        // used to sync uniforms
        virtual void sync() = 0;
        virtual void bind(Camera const & cam, Transform & transform) = 0;
        virtual ~AMaterial(){}
    };
    

    class Material : public AMaterial
    {
    private:
        glm::mat4 * mvp;
        glm::mat4 * mvpOrtho;
    protected:

        uint m_uni1f_stride = 0;
        uint m_uniMat4f_stride = 0;
        uint m_uniVec3f_stride = 0;
        
        Uniform1f ** m_uni1f = nullptr;
        UniformMat4x4f ** m_uniMat4f = nullptr;
        UniformVec3f ** m_uniVec3f = nullptr;
        std::vector<UniformTex2D> textures = std::vector<UniformTex2D>(0);
        
    public:
        


        Material() = delete;
        Material(Shader * shader) : AMaterial(shader){init();}
        void init() override
        {
            mvp = new glm::mat4 {glm::mat4(
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0
            )};
            mvpOrtho = new glm::mat4 {glm::mat4(
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0
            )};

            push_back(m_uniMat4f, new UniformMat4x4f("MVP", mvp), m_uniMat4f_stride);
            push_back(m_uniMat4f, new UniformMat4x4f("MVP_ORTHO", mvpOrtho), m_uniMat4f_stride);
        }
        
        void addTexture(std::string uniformLocation, std::string texturePath)
        {
            textures.push_back(UniformTex2D(uniformLocation, Texture2D(texturePath, textures.size())));
        }


        void sync() override
        {
            m_shader -> setActive();
            for (uint i = 0; i < m_uni1f_stride; i++)
                m_shader -> Upload((IUniform *)m_uni1f[i]);
            for (uint i = 0; i < m_uniMat4f_stride; i++)
                m_shader -> Upload((IUniform *)m_uniMat4f[i]);
            for (uint i = 0; i < textures.size(); i++)
                m_shader -> Upload((IUniform *)&(textures[i]));
            for (uint i = 0; i < m_uniVec3f_stride; i++)
                m_shader -> Upload((IUniform *)m_uniVec3f[i]);
        }
        
        void bind(Camera const & cam, Transform & transform) override
        {
            // TODO rebuild mvp here!
            *mvp = cam.projectionMatrix() * cam.inverseTransform() * transform.getModelWorld();
            *mvpOrtho = glm::transpose(glm::inverse(transform.getModelWorld()));
            sync();
        }

        ~Material()
        {
            for (uint i = 0; i < m_uni1f_stride; i++)
                delete (m_uni1f[i]);
            delete[] m_uni1f;
            for (uint i = 0; i < m_uniMat4f_stride; i++)
                delete (m_uniMat4f[i]);
            delete[] m_uniMat4f;
            delete mvp;
            delete mvpOrtho;
        }
    };
}

