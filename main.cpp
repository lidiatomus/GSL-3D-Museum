#if defined(__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "stb_image.h"

#include <iostream>
#include <string>
#include <algorithm>

// FUNCTION PROTOTYPES

// Initialization functions
void initOpenGLWindow();
void initOpenGLState();
void initShaders();
void initUniforms();
void initModels();
void initQuad();
void initCube();
void initShadowMap();
void initWindowShadowMap();
void initDust();

// Rendering functions
void renderScene();
void renderRoom(gps::Shader& shader);
void renderObjects(gps::Shader& shader);
void renderDecor(gps::Shader& shader);
void renderLampFixtures(gps::Shader& shader);
void renderDust(gps::Shader& shader);

// Shadow pass rendering
void renderRoomShadow(gps::Shader& sh);
void renderObjectsShadow(gps::Shader& sh);
void renderDecorShadow(gps::Shader& sh);

// Drawing primitives
void drawTexturedQuad(
    gps::Shader& shader, const glm::mat4& M,
    GLuint diffuseTex, GLuint specTex, GLuint roughTex, GLuint normalTex,
    glm::vec2 tiling,
    int isOutside = 0,
    int isGlass = 0,
    float glassFactor = 0.25f,
    GLuint opacityTex = 0       // NEW
);

void drawTexturedCube(
    gps::Shader& shader,
    const glm::mat4& M,
    GLuint diffuseTex, GLuint specTex, GLuint roughTex, GLuint normalTex,
    glm::vec2 tiling,
    int isGlass = 0,
    float glassFactor = 1.0f,
    GLuint opacityTex = 0
);

void drawModel(gps::Shader& shader, gps::Model3D& mdl, const glm::mat4& M);

// Shadow drawing functions
void drawShadowQuad(gps::Shader& sh, const glm::mat4& M);
void drawShadowCube(gps::Shader& sh, const glm::mat4& M);
void drawShadowModel(gps::Shader& sh, gps::Model3D& mdl, const glm::mat4& M);

// Light space matrix computation
glm::mat4 computeLightSpaceMatrix();
glm::mat4 computeWindowLightSpaceMatrix();

// Camera and input handling
void processMovement();
void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void windowResizeCallback(GLFWwindow* window, int width, int height);
void clampCameraInsideRoom();
void clampPersonInsideRoom();

// Utilities
GLenum glCheckError_(const char* file, int line);
void setWindowCallbacks();
void cleanup();
void setAnisotropy(GLuint texId);
void uploadSpotlights(gps::Shader& shader);

#define glCheckError() glCheckError_(__FILE__, __LINE__)

// SPOTLIGHT STRUCTURE

struct SpotlightCPU {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float cutoff;
    float outerCutoff;
    float constant;
    float linear;
    float quadratic;
};

// DUST MOTE STRUCTURE
struct DustMote {
    glm::vec3 position;
    float speed;
    float phase;
};

// GLOBAL VARIABLES - TEXTURES

GLuint quadVAO = 0, quadVBO = 0;
GLuint cubeVAO = 0, cubeVBO = 0;

GLuint floorDiffuse = 0;
GLuint floorSpecular = 0;
GLuint floorRoughness = 0;
GLuint floorNormal = 0;

GLuint wallDiffuse = 0;
GLuint wallSpecular = 0;
GLuint wallRoughness = 0;
GLuint wallNormal = 0;

GLuint glassDiffuse = 0;
GLuint glassSpec = 0;
GLuint glassRough = 0;
GLuint glassNormal = 0;
GLuint glassOpacity = 0;

GLuint outsideTex = 0;

// GLOBAL VARIABLES - CAMERA & INPUT

gps::Window myWindow;
gps::Camera myCamera(
    glm::vec3(0.0f, 1.6f, 6.0f),
    glm::vec3(0.0f, 1.6f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f)
);

GLfloat cameraSpeed = 0.1f;
GLboolean pressedKeys[1024];

float yaw = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;
double lastX = 512.0, lastY = 384.0;

bool gWireframe = false;
int gFlat = 0;

// GLOBAL VARIABLES - MODELS

gps::Model3D teapot;
gps::Model3D statueAntonius;
gps::Model3D statueJudas;
gps::Model3D statueKrieger;
gps::Model3D egyptDoor;
gps::Model3D museumEntrance;
gps::Model3D horrorPainting;
gps::Model3D person;

glm::vec3 personPos(0.0f, 0.0f, 2.0f);
float personYaw = 180.0f;
bool personAnimate = false;
float personAnimT = 0.0f;

std::vector<DustMote> motes;
const int NUM_MOTES = 250;

// GLOBAL VARIABLES - SPOTLIGHTS

SpotlightCPU spots[3] = {
    {
        glm::vec3(-3.0f, 2.5f, -2.0f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(1.0f, 0.9f, 0.8f),
        glm::cos(glm::radians(12.5f)),
        glm::cos(glm::radians(17.5f)),
        1.0f, 0.09f, 0.032f
    },
    {
        glm::vec3(2.0f, 2.5f, -2.0f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(0.8f, 0.9f, 1.0f),
        glm::cos(glm::radians(12.5f)),
        glm::cos(glm::radians(17.5f)),
        1.0f, 0.09f, 0.032f
    },
    {
        glm::vec3(4.0f, 2.5f, -2.0f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(0.9f, 1.0f, 0.9f),
        glm::cos(glm::radians(12.5f)),
        glm::cos(glm::radians(17.5f)),
        1.0f, 0.09f, 0.032f
    }
};

// GLOBAL VARIABLES - MATRICES & SHADERS
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

gps::Shader myBasicShader;
gps::Shader shadowShader;

GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;

glm::vec3 lightDir;
glm::vec3 lightColor;
GLfloat angle = 0.0f;

// GLOBAL VARIABLES - SHADOWS

glm::mat4 lightSpaceMatrix;
GLuint shadowFBO = 0;
GLuint shadowDepthTex = 0;
const GLuint SHADOW_SIZE = 2048;

glm::mat4 windowLightSpaceMatrix;
GLuint windowShadowFBO = 0;
GLuint windowShadowDepthTex = 0;
const GLuint WINDOW_SHADOW_SIZE = 2048;

glm::vec3 windowLightDir = glm::normalize(glm::vec3(0.0f, -0.2f, 1.0f));
glm::vec3 windowLightColor = glm::vec3(0.6f, 0.7f, 0.9f);

// GLOBAL VARIABLES - FOG

float fogDensity = 0.05f;
glm::vec3 fogColor = glm::vec3(0.5f, 0.5f, 0.5f);

// UTILITY FUNCTIONS

GLenum glCheckError_(const char* file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM:
            error = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error = "INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            error = "INVALID_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            error = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}

void setAnisotropy(GLuint texId) {
    glBindTexture(GL_TEXTURE_2D, texId);
    if (GLEW_EXT_texture_filter_anisotropic) {
        float maxA = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxA);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxA);
    }
}

// INITIALIZATION FUNCTIONS

void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project Core");
}

void initOpenGLState() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void initShaders() {
    myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    shadowShader.loadShader("shaders/shadow_depth.vert", "shaders/shadow_depth.frag");
}

void initUniforms() {
    myBasicShader.useShaderProgram();

    modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");
    viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");
    lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
    lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.3f));
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));

    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    projection = glm::perspective(glm::radians(45.0f),
        (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
        0.1f, 20.0f);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    lightDir = glm::normalize(glm::vec3(-1.0f, 1.0f, 0.3f));
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

    lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
}

void initModels() {
    teapot.LoadModel("models/teapot/teapot20segUT.obj");
    statueAntonius.LoadModel("models/teapot/hl-antonius/Antonius_C.obj");
    statueJudas.LoadModel("models/teapot/hl-judas-thaddaus/Judas_C.obj");
    statueKrieger.LoadModel("models/teapot/kriegerdenkmal/Kriegerdenkmal_C.obj");
    horrorPainting.LoadModel("models/teapot/horror-paintings-zdzislaw-beksinski/Paintings.obj");
    egyptDoor.LoadModel("models/teapot/egyptian-limestone-door-scan-ashmolean-museum/textured_output.obj");
    museumEntrance.LoadModel("models/teapot/museum-of-london-staff-entrance/MuseumOfLondonStaffEntrance03.obj");
    person.LoadModel("models/teapot/person/Duda.obj");
}

void initQuad() {
    float quadVertices[] = {
        -0.5f, 0.0f, -0.5f,  0, 1, 0,  0, 0,
         0.5f, 0.0f, -0.5f,  0, 1, 0,  1, 0,
         0.5f, 0.0f,  0.5f,  0, 1, 0,  1, 1,
        -0.5f, 0.0f, -0.5f,  0, 1, 0,  0, 0,
         0.5f, 0.0f,  0.5f,  0, 1, 0,  1, 1,
        -0.5f, 0.0f,  0.5f,  0, 1, 0,  0, 1
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void initCube() {
    float v[] = {
        // +X
        0.5f, -0.5f, -0.5f,  1, 0, 0,  0, 0,
        0.5f, -0.5f,  0.5f,  1, 0, 0,  1, 0,
        0.5f,  0.5f,  0.5f,  1, 0, 0,  1, 1,
        0.5f, -0.5f, -0.5f,  1, 0, 0,  0, 0,
        0.5f,  0.5f,  0.5f,  1, 0, 0,  1, 1,
        0.5f,  0.5f, -0.5f,  1, 0, 0,  0, 1,
        // -X
        -0.5f, -0.5f,  0.5f,  -1, 0, 0,  0, 0,
        -0.5f, -0.5f, -0.5f,  -1, 0, 0,  1, 0,
        -0.5f,  0.5f, -0.5f,  -1, 0, 0,  1, 1,
        -0.5f, -0.5f,  0.5f,  -1, 0, 0,  0, 0,
        -0.5f,  0.5f, -0.5f,  -1, 0, 0,  1, 1,
        -0.5f,  0.5f,  0.5f,  -1, 0, 0,  0, 1,
        // +Y
        -0.5f,  0.5f, -0.5f,  0, 1, 0,  0, 0,
         0.5f,  0.5f, -0.5f,  0, 1, 0,  1, 0,
         0.5f,  0.5f,  0.5f,  0, 1, 0,  1, 1,
        -0.5f,  0.5f, -0.5f,  0, 1, 0,  0, 0,
         0.5f,  0.5f,  0.5f,  0, 1, 0,  1, 1,
        -0.5f,  0.5f,  0.5f,  0, 1, 0,  0, 1,
        // -Y
        -0.5f, -0.5f,  0.5f,  0, -1, 0,  0, 0,
         0.5f, -0.5f,  0.5f,  0, -1, 0,  1, 0,
         0.5f, -0.5f, -0.5f,  0, -1, 0,  1, 1,
        -0.5f, -0.5f,  0.5f,  0, -1, 0,  0, 0,
         0.5f, -0.5f, -0.5f,  0, -1, 0,  1, 1,
        -0.5f, -0.5f, -0.5f,  0, -1, 0,  0, 1,
        // +Z
        -0.5f, -0.5f,  0.5f,  0, 0, 1,  0, 0,
        -0.5f,  0.5f,  0.5f,  0, 0, 1,  0, 1,
         0.5f,  0.5f,  0.5f,  0, 0, 1,  1, 1,
        -0.5f, -0.5f,  0.5f,  0, 0, 1,  0, 0,
         0.5f,  0.5f,  0.5f,  0, 0, 1,  1, 1,
         0.5f, -0.5f,  0.5f,  0, 0, 1,  1, 0,
         // -Z
          0.5f, -0.5f, -0.5f,  0, 0, -1,  0, 0,
          0.5f,  0.5f, -0.5f,  0, 0, -1,  0, 1,
         -0.5f,  0.5f, -0.5f,  0, 0, -1,  1, 1,
          0.5f, -0.5f, -0.5f,  0, 0, -1,  0, 0,
         -0.5f,  0.5f, -0.5f,  0, 0, -1,  1, 1,
         -0.5f, -0.5f, -0.5f,  0, 0, -1,  1, 0
    };

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void initShadowMap() {
    glGenFramebuffers(1, &shadowFBO);

    glGenTextures(1, &shadowDepthTex);
    glBindTexture(GL_TEXTURE_2D, shadowDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_SIZE, SHADOW_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border[] = { 1, 1, 1, 1 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowDepthTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initWindowShadowMap() {
    glGenFramebuffers(1, &windowShadowFBO);

    glGenTextures(1, &windowShadowDepthTex);
    glBindTexture(GL_TEXTURE_2D, windowShadowDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        WINDOW_SHADOW_SIZE, WINDOW_SHADOW_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border[] = { 1, 1, 1, 1 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    glBindFramebuffer(GL_FRAMEBUFFER, windowShadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, windowShadowDepthTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initDust() {
    for (int i = 0; i < NUM_MOTES; i++) {
        DustMote m;
        m.position = glm::vec3(
            ((rand() % 100) / 10.0f) - 5.0f,
            ((rand() % 40) / 10.0f),
            ((rand() % 100) / 10.0f) - 8.0f
        );
        m.speed = ((rand() % 100) / 2000.0f) + 0.005f;
        m.phase = (rand() % 100) / 10.0f;
        motes.push_back(m);
    }
}

void setWindowCallbacks() {
    glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}

// INPUT & CALLBACK FUNCTIONS

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_F2 && action == GLFW_PRESS)
        gFlat = 1 - gFlat;

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS)
            pressedKeys[key] = true;
        else if (action == GLFW_RELEASE)
            pressedKeys[key] = false;
    }

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_F1) {
            gWireframe = !gWireframe;
            glPolygonMode(GL_FRONT_AND_BACK, gWireframe ? GL_LINE : GL_FILL);
        }
    }

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_LEFT)
            personYaw += 3.0f;
        if (key == GLFW_KEY_RIGHT)
            personYaw -= 3.0f;

        float speed = 0.1f;
        if (key == GLFW_KEY_UP) {
            personPos.x += sin(glm::radians(personYaw)) * speed;
            personPos.z += cos(glm::radians(personYaw)) * speed;
        }
        if (key == GLFW_KEY_DOWN) {
            personPos.x -= sin(glm::radians(personYaw)) * speed;
            personPos.z -= cos(glm::radians(personYaw)) * speed;
        }

        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
            personAnimate = !personAnimate;
    }

    if (action == GLFW_RELEASE)
        pressedKeys[key] = false;

    myBasicShader.useShaderProgram();
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "useFlatShading"), gFlat);
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = float(xpos - lastX);
    float yoffset = float(lastY - ypos);

    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.08f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;
    pitch = std::clamp(pitch, -89.0f, 89.0f);

    myCamera.rotate(pitch, yaw);
    view = myCamera.getViewMatrix();

    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "Window resized! New width: %d, height: %d\n", width, height);

    glViewport(0, 0, width, height);
    projection = glm::perspective(glm::radians(45.0f),
        (float)width / (float)height, 0.1f, 20.0f);

    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

void processMovement() {
    bool moved = false;

    if (pressedKeys[GLFW_KEY_W]) {
        myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
        moved = true;
    }
    if (pressedKeys[GLFW_KEY_S]) {
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        moved = true;
    }
    if (pressedKeys[GLFW_KEY_A]) {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        moved = true;
    }
    if (pressedKeys[GLFW_KEY_D]) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        moved = true;
    }

    if (moved) {
        clampCameraInsideRoom();
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_Q]) {
        angle -= 1.0f;
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }
    if (pressedKeys[GLFW_KEY_E]) {
        angle += 1.0f;
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }
}

void clampCameraInsideRoom() {
    const float W = 12.0f, D = 16.0f;
    const float margin = 0.35f;
    glm::vec3 p = myCamera.getPosition();

    p.x = std::clamp(p.x, -W * 0.5f + margin, W * 0.5f - margin);
    p.z = std::clamp(p.z, -D * 0.5f + margin, D * 0.5f - margin);
    p.y = 1.6f;

    glm::vec3 objPos[3] = {
        glm::vec3(-3.0f, 0.0f, -2.0f),
        glm::vec3(2.0f, 0.0f, -2.0f),
        glm::vec3(4.0f, 0.0f, -2.0f)
    };

    float hSize = 0.45f + margin;

    for (int i = 0; i < 3; i++) {
        float dx = p.x - objPos[i].x;
        float dz = p.z - objPos[i].z;

        if (std::abs(dx) < hSize && std::abs(dz) < hSize) {
            if (std::abs(dx) > std::abs(dz)) {
                p.x = objPos[i].x + (dx > 0 ? hSize : -hSize);
            }
            else {
                p.z = objPos[i].z + (dz > 0 ? hSize : -hSize);
            }
        }
    }

    myCamera.setPosition(p);
}

void clampPersonInsideRoom() {
    const float W = 12.0f, D = 16.0f;
    const float pMargin = 0.5f;

    personPos.x = std::clamp(personPos.x, -W * 0.5f + pMargin, W * 0.5f - pMargin);
    personPos.z = std::clamp(personPos.z, -D * 0.5f + pMargin, D * 0.5f - pMargin);
    personPos.y = 0.0f;

    glm::vec3 objPos[3] = {
        glm::vec3(-3.0f, 0.0f, -2.0f),
        glm::vec3(2.0f, 0.0f, -2.0f),
        glm::vec3(4.0f, 0.0f, -2.0f)
    };

    float hSize = 0.45f + pMargin;

    for (int i = 0; i < 3; i++) {
        float dx = personPos.x - objPos[i].x;
        float dz = personPos.z - objPos[i].z;

        if (std::abs(dx) < hSize && std::abs(dz) < hSize) {
            if (std::abs(dx) > std::abs(dz)) {
                personPos.x = objPos[i].x + (dx > 0 ? hSize : -hSize);
            }
            else {
                personPos.z = objPos[i].z + (dz > 0 ? hSize : -hSize);
            }
        }
    }
}

// DRAWING FUNCTIONS

void drawTexturedQuad(
    gps::Shader& shader,
    const glm::mat4& M,
    GLuint diffuseTex, GLuint specTex, GLuint roughTex, GLuint normalTex,
    glm::vec2 tiling,
    int isOutside,
    int isGlass,
    float glassFactor,
    GLuint opacityTex,
    glm::vec2 uvMin = glm::vec2(0.0f, 0.0f),
    glm::vec2 uvMax = glm::vec2(1.0f, 1.0f),
    glm::vec2 uvOffset = glm::vec2(0.0f, 0.0f)
)
{
    shader.useShaderProgram();

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(M));
    glm::mat3 NM = glm::mat3(glm::inverseTranspose(view * M));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(NM));

    // UV controls 
    glUniform2fv(glGetUniformLocation(shader.shaderProgram, "uvTiling"), 1, glm::value_ptr(tiling));
    glUniform2fv(glGetUniformLocation(shader.shaderProgram, "uvOffset"), 1, glm::value_ptr(uvOffset));
    glUniform2fv(glGetUniformLocation(shader.shaderProgram, "uvMin"), 1, glm::value_ptr(uvMin));
    glUniform2fv(glGetUniformLocation(shader.shaderProgram, "uvMax"), 1, glm::value_ptr(uvMax));

    // textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseTex);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "diffuseTexture"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specTex);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "specularTexture"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, roughTex);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "roughnessTexture"), 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, normalTex ? normalTex : 0);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "normalTexture"), 3);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "useNormalMap"), normalTex ? 1 : 0);

    // flags
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "isOutside"), isOutside);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "isGlass"), isGlass);
    glUniform1f(glGetUniformLocation(shader.shaderProgram, "glassFactor"), glassFactor);

    // opacity map only for glass
    int useOp = (isGlass == 1 && opacityTex != 0) ? 1 : 0;
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "useOpacityMap"), useOp);

    if (useOp)
    {
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, opacityTex);
        glUniform1i(glGetUniformLocation(shader.shaderProgram, "opacityTexture"), 4);
    }

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}


void drawTexturedCube(
    gps::Shader& shader,
    const glm::mat4& M,
    GLuint diffuseTex, GLuint specTex, GLuint roughTex, GLuint normalTex,
    glm::vec2 tiling,
    int isGlass,
    float glassFactor,
    GLuint opacityTex) {

    shader.useShaderProgram();

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(M));
    glm::mat3 NM = glm::mat3(glm::inverseTranspose(view * M));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(NM));

    glUniform2fv(glGetUniformLocation(shader.shaderProgram, "uvTiling"), 1, glm::value_ptr(tiling));
    glUniform2fv(glGetUniformLocation(shader.shaderProgram, "uvMin"), 1, glm::value_ptr(glm::vec2(0.0f)));
    glUniform2fv(glGetUniformLocation(shader.shaderProgram, "uvMax"), 1, glm::value_ptr(glm::vec2(1.0f)));
    glUniform2fv(glGetUniformLocation(shader.shaderProgram, "uvOffset"), 1, glm::value_ptr(glm::vec2(0.0f)));

    glUniform1i(glGetUniformLocation(shader.shaderProgram, "isOutside"), 0);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "isGlass"), isGlass);
    glUniform1f(glGetUniformLocation(shader.shaderProgram, "glassFactor"), glassFactor);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseTex);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "diffuseTexture"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specTex);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "specularTexture"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, roughTex);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "roughnessTexture"), 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, normalTex ? normalTex : 0);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "normalTexture"), 3);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "useNormalMap"), normalTex ? 1 : 0);

    int useOp = (isGlass == 1 && opacityTex != 0) ? 1 : 0;
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "useOpacityMap"), useOp);
    if (useOp) {
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, opacityTex);
        glUniform1i(glGetUniformLocation(shader.shaderProgram, "opacityTexture"), 4);
    }

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void drawModel(
    gps::Shader& shader,
    gps::Model3D& mdl,
    const glm::mat4& M) {

    shader.useShaderProgram();

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(M));
    glm::mat3 NM = glm::mat3(glm::inverseTranspose(view * M));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(NM));

    glUniform2fv(glGetUniformLocation(shader.shaderProgram, "uvTiling"), 1, glm::value_ptr(glm::vec2(1.0f)));
    glUniform2fv(glGetUniformLocation(shader.shaderProgram, "uvOffset"), 1, glm::value_ptr(glm::vec2(0.0f)));
    glUniform2fv(glGetUniformLocation(shader.shaderProgram, "uvMin"), 1, glm::value_ptr(glm::vec2(0.0f)));
    glUniform2fv(glGetUniformLocation(shader.shaderProgram, "uvMax"), 1, glm::value_ptr(glm::vec2(1.0f)));

    glUniform1i(glGetUniformLocation(shader.shaderProgram, "isOutside"), 0);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "isGlass"), 0);
    glUniform1f(glGetUniformLocation(shader.shaderProgram, "glassFactor"), 1.0f);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "useOpacityMap"), 0);

    mdl.Draw(shader);
}

// SHADOW DRAWING FUNCTIONS

void drawShadowQuad(gps::Shader& sh, const glm::mat4& M) {
    sh.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(sh.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(M));
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void drawShadowCube(gps::Shader& sh, const glm::mat4& M) {
    sh.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(sh.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(M));
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void drawShadowModel(gps::Shader& sh, gps::Model3D& mdl, const glm::mat4& M) {
    sh.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(sh.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(M));
    mdl.Draw(sh);
}

// LIGHT SPACE MATRIX COMPUTATION

glm::mat4 computeLightSpaceMatrix() {
    glm::vec3 center(0.0f, 1.5f, 0.0f);
    glm::vec3 lightPos = center - lightDir * 10.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, center, glm::vec3(0, 1, 0));

    float orthoSize = 12.0f;
    glm::mat4 lightProj = glm::ortho(
        -orthoSize, orthoSize,
        -orthoSize, orthoSize,
        1.0f, 25.0f
    );

    return lightProj * lightView;
}

glm::mat4 computeWindowLightSpaceMatrix() {
    glm::vec3 center(0.0f, 1.5f, 0.0f);
    glm::vec3 lightPos = center - windowLightDir * 10.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, center, glm::vec3(0, 1, 0));

    float orthoSize = 10.0f;
    glm::mat4 lightProj = glm::ortho(
        -orthoSize, orthoSize,
        -orthoSize, orthoSize,
        1.0f, 25.0f
    );

    return lightProj * lightView;
}

void uploadSpotlights(gps::Shader& shader) {
    shader.useShaderProgram();

    const int N = 3;
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "numSpots"), N);

    float intensity[N] = { 2.0f, 2.0f, 2.0f };

    for (int i = 0; i < N; i++) {
        glUniform3fv(glGetUniformLocation(shader.shaderProgram, ("spotPos[" + std::to_string(i) + "]").c_str()),
            1, glm::value_ptr(spots[i].position));

        glUniform3fv(glGetUniformLocation(shader.shaderProgram, ("spotDir[" + std::to_string(i) + "]").c_str()),
            1, glm::value_ptr(glm::normalize(spots[i].direction)));

        glUniform3fv(glGetUniformLocation(shader.shaderProgram, ("spotColor[" + std::to_string(i) + "]").c_str()),
            1, glm::value_ptr(spots[i].color));

        glUniform1f(glGetUniformLocation(shader.shaderProgram, ("spotCutOff[" + std::to_string(i) + "]").c_str()),
            spots[i].cutoff);

        glUniform1f(glGetUniformLocation(shader.shaderProgram, ("spotOuterCutOff[" + std::to_string(i) + "]").c_str()),
            spots[i].outerCutoff);

        glUniform1f(glGetUniformLocation(shader.shaderProgram, ("spotIntensity[" + std::to_string(i) + "]").c_str()),
            intensity[i]);

        glUniform1f(glGetUniformLocation(shader.shaderProgram, ("spotConstant[" + std::to_string(i) + "]").c_str()),
            spots[i].constant);

        glUniform1f(glGetUniformLocation(shader.shaderProgram, ("spotLinear[" + std::to_string(i) + "]").c_str()),
            spots[i].linear);

        glUniform1f(glGetUniformLocation(shader.shaderProgram, ("spotQuadratic[" + std::to_string(i) + "]").c_str()),
            spots[i].quadratic);
    }
}

// RENDERING FUNCTIONS

void renderRoom(gps::Shader& shader) {
    float W = 12.0f;
    float D = 16.0f;
    float H = 4.0f;

    glm::vec2 tileFloor(6.0f, 6.0f);
    glm::vec2 tileWall(4.0f, 2.0f);

    const glm::vec2 WIN_UV_MIN(0.14648f, 0.24707f);
    const glm::vec2 WIN_UV_MAX(0.85254f, 0.75195f);

    auto drawWallQuad = [&](const glm::mat4& M, glm::vec2 tiling) {
        drawTexturedQuad(shader, M,
            wallDiffuse, wallSpecular, wallRoughness, wallNormal,
            tiling, 0, 0, 1.0f, 0,
            glm::vec2(0.0f), glm::vec2(1.0f));
        };

    // Opaque pass
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    // Floor
    {
        glm::mat4 M(1.0f);
        M = glm::scale(M, glm::vec3(W, 1.0f, D));
        drawTexturedQuad(shader, M,
            floorDiffuse, floorSpecular, floorRoughness, floorNormal,
            tileFloor, 0, 0, 1.0f, 0,
            glm::vec2(0.0f), glm::vec2(1.0f));
    }

    // Ceiling
    {
        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(0.0f, H, 0.0f));
        M = glm::rotate(M, glm::radians(180.0f), glm::vec3(1, 0, 0));
        M = glm::scale(M, glm::vec3(W, 1.0f, D));
        drawWallQuad(M, tileWall);
    }

    // Front wall
    {
        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(0.0f, H * 0.5f, D * 0.5f));
        M = glm::rotate(M, glm::radians(-90.0f), glm::vec3(1, 0, 0));
        M = glm::scale(M, glm::vec3(W, 1.0f, H));
        drawWallQuad(M, tileWall);
    }

    // Left wall
    {
        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(-W * 0.5f, H * 0.5f, 0.0f));
        M = glm::rotate(M, glm::radians(90.0f), glm::vec3(0, 0, 1));
        M = glm::scale(M, glm::vec3(H, 1.0f, D));
        drawWallQuad(M, tileWall);
    }

    // Right wall
    {
        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(W * 0.5f, H * 0.5f, 0.0f));
        M = glm::rotate(M, glm::radians(-90.0f), glm::vec3(0, 0, 1));
        M = glm::scale(M, glm::vec3(H, 1.0f, D));
        drawWallQuad(M, tileWall);
    }

    // Back wall with window hole
    float zBack = -D * 0.5f;
    float winW = 4.5f;
    float winH = 2.2f;
    float winBottom = 1.2f;
    float winTop = winBottom + winH;

    auto backPiece = [&](float xCenter, float yCenter, float xSize, float ySize) {
        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(xCenter, yCenter, zBack));
        M = glm::rotate(M, glm::radians(90.0f), glm::vec3(1, 0, 0));
        M = glm::scale(M, glm::vec3(xSize, 1.0f, ySize));
        drawWallQuad(M, tileWall);
        };

    float sideW = (W - winW) * 0.5f;
    backPiece(-W * 0.5f + sideW * 0.5f, H * 0.5f, sideW, H);
    backPiece(W * 0.5f - sideW * 0.5f, H * 0.5f, sideW, H);
    backPiece(0.0f, winBottom * 0.5f, winW, winBottom);
    backPiece(0.0f, (winTop + H) * 0.5f, winW, (H - winTop));

    // Sky background
    {
        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(0.0f, winBottom + winH * 0.5f, zBack - 0.5f));
        M = glm::rotate(M, glm::radians(90.0f), glm::vec3(1, 0, 0));
        M = glm::scale(M, glm::vec3(winW * 1.2f, 1.0f, winH * 1.2f));

        drawTexturedQuad(shader, M,
            outsideTex, 0, 0, 0,
            glm::vec2(1.0f, 1.0f), 1, 0, 1.0f, 0,
            glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f));
    }

    // Window pane (transparent)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    {
        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(0.0f, winBottom + winH * 0.5f, zBack + 0.01f));
        M = glm::rotate(M, glm::radians(90.0f), glm::vec3(1, 0, 0));
        M = glm::scale(M, glm::vec3(winW, 1.0f, winH));

        drawTexturedQuad(shader, M,
            glassDiffuse, glassSpec, glassRough, glassNormal,
            glm::vec2(1.0f, 1.0f), 0, 1, 0.4f,
            glassOpacity,
            WIN_UV_MIN, WIN_UV_MAX);
    }
    glDepthMask(GL_TRUE);
}

void renderLampFixtures(gps::Shader& shader) {
    shader.useShaderProgram();

    float H = 4.0f;

    for (int i = 0; i < 3; i++) {
        glm::mat4 M(1.0f);
        M = glm::translate(M, spots[i].position + glm::vec3(0.0f, H - spots[i].position.y - 0.1f, 0.0f));
        M = glm::scale(M, glm::vec3(0.4f, 0.05f, 0.4f));

        glUniform1i(glGetUniformLocation(shader.shaderProgram, "isOutside"), 0);
        drawTexturedCube(shader, M, wallDiffuse, wallSpecular, wallRoughness, wallNormal, glm::vec2(1.0f));
    }
}

void renderObjects(gps::Shader& shader) {
    float t = (float)glfwGetTime();

    glm::vec3 objPos[3] = {
        glm::vec3(-3.0f, 0.0f, -2.0f),
        glm::vec3(2.0f, 0.0f, -2.0f),
        glm::vec3(4.0f, 0.0f, -2.0f)
    };

    glm::vec3 pedestalScale(0.9f, 1.2f, 0.9f);
    float pedestalCenterY = 0.6f;
    float pedestalTopY = pedestalCenterY + pedestalScale.y * 0.5f;

    GLuint diff = wallDiffuse;
    GLuint spec = wallSpecular;
    GLuint rough = wallRoughness;
    GLuint norm = wallNormal;

    for (int i = 0; i < 3; i++) {
        float spin = t * (40.0f + 15.0f * i);
        float bob = 0.05f * sin(t * 2.0f + (float)i);

        // Pedestal
        {
            glm::mat4 M(1.0f);
            M = glm::translate(M, objPos[i] + glm::vec3(0.0f, pedestalCenterY, 0.0f));
            M = glm::scale(M, pedestalScale);
            drawTexturedCube(shader, M, diff, spec, rough, norm, glm::vec2(1.0f), 0, 1.0f, 0);
        }

        // Statue
        gps::Model3D* statue = nullptr;
        float statueScale = 0.25f;
        float statueLift = 0.02f;

        if (i == 0) { statue = &statueAntonius; statueScale = 0.20f; }
        if (i == 1) { statue = &statueJudas;    statueScale = 0.22f; }
        if (i == 2) { statue = &statueKrieger;  statueScale = 0.09f; }

        if (statue) {
            glm::mat4 S(1.0f);
            S = glm::translate(S, objPos[i] + glm::vec3(0.0f, pedestalTopY + statueLift + bob, 0.0f));
            S = glm::rotate(S, glm::radians(spin), glm::vec3(0, 1, 0));
            S = glm::scale(S, glm::vec3(statueScale));
            drawModel(shader, *statue, S);
        }
    }
}

void renderDecor(gps::Shader& shader) {
    float W = 12.0f;
    float D = 16.0f;
    const float wallOffset = 0.3f;

    glDisable(GL_CULL_FACE);

    // Egyptian door
    {
        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(W * 0.5f - wallOffset, 1.5f, 2.0f));
        M = glm::rotate(M, glm::radians(-90.0f), glm::vec3(0, 1, 0));
        M = glm::scale(M, glm::vec3(4.0f));
        drawModel(shader, egyptDoor, M);
    }

    // Museum entrance
    {
        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(-2.0f, 0.8f, D * 0.5f - 1.5));
        M = glm::rotate(M, glm::radians(-90.0f), glm::vec3(1, 0, 0));
        M = glm::rotate(M, glm::radians(180.0f), glm::vec3(0, 0, 1));
        M = glm::scale(M, glm::vec3(0.1f));
        drawModel(shader, museumEntrance, M);
    }

    // Horror painting
    {
        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(-W * 0.5f + wallOffset, 1.3f, 0.0f));
        M = glm::rotate(M, glm::radians(90.0f), glm::vec3(0, 1, 0));
        M = glm::scale(M, glm::vec3(0.2f));
        drawModel(shader, horrorPainting, M);
    }
}

void renderDust(gps::Shader& shader) {
    shader.useShaderProgram();

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    for (auto& m : motes) {
        m.position.y -= m.speed;
        m.position.x += sin(glfwGetTime() + m.phase) * 0.001f;

        if (m.position.y < 0.0f)
            m.position.y = 4.0f;

        glm::mat4 M = glm::translate(glm::mat4(1.0f), m.position);
        M = glm::scale(M, glm::vec3(0.008f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(M));
        glUniform1i(glGetUniformLocation(shader.shaderProgram, "isOutside"), 1);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// SHADOW RENDERING FUNCTIONS

void renderRoomShadow(gps::Shader& sh) {
    float W = 12.0f, D = 16.0f;

    glm::mat4 M(1.0f);
    M = glm::scale(M, glm::vec3(W, 1.0f, D));
    drawShadowQuad(sh, M);
}

void renderObjectsShadow(gps::Shader& sh) {
    float t = (float)glfwGetTime();

    glm::vec3 objPos[3] = {
        glm::vec3(-3.0f, 0.0f, -2.0f),
        glm::vec3(2.0f, 0.0f, -2.0f),
        glm::vec3(4.0f, 0.0f, -2.0f)
    };

    glm::vec3 pedestalScale(0.9f, 1.2f, 0.9f);
    float pedestalCenterY = 0.6f;
    float pedestalTopY = pedestalCenterY + pedestalScale.y * 0.5f;

    for (int i = 0; i < 3; i++) {
        float bob = 0.05f * sin(t * 2.0f + (float)i);

        // Pedestal
        {
            glm::mat4 M(1.0f);
            M = glm::translate(M, objPos[i] + glm::vec3(0.0f, pedestalCenterY, 0.0f));
            M = glm::scale(M, pedestalScale);
            drawShadowCube(sh, M);
        }

        // Statues
        gps::Model3D* statue = nullptr;
        float s = 0.2f;
        if (i == 0) { statue = &statueAntonius; s = 0.20f; }
        if (i == 1) { statue = &statueJudas;    s = 0.22f; }
        if (i == 2) { statue = &statueKrieger;  s = 0.18f; }

        if (statue) {
            glm::mat4 S(1.0f);
            S = glm::translate(S, objPos[i] + glm::vec3(0.0f, pedestalTopY + 0.02f + bob, 0.0f));
            S = glm::scale(S, glm::vec3(s));
            drawShadowModel(sh, *statue, S);
        }
    }
}

void renderDecorShadow(gps::Shader& sh) {
    float W = 12.0f;
    float D = 16.0f;
    const float wallOffset = 0.3f;

    // Egyptian door
    {
        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(W * 0.5f - wallOffset, 0.8f, 2.0f));
        M = glm::rotate(M, glm::radians(-90.0f), glm::vec3(0, 1, 0));
        M = glm::scale(M, glm::vec3(1.5f));
        drawShadowModel(sh, egyptDoor, M);
    }

    // Museum entrance
    {
        glm::mat4 M(1.0f);
        M = glm::translate(M, glm::vec3(-3.0f, 0.0f, D * 0.5f - wallOffset));
        M = glm::rotate(M, glm::radians(180.0f), glm::vec3(0, 1, 0));
        M = glm::scale(M, glm::vec3(0.6f));
        drawShadowModel(sh, museumEntrance, M);
    }
}

// RENDER SCENE

void renderScene() {
    // Shadow pass
    lightSpaceMatrix = computeLightSpaceMatrix();

    glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    shadowShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(shadowShader.shaderProgram, "lightSpaceMatrix"),
        1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    if (personAnimate)
        personAnimT += 0.02f;

    float walk = personAnimate ? 0.25f * sin(personAnimT) : 0.0f;
    glm::mat4 mPerson(1.0f);
    mPerson = glm::translate(mPerson, personPos + glm::vec3(0.0f, 0.0f, walk));
    mPerson = glm::rotate(mPerson, glm::radians(personYaw), glm::vec3(0, 1, 0));
    mPerson = glm::scale(mPerson, glm::vec3(0.01f));

    renderRoomShadow(shadowShader);
    renderObjectsShadow(shadowShader);
    renderDecorShadow(shadowShader);

    for (int i = 0; i < 3; i++) {
        glm::mat4 M(1.0f);
        M = glm::translate(M, spots[i].position + glm::vec3(0.0f, 4.0f - spots[i].position.y - 0.1f, 0.0f));
        M = glm::scale(M, glm::vec3(0.4f, 0.05f, 0.4f));
        drawShadowCube(shadowShader, M);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Window shadow pass
    windowLightSpaceMatrix = computeWindowLightSpaceMatrix();

    glViewport(0, 0, WINDOW_SHADOW_SIZE, WINDOW_SHADOW_SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, windowShadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    shadowShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(shadowShader.shaderProgram, "lightSpaceMatrix"),
        1, GL_FALSE, glm::value_ptr(windowLightSpaceMatrix));

    renderRoomShadow(shadowShader);
    renderObjectsShadow(shadowShader);
    renderDecorShadow(shadowShader);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Normal rendering pass
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    myBasicShader.useShaderProgram();
    uploadSpotlights(myBasicShader);

    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceMatrix"),
        1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity"), fogDensity);
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "fogColor"),
        1, glm::value_ptr(fogColor));

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, shadowDepthTex);
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 5);

    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "windowLightSpaceMatrix"),
        1, GL_FALSE, glm::value_ptr(windowLightSpaceMatrix));

    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "windowLightDir"),
        1, glm::value_ptr(windowLightDir));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "windowLightColor"),
        1, glm::value_ptr(windowLightColor));

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, windowShadowDepthTex);
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "windowShadowMap"), 6);

    clampPersonInsideRoom();
    drawModel(myBasicShader, person, mPerson);
    renderRoom(myBasicShader);
    renderObjects(myBasicShader);
    renderDecor(myBasicShader);
    renderLampFixtures(myBasicShader);
    renderDust(myBasicShader);
}

void cleanup() {
    myWindow.Delete();
}


int main(int argc, const char* argv[]) {
    try {
        initOpenGLWindow();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
    initDust();
    initModels();
    initQuad();
    initCube();
    initShadowMap();
    initWindowShadowMap();

    // Load textures
    gps::Texture floorDiff = teapot.LoadTexture("models/teapot/marble_01_diff_4k.jpg", "diffuseTexture");
    gps::Texture floorSpec = teapot.LoadTexture("models/teapot/marble_01_disp_4k.png", "specularTexture");
    gps::Texture floorRough = teapot.LoadTexture("models/teapot/marble_01_rough_4k.jpg", "roughnessTexture");

    floorDiffuse = floorDiff.id;
    floorSpecular = floorSpec.id;
    floorRoughness = floorRough.id;

    gps::Texture wallDiff = teapot.LoadTexture("models/teapot/white_plaster_02_diff_4k.jpg", "diffuseTexture");
    gps::Texture wallSpec = wallDiff;
    gps::Texture wallRough = teapot.LoadTexture("models/teapot/white_plaster_02_rough_4k.jpg", "roughnessTexture");

    wallDiffuse = wallDiff.id;
    wallSpecular = wallSpec.id;
    wallRoughness = wallRough.id;

    gps::Texture floorNor = teapot.LoadTexture("models/teapot/marble_01_nor_gl_4k.png", "normalTexture");
    floorNormal = floorNor.id;

    gps::Texture wallNor = teapot.LoadTexture("models/teapot/white_plaster_02_nor_gl_4k.png", "normalTexture");
    wallNormal = wallNor.id;

    gps::Texture outside = teapot.LoadTexture("models/teapot/blue-sky-with-windy-clouds-vertical-shot.jpg", "diffuseTexture");
    outsideTex = outside.id;

    glBindTexture(GL_TEXTURE_2D, outsideTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    gps::Texture glassBase = teapot.LoadTexture("models/teapot/Window_001_basecolor.jpg", "diffuseTexture");
    gps::Texture glassRgh = teapot.LoadTexture("models/teapot/Window_001_roughness.jpg", "roughnessTexture");
    gps::Texture glassMet = teapot.LoadTexture("models/teapot/Window_001_metallic.jpg", "specularTexture");
    gps::Texture glassOp = teapot.LoadTexture("models/teapot/Window_001_opacity.jpg", "diffuseTexture");
    gps::Texture glassNor = teapot.LoadTexture("models/teapot/Window_001_normal.jpg", "normalTexture");

    glassDiffuse = glassBase.id;
    glassRough = glassRgh.id;
    glassSpec = glassMet.id;
    glassOpacity = glassOp.id;
    glassNormal = glassNor.id;

    setAnisotropy(floorDiffuse);
    setAnisotropy(wallDiffuse);
    setAnisotropy(glassDiffuse);

    auto clampTex = [](GLuint id) {
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        };

    clampTex(glassDiffuse);
    clampTex(glassOpacity);
    clampTex(glassNormal);

    glBindTexture(GL_TEXTURE_2D, glassOpacity);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    initShaders();
    initUniforms();
    setWindowCallbacks();
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glCheckError();

    // Application loop
    while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
        renderScene();

        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());

        glCheckError();
    }

    cleanup();

    return EXIT_SUCCESS;
}
