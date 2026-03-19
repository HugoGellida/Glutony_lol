#pragma once
#include <map>
#include <GLFW/glfw3.h>
#include <iostream>
namespace inputProcessor
{
    enum class KeyState
    {
        ONCE, HOLD
    };


    class InputProcessor
    {
    private:
        std::map<int, KeyState> m_keyWatchers;
        std::map<int, bool> m_keyStates;
        double m_mouseX = 0.0f;
        double m_mouseY = 0.0f;
        double m_mousePX = 0.0f;
        double m_mousePY = 0.0f;
        double m_mouseDeltaX = 0.0f;
        double m_mouseDeltaY = 0.0f;
    public:
        InputProcessor() {
            m_keyWatchers = std::map<int, KeyState>();
            m_keyStates = std::map<int, bool>();
        }
        ~InputProcessor() {
            m_keyWatchers.clear();
            m_keyStates.clear();
        }

        void registerKey(int key, KeyState state)
        {
            if (m_keyWatchers.find(key) != m_keyWatchers.end())
                return; // already registered
            if (state == KeyState::ONCE)
            {
                m_keyWatchers[key] = state;
                m_keyStates[key] = false;
            }
            else if (state == KeyState::HOLD)
            {
                m_keyWatchers[key] = state;
                m_keyStates[key] = false;
            }
        }

        

        void update(GLFWwindow * window)
        {
            for (auto & [key, state] : m_keyWatchers)
            {
                if (state == KeyState::ONCE)
                {
                    if (glfwGetKey(window, key) == GLFW_PRESS)
                    {
                        if (!m_keyStates[key])
                            m_keyStates[key] = true;
                    } // dont reset, it will be reset on consume
                }
                else if (state == KeyState::HOLD)
                {
                    m_keyStates[key] = (glfwGetKey(window, key) == GLFW_PRESS);
                }
            }
            
            glfwGetCursorPos(window, &m_mouseX, &m_mouseY);
            if (m_mousePX - m_mouseX != 0 || m_mousePY - m_mouseY != 0)
            {
                float sensitivity = 0.1f;
                m_mouseDeltaX = m_mouseX - m_mousePX;
                m_mouseDeltaY = m_mousePY - m_mouseY;

                
                // reset mouse pos
                int scrWidth, scrHeight;
                glfwGetWindowSize(window, &scrWidth, &scrHeight);
                m_mousePX = scrWidth / 2;
                m_mousePY = scrHeight / 2;
            }
            else
            {
                m_mousePX = m_mouseX;
                m_mousePY = m_mouseY;
            }
        }

        bool queryKey(GLFWwindow * window, int key)
        {
            if (m_keyWatchers[key] == KeyState::ONCE)
            {
                if (m_keyStates[key] && glfwGetKey(window, key) == GLFW_RELEASE)
                {
                    m_keyStates[key] = false; // reset
                    return true;
                }
                return false;
            }
            else if (m_keyWatchers[key] == KeyState::HOLD)
            {
                return m_keyStates[key];
            }
            return false;
        }

        float getMouseDeltaX()
        {
            return m_mouseX - m_mousePX;
        }

        float getMouseDeltaY()
        {
            return m_mouseY - m_mousePY;
        }

        void unregisterKey(int key)
        {
            m_keyWatchers.erase(key);
            m_keyStates.erase(key);
        }
    };
}