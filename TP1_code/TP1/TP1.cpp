// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <memory>
#include <vector>
#include <iostream>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

using namespace glm;


#include <common/geometry/Plane.hpp>
#include <common/shader/Shader.hpp>
#include <common/meshRenderer/simpleMeshrenderer.hpp>
#include <common/Scene.hpp>
#include <common/ui/EditorUi.hpp>

#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <RmlUi_Platform_GLFW.h>
#include <RmlUi_Renderer_GL3.h>


void processInput(GLFWwindow *window);
void setup_glfw_callbacks(GLFWwindow* glfwWindow);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
bool g_editorModeEnabled = true;
bool g_uiBuilderEnabled = false;
bool g_uiBuilderShowStylePanel = false;

// camera
glm::vec3 camera_position   = glm::vec3(0.0f, 0.0f,  3.0f);
glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camera_up    = glm::vec3(0.0f, 1.0f,  0.0f);

// SCENE
Scene * scene;


// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

int g_windowFramebufferWidth = 1024;
int g_windowFramebufferHeight = 768;
int g_glfwActiveModifiers = 0;
int g_defaultFramebufferSamples = 0;

std::unique_ptr<SystemInterface_GLFW> g_rmlSystemInterface;
std::unique_ptr<RenderInterface_GL3> g_rmlRenderInterface;
Rml::Context* g_rmlContext = nullptr;
EditorUiController g_editorUi;

struct ViewportFramebuffer
{
    GLuint framebuffer = 0;
    GLuint colorTexture = 0;
    GLuint depthStencilRenderbuffer = 0;
    int width = 0;
    int height = 0;

    void destroy()
    {
        if (depthStencilRenderbuffer != 0)
        {
            glDeleteRenderbuffers(1, &depthStencilRenderbuffer);
            depthStencilRenderbuffer = 0;
        }

        if (colorTexture != 0)
        {
            glDeleteTextures(1, &colorTexture);
            colorTexture = 0;
        }

        if (framebuffer != 0)
        {
            glDeleteFramebuffers(1, &framebuffer);
            framebuffer = 0;
        }

        width = 0;
        height = 0;
    }

    bool ensureSize(int targetWidth, int targetHeight)
    {
        targetWidth = std::max(targetWidth, 1);
        targetHeight = std::max(targetHeight, 1);

        if (framebuffer != 0 && width == targetWidth && height == targetHeight)
            return true;

        destroy();

        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        glGenTextures(1, &colorTexture);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, targetWidth, targetHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

        glGenRenderbuffers(1, &depthStencilRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, targetWidth, targetHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilRenderbuffer);

        const bool isComplete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (!isComplete)
        {
            destroy();
            return false;
        }

        width = targetWidth;
        height = targetHeight;
        return true;
    }
};

ViewportFramebuffer g_viewportFramebuffer;

int mousePX = 0;
int mousePY = 0;
bool fpsControl = false;
bool gWasPressed = false;
bool g_editorTogglePressed = false;

//rotation
float angle = 0.;
float zoom = 1.;
/*******************************************************************************/


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    g_windowFramebufferWidth = width;
    g_windowFramebufferHeight = height;

    if (g_rmlRenderInterface)
        g_rmlRenderInterface->SetViewport(width, height);

    if (g_rmlContext)
        RmlGLFW::ProcessFramebufferSizeCallback(g_rmlContext, width, height);

    if (!g_editorModeEnabled && scene != nullptr && height > 0)
        scene -> updateCamSettings((float)width/(float)height);
}

void content_scale_callback(GLFWwindow* window, float xscale, float yscale)
{
    (void)window;
    (void)yscale;
    if (g_rmlContext)
        RmlGLFW::ProcessContentScaleCallback(g_rmlContext, xscale);
}

void setup_glfw_callbacks(GLFWwindow* glfwWindow)
{
    glfwSetKeyCallback(glfwWindow, [](GLFWwindow* callbackWindow, int glfwKey, int scancode, int glfwAction, int glfwMods)
    {
        (void)callbackWindow;
        (void)scancode;
        g_glfwActiveModifiers = glfwMods;
        if (g_rmlContext)
            RmlGLFW::ProcessKeyCallback(g_rmlContext, glfwKey, glfwAction, glfwMods);
    });

    glfwSetCharCallback(glfwWindow, [](GLFWwindow* callbackWindow, unsigned int codepoint)
    {
        (void)callbackWindow;
        if (g_rmlContext)
            RmlGLFW::ProcessCharCallback(g_rmlContext, codepoint);
    });

    glfwSetCursorEnterCallback(glfwWindow, [](GLFWwindow* callbackWindow, int entered)
    {
        (void)callbackWindow;
        if (g_rmlContext)
            RmlGLFW::ProcessCursorEnterCallback(g_rmlContext, entered);
    });

    glfwSetCursorPosCallback(glfwWindow, [](GLFWwindow* callbackWindow, double xpos, double ypos)
    {
        if (g_rmlContext)
            RmlGLFW::ProcessCursorPosCallback(g_rmlContext, callbackWindow, xpos, ypos, g_glfwActiveModifiers);
    });

    glfwSetMouseButtonCallback(glfwWindow, [](GLFWwindow* callbackWindow, int button, int action, int mods)
    {
        (void)callbackWindow;
        g_glfwActiveModifiers = mods;
        if (g_rmlContext)
            RmlGLFW::ProcessMouseButtonCallback(g_rmlContext, button, action, mods);
    });

    glfwSetScrollCallback(glfwWindow, [](GLFWwindow* callbackWindow, double xoffset, double yoffset)
    {
        (void)callbackWindow;
        (void)xoffset;
        if (g_rmlContext)
            RmlGLFW::ProcessScrollCallback(g_rmlContext, yoffset, g_glfwActiveModifiers);
    });

    glfwSetFramebufferSizeCallback(glfwWindow, framebuffer_size_callback);
#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 3)
    glfwSetWindowContentScaleCallback(glfwWindow, content_scale_callback);
#endif
}

int main( void )
{
    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        getchar();
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    

    // Open a window and create its OpenGL context
    window = glfwCreateWindow( 1024, 768, "glutglut", NULL, NULL);
    if( window == NULL ){
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    //  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 768/2);
    glfwGetFramebufferSize(window, &g_windowFramebufferWidth, &g_windowFramebufferHeight);

    // Dark blue background
    glClearColor(0.8f, 0.8f, 0.8f, 0.0f);
    glGetIntegerv(GL_SAMPLES, &g_defaultFramebufferSamples);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LEQUAL);

    // Cull triangles which normal is not towards the camera
    //glEnable(GL_CULL_FACE);

    // LOAD SCENE    
    scene = new Scene();
    setup_glfw_callbacks(window);

    {
        Rml::String rendererMessage;
        if (!RmlGL3::Initialize(&rendererMessage))
        {
            std::cerr << "Failed to initialize RmlUi GL3 renderer" << std::endl;
            delete scene;
            glfwTerminate();
            return -1;
        }

        g_rmlSystemInterface = std::make_unique<SystemInterface_GLFW>(window);
        g_rmlRenderInterface = std::make_unique<RenderInterface_GL3>();

        if (!g_rmlRenderInterface || !(*g_rmlRenderInterface))
        {
            std::cerr << "Failed to construct RmlUi render interface" << std::endl;
            delete scene;
            g_rmlSystemInterface.reset();
            RmlGL3::Shutdown();
            glfwTerminate();
            return -1;
        }

        g_rmlRenderInterface->SetViewport(g_windowFramebufferWidth, g_windowFramebufferHeight);

        Rml::SetSystemInterface(g_rmlSystemInterface.get());
        Rml::SetRenderInterface(g_rmlRenderInterface.get());

        if (!Rml::Initialise())
        {
            std::cerr << "Failed to initialise RmlUi core" << std::endl;
            delete scene;
            g_rmlRenderInterface.reset();
            g_rmlSystemInterface.reset();
            RmlGL3::Shutdown();
            glfwTerminate();
            return -1;
        }

        Rml::LoadFontFace("../external/RmlUi/Samples/assets/LatoLatin-Regular.ttf");
        Rml::LoadFontFace("../external/RmlUi/Samples/assets/LatoLatin-Bold.ttf");

        g_rmlContext = Rml::CreateContext("editor", Rml::Vector2i(g_windowFramebufferWidth, g_windowFramebufferHeight));
        if (g_rmlContext == nullptr || !g_editorUi.initialize(g_rmlContext))
        {
            std::cerr << "Failed to create editor UI" << std::endl;
            if (g_rmlContext != nullptr)
                g_rmlContext->UnloadAllDocuments();
            Rml::Shutdown();
            g_rmlRenderInterface.reset();
            g_rmlSystemInterface.reset();
            RmlGL3::Shutdown();
            delete scene;
            glfwTerminate();
            return -1;
        }

        g_editorUi.setUiBuilderEnabled(g_uiBuilderEnabled);

        scene->setUiContext(g_rmlContext);
    }
    

    // For speed computation
    double lastTime = glfwGetTime();
    int nbFrames = 0;

    do{

        // Measure speed
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
        {
            if (!g_editorTogglePressed)
            {
                g_editorModeEnabled = !g_editorModeEnabled;
                g_editorTogglePressed = true;

                if (scene->isFpsControlEnabled())
                    glfwSetInputMode(window, GLFW_CURSOR, g_editorModeEnabled ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_DISABLED);
                else
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

                if (!g_editorModeEnabled && g_windowFramebufferHeight > 0)
                    scene->updateCamSettings((float)g_windowFramebufferWidth / (float)g_windowFramebufferHeight);
            }
        }
        else
        {
            g_editorTogglePressed = false;
        }

        if (g_editorModeEnabled)
        {
            g_editorUi.setUiBuilderShowStylePanel(g_uiBuilderShowStylePanel);
            g_editorUi.syncToWindow(g_windowFramebufferWidth, g_windowFramebufferHeight);
            g_editorUi.update();

            g_uiBuilderEnabled = g_editorUi.isUiBuilderEnabled();

            if (g_editorUi.isUiBuilderEnabled())
            {
                glDisable(GL_SCISSOR_TEST);
                glViewport(0, 0, g_windowFramebufferWidth, g_windowFramebufferHeight);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                GLint polygonMode[2] = { GL_FILL, GL_FILL };
                glGetIntegerv(GL_POLYGON_MODE, polygonMode);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                g_rmlRenderInterface->BeginFrame();
                g_editorUi.render();
                g_rmlRenderInterface->EndFrame();
                glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(polygonMode[0]));
                glfwSwapBuffers(window);
                continue;
            }

            UiRect viewportRect = g_editorUi.getViewportRect();
            double cursorX = 0.0;
            double cursorY = 0.0;
            glfwGetCursorPos(window, &cursorX, &cursorY);

            const bool viewportHovered = viewportRect.isValid() && g_editorUi.isViewportHovered(cursorX, cursorY);
            const bool sceneInputEnabled = (viewportHovered && !g_editorUi.isDragging()) || scene->isFpsControlEnabled() || scene->isOrbitModeEnabled();
            const bool viewportFramebufferReady = viewportRect.isValid() && g_viewportFramebuffer.ensureSize(viewportRect.width, viewportRect.height);
            const bool useViewportFramebuffer = viewportFramebufferReady && g_defaultFramebufferSamples == 0;

            if (viewportRect.isValid())
                scene->updateCamSettings((float)viewportRect.width / (float)std::max(viewportRect.height, 1));

            scene->update(
                deltaTime,
                window,
                sceneInputEnabled,
                true,
                viewportRect.centerX(),
                viewportRect.centerY()
            );

            glDisable(GL_SCISSOR_TEST);

            if (useViewportFramebuffer)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, g_viewportFramebuffer.framebuffer);
                glViewport(0, 0, g_viewportFramebuffer.width, g_viewportFramebuffer.height);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                scene->renderScene();
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }

            glViewport(0, 0, g_windowFramebufferWidth, g_windowFramebufferHeight);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if (useViewportFramebuffer)
            {
                const int destinationX0 = viewportRect.x;
                const int destinationY0 = g_windowFramebufferHeight - viewportRect.y - viewportRect.height;
                const int destinationX1 = destinationX0 + viewportRect.width;
                const int destinationY1 = destinationY0 + viewportRect.height;

                glBindFramebuffer(GL_READ_FRAMEBUFFER, g_viewportFramebuffer.framebuffer);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                glBlitFramebuffer(
                    0,
                    0,
                    g_viewportFramebuffer.width,
                    g_viewportFramebuffer.height,
                    destinationX0,
                    destinationY0,
                    destinationX1,
                    destinationY1,
                    GL_COLOR_BUFFER_BIT,
                    GL_LINEAR
                );
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            else if (viewportRect.isValid())
            {
                const int glViewportY = g_windowFramebufferHeight - viewportRect.y - viewportRect.height;
                glEnable(GL_SCISSOR_TEST);
                glViewport(viewportRect.x, glViewportY, viewportRect.width, viewportRect.height);
                glScissor(viewportRect.x, glViewportY, viewportRect.width, viewportRect.height);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                scene->renderScene();
                glDisable(GL_SCISSOR_TEST);
                glViewport(0, 0, g_windowFramebufferWidth, g_windowFramebufferHeight);
            }

            GLint polygonMode[2] = { GL_FILL, GL_FILL };
            glGetIntegerv(GL_POLYGON_MODE, polygonMode);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            g_rmlRenderInterface->BeginFrame();
            g_editorUi.render();
            g_rmlRenderInterface->EndFrame();
            glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(polygonMode[0]));
            glfwSwapBuffers(window);
        }
        else
        {
            scene -> update(deltaTime, window);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            scene -> renderScene();

            if (scene->hasUiRenderers())
            {
                scene->setUiViewportRect(0, 0, g_windowFramebufferWidth, g_windowFramebufferHeight);
                g_rmlContext->Update();
                g_rmlRenderInterface->BeginFrame();
                scene->renderUi();
                g_rmlContext->Render();
                g_rmlRenderInterface->EndFrame();
            }

            glfwSwapBuffers(window);
        }

    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 );


    // UNLOAD SCENE

    delete scene;

    g_viewportFramebuffer.destroy();

    if (g_rmlContext != nullptr)
    {
        g_editorUi.shutdown();
        Rml::Shutdown();
        g_rmlContext = nullptr;
        g_rmlRenderInterface.reset();
        g_rmlSystemInterface.reset();
        RmlGL3::Shutdown();
    }

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    
}


