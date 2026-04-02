#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace runtime_app
{
struct RuntimeWindow
{
    GLFWwindow* handle = nullptr;
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    int defaultFramebufferSamples = 0;
};

bool initializeWindow(RuntimeWindow& runtimeWindow, const char* title, const char* createWindowErrorMessage, bool visible = true);
void shutdownWindow(RuntimeWindow& runtimeWindow);
}