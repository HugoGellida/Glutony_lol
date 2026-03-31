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

glm::dvec2 windowToFramebufferCoords(GLFWwindow* glfwWindow, double x, double y)
{
    int windowWidth = 1;
    int windowHeight = 1;
    int framebufferWidth = 1;
    int framebufferHeight = 1;
    glfwGetWindowSize(glfwWindow, &windowWidth, &windowHeight);
    glfwGetFramebufferSize(glfwWindow, &framebufferWidth, &framebufferHeight);

    if (windowWidth <= 0 || windowHeight <= 0)
        return glm::dvec2(0.0);

    return glm::dvec2(
        x * static_cast<double>(framebufferWidth) / static_cast<double>(windowWidth),
        y * static_cast<double>(framebufferHeight) / static_cast<double>(windowHeight)
    );
}

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

struct ViewportTexturePresenter
{
    GLuint program = 0;
    GLuint vertexShader = 0;
    GLuint fragmentShader = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLint textureUniformLocation = -1;

    static bool compileShader(GLuint shader, const char* source, const char* label)
    {
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint compileStatus = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
        if (compileStatus == GL_TRUE)
            return true;

        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> logBuffer(static_cast<size_t>(std::max(logLength, 1)), '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, logBuffer.data());
        std::cerr << "Viewport presenter " << label << " shader compilation failed: " << logBuffer.data() << std::endl;
        return false;
    }

    bool initialize()
    {
        if (program != 0)
            return true;

        static const char* kVertexShaderSource = R"GLSL(
            #version 330 core
            layout(location = 0) in vec2 inPosition;
            layout(location = 1) in vec2 inUv;

            out vec2 fragUv;

            void main()
            {
                fragUv = inUv;
                gl_Position = vec4(inPosition, 0.0, 1.0);
            }
        )GLSL";

        static const char* kFragmentShaderSource = R"GLSL(
            #version 330 core
            in vec2 fragUv;
            out vec4 outColor;

            uniform sampler2D viewportTexture;

            void main()
            {
                outColor = texture(viewportTexture, fragUv);
            }
        )GLSL";

        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        if (vertexShader == 0 || fragmentShader == 0)
        {
            destroy();
            return false;
        }

        if (!compileShader(vertexShader, kVertexShaderSource, "vertex") ||
            !compileShader(fragmentShader, kFragmentShaderSource, "fragment"))
        {
            destroy();
            return false;
        }

        program = glCreateProgram();
        if (program == 0)
        {
            destroy();
            return false;
        }

        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE)
        {
            GLint logLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<char> logBuffer(static_cast<size_t>(std::max(logLength, 1)), '\0');
            glGetProgramInfoLog(program, logLength, nullptr, logBuffer.data());
            std::cerr << "Viewport presenter program link failed: " << logBuffer.data() << std::endl;
            destroy();
            return false;
        }

        textureUniformLocation = glGetUniformLocation(program, "viewportTexture");

        static const float kVertices[] = {
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f, 1.0f,
             1.0f,  1.0f, 1.0f, 1.0f,
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        if (vao == 0 || vbo == 0)
        {
            destroy();
            return false;
        }

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        return true;
    }

    void render(GLuint texture, int x, int y, int width, int height) const
    {
        if (program == 0 || vao == 0 || texture == 0 || width <= 0 || height <= 0)
            return;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(x, y, width, height);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glUseProgram(program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(textureUniformLocation, 0);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }

    void destroy()
    {
        if (vbo != 0)
        {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }

        if (vao != 0)
        {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }

        if (program != 0)
        {
            glDeleteProgram(program);
            program = 0;
        }

        if (vertexShader != 0)
        {
            glDeleteShader(vertexShader);
            vertexShader = 0;
        }

        if (fragmentShader != 0)
        {
            glDeleteShader(fragmentShader);
            fragmentShader = 0;
        }

        textureUniformLocation = -1;
    }

    explicit operator bool() const
    {
        return program != 0 && vao != 0;
    }
};

ViewportTexturePresenter g_viewportTexturePresenter;

int mousePX = 0;
int mousePY = 0;
bool fpsControl = false;
bool gWasPressed = false;
bool g_editorTogglePressed = false;
bool g_sceneClickPending = false;
double g_sceneClickX = 0.0;
double g_sceneClickY = 0.0;

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
        g_glfwActiveModifiers = mods;

        bool uiHandled = false;
        if (g_rmlContext)
            uiHandled = RmlGLFW::ProcessMouseButtonCallback(g_rmlContext, button, action, mods);

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        {
            double clickWindowX = 0.0;
            double clickWindowY = 0.0;
            glfwGetCursorPos(callbackWindow, &clickWindowX, &clickWindowY);
            const glm::dvec2 clickPosition = windowToFramebufferCoords(callbackWindow, clickWindowX, clickWindowY);

            bool shouldDispatchToScene = !uiHandled;
            if (g_editorModeEnabled)
            {
                const UiRect viewportRect = g_editorUi.getViewportRect();
                shouldDispatchToScene = viewportRect.isValid() && viewportRect.contains(clickPosition.x, clickPosition.y) && !g_editorUi.isDragging();
            }

            if (shouldDispatchToScene)
            {
                g_sceneClickPending = true;
                g_sceneClickX = clickPosition.x;
                g_sceneClickY = clickPosition.y;
            }
        }
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

    if (!g_viewportTexturePresenter.initialize())
    {
        std::cerr << "Failed to initialize viewport texture presenter, falling back to direct scene rendering." << std::endl;
    }

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
            if (scene != nullptr)
                g_editorUi.sync(*scene);
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
            double cursorWindowX = 0.0;
            double cursorWindowY = 0.0;
            glfwGetCursorPos(window, &cursorWindowX, &cursorWindowY);
            const glm::dvec2 cursorPosition = windowToFramebufferCoords(window, cursorWindowX, cursorWindowY);

            const bool viewportHovered = viewportRect.isValid() && g_editorUi.isViewportHovered(cursorPosition.x, cursorPosition.y);
            const bool sceneInputEnabled =
                (viewportHovered && !g_editorUi.isDragging()) ||
                scene->isFpsControlEnabled() ||
                scene->isOrbitModeEnabled() ||
                g_sceneClickPending;
            const bool viewportFramebufferReady = viewportRect.isValid() && g_viewportFramebuffer.ensureSize(viewportRect.width, viewportRect.height);
            const bool useViewportFramebuffer = viewportFramebufferReady && static_cast<bool>(g_viewportTexturePresenter);

            if (viewportRect.isValid())
                scene->setUiViewportRect(viewportRect.x, viewportRect.y, viewportRect.width, viewportRect.height);
            else
                scene->setUiViewportRect(0, 0, 0, 0);

            if (viewportRect.isValid())
                scene->updateCamSettings((float)viewportRect.width / (float)std::max(viewportRect.height, 1));

            scene->update(
                deltaTime,
                window,
                sceneInputEnabled,
                true,
                viewportRect.centerX(),
                viewportRect.centerY(),
                g_sceneClickPending,
                g_sceneClickX,
                g_sceneClickY
            );

            g_sceneClickPending = false;

            glDisable(GL_SCISSOR_TEST);

            if (useViewportFramebuffer)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, g_viewportFramebuffer.framebuffer);
                glViewport(0, 0, g_viewportFramebuffer.width, g_viewportFramebuffer.height);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                scene->renderSceneWithSelectionHighlight();
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }

            glViewport(0, 0, g_windowFramebufferWidth, g_windowFramebufferHeight);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if (useViewportFramebuffer)
            {
                const int destinationY = g_windowFramebufferHeight - viewportRect.y - viewportRect.height;
                g_viewportTexturePresenter.render(
                    g_viewportFramebuffer.colorTexture,
                    viewportRect.x,
                    destinationY,
                    viewportRect.width,
                    viewportRect.height
                );
                glViewport(0, 0, g_windowFramebufferWidth, g_windowFramebufferHeight);
            }
            else if (viewportRect.isValid())
            {
                const int glViewportY = g_windowFramebufferHeight - viewportRect.y - viewportRect.height;
                glEnable(GL_SCISSOR_TEST);
                glViewport(viewportRect.x, glViewportY, viewportRect.width, viewportRect.height);
                glScissor(viewportRect.x, glViewportY, viewportRect.width, viewportRect.height);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                scene->renderSceneWithSelectionHighlight();
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
            scene -> update(deltaTime, window, true, false, 0.0, 0.0, g_sceneClickPending, g_sceneClickX, g_sceneClickY);
            g_sceneClickPending = false;
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            scene -> renderSceneWithSelectionHighlight();

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

    } // Check if window was closed
    while(glfwWindowShouldClose(window) == 0 );


    // UNLOAD SCENE

    delete scene;

    g_viewportFramebuffer.destroy();
    g_viewportTexturePresenter.destroy();

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


