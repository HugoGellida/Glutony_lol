#include "RuntimeWindow.hpp"

#include <cstdio>

namespace runtime_app
{
bool initializeWindow(RuntimeWindow& runtimeWindow, const char* title, const char* createWindowErrorMessage, bool visible)
{
    if (!glfwInit())
    {
        std::fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, visible ? 1 : 0);

    runtimeWindow.handle = glfwCreateWindow(1024, 768, title, NULL, NULL);
    if (runtimeWindow.handle == NULL)
    {
        std::fprintf(stderr, "%s\n", createWindowErrorMessage);
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(runtimeWindow.handle);
    glfwSwapInterval(1);

    glewExperimental = true;
    if (glewInit() != GLEW_OK)
    {
        std::fprintf(stderr, "Failed to initialize GLEW\n");
        shutdownWindow(runtimeWindow);
        return false;
    }

    glfwSetInputMode(runtimeWindow.handle, GLFW_STICKY_KEYS, GL_TRUE);
    glfwPollEvents();
    glfwSetCursorPos(runtimeWindow.handle, 1024 / 2, 768 / 2);
    glfwGetFramebufferSize(runtimeWindow.handle, &runtimeWindow.framebufferWidth, &runtimeWindow.framebufferHeight);

    glClearColor(0.8f, 0.8f, 0.8f, 0.0f);
    glGetIntegerv(GL_SAMPLES, &runtimeWindow.defaultFramebufferSamples);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    return true;
}

void shutdownWindow(RuntimeWindow& runtimeWindow)
{
    if (runtimeWindow.handle != nullptr)
    {
        glfwDestroyWindow(runtimeWindow.handle);
        runtimeWindow.handle = nullptr;
    }

    glfwTerminate();
    runtimeWindow.framebufferWidth = 0;
    runtimeWindow.framebufferHeight = 0;
    runtimeWindow.defaultFramebufferSamples = 0;
}
}