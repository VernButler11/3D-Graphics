#include <iostream>         // error handling and output
#include <cstdlib>          // EXIT_FAILURE

#include <GL/glew.h>        // GLEW library
#include "GLFW/glfw3.h"     // GLFW library

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "SceneManager.h"
#include "ViewManager.h"
#include "ShapeMeshes.h"
#include "ShaderManager.h"

// ===================== CAMERA STATE (M3 REQUIREMENT) =====================

glm::vec3 cameraPos = glm::vec3(0.0f, 8.0f, 15.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.3f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = -15.0f;
float lastX = 800.0f / 2.0f;
float lastY = 600.0f / 2.0f;
bool firstMouse = true;

float cameraSpeed = 5.0f;
bool  usePerspective = true;

// ===================== FORWARD DECLARATIONS =============================

bool InitializeGLFW();
bool InitializeGLEW();

void ProcessKeyboardInput(GLFWwindow* window, float deltaTime);
void MouseCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// Namespace for declaring global variables
namespace
{
    // Macro for window title
    const char* const WINDOW_TITLE = "7-1 FinalProject and Milestones";

    // Main GLFW window
    GLFWwindow* g_Window = nullptr;

    // scene manager object for managing the 3D scene prepare and render
    SceneManager* g_SceneManager = nullptr;
    // shader manager object for dynamic interaction with the shader code
    ShaderManager* g_ShaderManager = nullptr;
    // view manager object for managing the 3D view setup and projection to 2D
    ViewManager* g_ViewManager = nullptr;
}

/***********************************************************
 *  main(int, char*)
 ***********************************************************/
int main(int argc, char* argv[])
{
    // if GLFW fails initialization, then terminate the application
    if (InitializeGLFW() == false)
    {
        return EXIT_FAILURE;
    }

    // create shader and view managers
    g_ShaderManager = new ShaderManager();
    g_ViewManager = new ViewManager(g_ShaderManager);

    // create the main display window
    g_Window = g_ViewManager->CreateDisplayWindow(WINDOW_TITLE);

    // if GLEW fails initialization, then terminate the application
    if (InitializeGLEW() == false)
    {
        return EXIT_FAILURE;
    }

    // load the shader code from the external GLSL files
    g_ShaderManager->LoadShaders(
        "../../Utilities/shaders/vertexShader.glsl",
        "../../Utilities/shaders/fragmentShader.glsl");
    g_ShaderManager->use();

    // register input callbacks for Milestone 3
    glfwSetCursorPosCallback(g_Window, MouseCallback);
    glfwSetScrollCallback(g_Window, ScrollCallback);
    glfwSetInputMode(g_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // create the scene manager and prepare the 3D scene
    g_SceneManager = new SceneManager(g_ShaderManager);
    g_SceneManager->PrepareScene();

    // timing
    float lastFrame = 0.0f;

    // main loop
    while (!glfwWindowShouldClose(g_Window))
    {
        float currentFrame = (float)glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // process keyboard input for camera
        ProcessKeyboardInput(g_Window, deltaTime);

        // Enable z-depth
        glEnable(GL_DEPTH_TEST);

        // Clear the frame and z buffers
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ===================== VIEW & PROJECTION (M3 REQUIREMENT) =====================

        // compute view matrix from camera state
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        g_ShaderManager->setMat4Value("view", view);

        // compute projection matrix (perspective or orthographic)
        int width, height;
        glfwGetFramebufferSize(g_Window, &width, &height);
        float aspect = (height > 0) ? (float)width / (float)height : 4.0f / 3.0f;

        glm::mat4 projection;
        if (usePerspective)
        {
            projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        }
        else
        {
            float orthoSize = 10.0f;
            projection = glm::ortho(
                -orthoSize * aspect, orthoSize * aspect,
                -orthoSize, orthoSize,
                0.1f, 100.0f);
        }
        g_ShaderManager->setMat4Value("projection", projection);
        // ===================== LIGHTING (M5 REQUIREMENT) =====================

// camera position for specular lighting
        g_ShaderManager->setVec3Value("viewPosition", cameraPos);

        // ----- LIGHT 0 (main white point light) -----
        g_ShaderManager->setVec3Value("lightSources[0].position", glm::vec3(5.0f, 10.0f, 5.0f));
        g_ShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.2f, 0.2f, 0.2f));
        g_ShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(1.0f, 1.0f, 1.0f));
        g_ShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(1.0f, 1.0f, 1.0f));
        g_ShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
        g_ShaderManager->setFloatValue("lightSources[0].specularIntensity", 1.0f);

        // ----- DISABLE LIGHTS 1–3 -----
        for (int i = 1; i < 4; ++i)
        {
            std::string base = "lightSources[" + std::to_string(i) + "]";
            g_ShaderManager->setVec3Value((base + ".position").c_str(), glm::vec3(0.0f));
            g_ShaderManager->setVec3Value((base + ".ambientColor").c_str(), glm::vec3(0.0f));
            g_ShaderManager->setVec3Value((base + ".diffuseColor").c_str(), glm::vec3(0.0f));
            g_ShaderManager->setVec3Value((base + ".specularColor").c_str(), glm::vec3(0.0f));
            g_ShaderManager->setFloatValue((base + ".focalStrength").c_str(), 1.0f);
            g_ShaderManager->setFloatValue((base + ".specularIntensity").c_str(), 0.0f);
        }

        // ----- MATERIAL FOR LIT SURFACES (PLANE) -----
        g_ShaderManager->setVec3Value("material.ambientColor", glm::vec3(0.8f, 0.8f, 0.8f));
        g_ShaderManager->setFloatValue("material.ambientStrength", 0.3f);
        g_ShaderManager->setVec3Value("material.diffuseColor", glm::vec3(0.9f, 0.9f, 0.9f));
        g_ShaderManager->setVec3Value("material.specularColor", glm::vec3(1.0f, 1.0f, 1.0f));
        g_ShaderManager->setFloatValue("material.shininess", 32.0f);


        // (Optional) still let ViewManager handle any additional state if needed
        // g_ViewManager->PrepareSceneView();  // not required now for camera

        // refresh the 3D scene
        g_SceneManager->RenderScene();

        // swap buffers and poll events
        glfwSwapBuffers(g_Window);
        glfwPollEvents();
    }

    // cleanup
    if (NULL != g_SceneManager)
    {
        delete g_SceneManager;
        g_SceneManager = NULL;
    }
    if (NULL != g_ViewManager)
    {
        delete g_ViewManager;
        g_ViewManager = NULL;
    }
    if (NULL != g_ShaderManager)
    {
        delete g_ShaderManager;
        g_ShaderManager = NULL;
    }

    exit(EXIT_SUCCESS);
}

/***********************************************************
 *	InitializeGLFW()
 ***********************************************************/
bool InitializeGLFW()
{
    glfwInit();

#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    return true;
}

/***********************************************************
 *	InitializeGLEW()
 ***********************************************************/
bool InitializeGLEW()
{
    GLenum GLEWInitResult = GLEW_OK;

    GLEWInitResult = glewInit();
    if (GLEW_OK != GLEWInitResult)
    {
        std::cerr << glewGetErrorString(GLEWInitResult) << std::endl;
        return false;
    }

    std::cout << "INFO: OpenGL Successfully Initialized\n";
    std::cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << "\n" << std::endl;

    return true;
}

/***********************************************************
 *  ProcessKeyboardInput()
 *  Milestone 3: WASD + QE camera navigation
 ***********************************************************/
void ProcessKeyboardInput(GLFWwindow* window, float deltaTime)
{
    float velocity = cameraSpeed * deltaTime;

    // Forward / Backward
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraFront * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraFront * velocity;

    // Left / Right
    glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= cameraRight * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += cameraRight * velocity;

    // Up / Down
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        cameraPos -= cameraUp * velocity;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cameraPos += cameraUp * velocity;

    // Projection toggle
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        usePerspective = true;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
        usePerspective = false;
}

/***********************************************************
 *  MouseCallback()
 *  Milestone 3: mouse look
 ***********************************************************/
void MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

/***********************************************************
 *  ScrollCallback()
 *  Milestone 3: scroll to adjust movement speed
 ***********************************************************/
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    cameraSpeed += (float)yoffset;
    if (cameraSpeed < 1.0f)  cameraSpeed = 1.0f;
    if (cameraSpeed > 20.0f) cameraSpeed = 20.0f;
}