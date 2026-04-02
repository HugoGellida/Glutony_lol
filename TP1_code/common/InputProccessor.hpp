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
        std::map<int, bool> m_keyPressedNow;
        double m_mouseX = 0.0f;
        double m_mouseY = 0.0f;
        double m_mousePX = 0.0f;
        double m_mousePY = 0.0f;
        double m_mouseDeltaX = 0.0f;
        double m_mouseDeltaY = 0.0f;
        bool m_injectedInputEnabled = false;
        std::map<int, bool> m_injectedKeyPressed;
        double m_injectedMouseDeltaX = 0.0f;
        double m_injectedMouseDeltaY = 0.0f;
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
                m_keyPressedNow[key] = false;
            }
            else if (state == KeyState::HOLD)
            {
                m_keyWatchers[key] = state;
                m_keyStates[key] = false;
                m_keyPressedNow[key] = false;
            }
        }

        void setInjectedInputState(const std::map<int, bool>& pressedKeys, double mouseDeltaX, double mouseDeltaY)
        {
            m_injectedInputEnabled = true;
            m_injectedKeyPressed = pressedKeys;
            m_injectedMouseDeltaX = mouseDeltaX;
            m_injectedMouseDeltaY = mouseDeltaY;
        }

        void clearInjectedInputState()
        {
            m_injectedInputEnabled = false;
            m_injectedKeyPressed.clear();
            m_injectedMouseDeltaX = 0.0;
            m_injectedMouseDeltaY = 0.0;
        }

        

        void update(GLFWwindow * window, bool relativeMouseMode = false, double referenceX = 0.0, double referenceY = 0.0)
        {
            for (auto & [key, state] : m_keyWatchers)
            {
                const bool keyPressed = m_injectedInputEnabled
                    ? (m_injectedKeyPressed.find(key) != m_injectedKeyPressed.end() && m_injectedKeyPressed.at(key))
                    : (glfwGetKey(window, key) == GLFW_PRESS);

                m_keyPressedNow[key] = keyPressed;

                if (state == KeyState::ONCE)
                {
                    if (keyPressed)
                    {
                        if (!m_keyStates[key])
                            m_keyStates[key] = true;
                    } // dont reset, it will be reset on consume
                }
                else if (state == KeyState::HOLD)
                {
                    m_keyStates[key] = keyPressed;
                }
            }

            if (m_injectedInputEnabled)
            {
                m_mousePX = referenceX;
                m_mousePY = referenceY;
                m_mouseX = referenceX + m_injectedMouseDeltaX;
                m_mouseY = referenceY - m_injectedMouseDeltaY;
                m_mouseDeltaX = relativeMouseMode ? m_injectedMouseDeltaX : 0.0;
                m_mouseDeltaY = relativeMouseMode ? m_injectedMouseDeltaY : 0.0;
            }
            else
            {
                glfwGetCursorPos(window, &m_mouseX, &m_mouseY);
                if (relativeMouseMode)
                {
                    m_mousePX = referenceX;
                    m_mousePY = referenceY;
                    m_mouseDeltaX = m_mouseX - m_mousePX;
                    m_mouseDeltaY = m_mousePY - m_mouseY;
                }
                else
                {
                    m_mouseDeltaX = 0.0f;
                    m_mouseDeltaY = 0.0f;
                    m_mousePX = m_mouseX;
                    m_mousePY = m_mouseY;
                }
            }
        }

        bool queryKey(GLFWwindow * window, int key)
        {
            (void)window;
            if (m_keyWatchers[key] == KeyState::ONCE)
            {
                if (m_keyStates[key] && !m_keyPressedNow[key])
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
            return m_mouseDeltaX;
        }

        float getMouseDeltaY()
        {
            return m_mouseDeltaY;
        }

        void resetMouseState(double mouseX, double mouseY)
        {
            m_mouseX = mouseX;
            m_mouseY = mouseY;
            m_mousePX = mouseX;
            m_mousePY = mouseY;
            m_mouseDeltaX = 0.0;
            m_mouseDeltaY = 0.0;
        }

        void unregisterKey(int key)
        {
            m_keyWatchers.erase(key);
            m_keyStates.erase(key);
            m_keyPressedNow.erase(key);
        }
    };
}