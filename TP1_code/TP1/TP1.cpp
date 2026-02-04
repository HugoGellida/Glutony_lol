// Include standard headers
#include <stdio.h>
#include <stdlib.h>
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


void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
glm::vec3 camera_position   = glm::vec3(0.0f, 0.0f,  3.0f);
glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camera_up    = glm::vec3(0.0f, 1.0f,  0.0f);

// SCENE
Scene * scene;


// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

int mousePX = 0;
int mousePY = 0;
bool fpsControl = false;
bool gWasPressed = false;

//rotation
float angle = 0.;
float zoom = 1.;
/*******************************************************************************/


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    scene -> updateCamSettings((float)width/(float)height);
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
    window = glfwCreateWindow( 1024, 768, "TP1 - GLFW", NULL, NULL);
    if( window == NULL ){
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

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

    // Dark blue background
    glClearColor(0.8f, 0.8f, 0.8f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LEQUAL);

    // Cull triangles which normal is not towards the camera
    //glEnable(GL_CULL_FACE);

    // LOAD SCENE    
    scene = new Scene();
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    

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

        // input
        // -----
        processInput(window);

        scene -> update(deltaTime);
        // UPDATE
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // RENDER
        scene -> renderScene();
        

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 );


    // UNLOAD SCENE

    delete scene;

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE && gWasPressed) // TODO: switch to click and shift or alt   
    {
        gWasPressed = false;
        fpsControl = !fpsControl;
        if (fpsControl) // disable cursor
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        else
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        std::cout << "FPS CONTROLS - " << (fpsControl ? "ON" : "OFF") << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
        gWasPressed = true;

    if (fpsControl)
    {

        //Camera zoom in and out
        float cameraSpeed = 2.5 * deltaTime;
        glm::vec3 move = glm::vec3(0, 0, 0);
        unsigned int a = 0;
        // CAMERA
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            move+=glm::vec3(0, 0, -1);
            a++;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            move+=glm::vec3(0, 0, 1);
            a++;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            move+=glm::vec3(-1, 0, 0);
            a++;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            move+=glm::vec3(1, 0, 0);
            a++;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        {
            move+=glm::vec3(0, 1, 0);
            a++;
        }
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        {
            move+=glm::vec3(0, -1, 0);
            a++;
        }

        // mouse
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        if (mousePX - mouseX != 0 || mousePY - mouseY != 0)
        {
            float sensitivity = 0.1f;
            float xoffset = mouseX - mousePX;
            float yoffset = mousePY - mouseY; // reversed since y-coordinates go from bottom to top

            scene -> updateCamera(glm::vec3(0, 0, 0), glm::vec3(-yoffset * sensitivity, xoffset * sensitivity, 0.0f));
            // reset mouse pos
            glfwSetCursorPos(window, 1024/2, 768/2);
            mousePX = 1024/2;
            mousePY = 768/2;
        }
        else
        {
            mousePX = mouseX;
            mousePY = mouseY;
        }

        if (a>0)
            scene -> updateCamera(deltaTime * (move / (float)a), glm::vec3(0, 0, 0));
    }
}


