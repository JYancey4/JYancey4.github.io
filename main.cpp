#include <iostream>
#include <cstdlib>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include "camera.h"

// Global camera variables
float gDeltaTime = 0.0f; // Time between current frame and last frame
float gLastFrame = 0.0f; // Time of the last frame
float gCameraSpeedScale = 1.0f;
bool isPerspective = true; // Toggles between perspective and orthographic
Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f)); // Initialize the camera

using namespace std;

#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

void ResizeWindow(GLFWwindow* window, int width, int height);

// Constants for window dimensions and title
namespace {
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;
    const char* WINDOW_TITLE = "Textured 3D Coffee Mug";
    // Constants for cylinder
    const float BASE_RADIUS = 0.5f;
    const float TOP_RADIUS = 0.5f;
    const float HEIGHT = 1.0f;
    const int RADIAL_SEGMENTS = 36;
    const int HEIGHT_SEGMENTS = 1;
    // Constants for the torus dimensions
    const float INNER_RADIUS = 0.1f;
    const float OUTER_RADIUS = 0.2f;
    const int TUBULAR_SEGMENTS = 100;

    // Structure to hold mesh data
    struct GLMesh {
        GLuint vao;
        GLuint vbos[2];
        GLuint nIndices;
        GLMesh() : vao(0), nIndices(0) {
            vbos[0] = 0;
            vbos[1] = 0;
        }
    };

    GLFWwindow* gWindow = nullptr;
    GLMesh gMesh;
    GLuint gTextureIdHappy;
    GLuint gTextureIdHat;
    bool gIsHatOn = true;
    GLuint shaderProgram;
}

// Vertex Shader Source Code
const GLchar* vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 position;
    layout (location = 1) in vec2 texCoords;
    layout (location = 2) in vec3 normal; // Normal vector input

    out vec2 TexCoords;
    out vec3 Normal; // Passed to fragment shader
    out vec3 FragPosition; // Passed to fragment shader

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        FragPosition = vec3(model * vec4(position, 1.0));
        Normal = mat3(transpose(inverse(model))) * normal;
        gl_Position = projection * view * model * vec4(position, 1.0);
        TexCoords = texCoords;
}

)glsl";


// Fragment Shader Source Code
const GLchar* fragmentShaderSource = R"glsl(
    #version 330 core
    in vec2 TexCoords;
    in vec3 Normal;
    in vec3 FragPosition;

    out vec4 FragColor;

    struct Light {
        vec3 position;
        vec3 color;
        float intensity;
    };

    uniform Light keyLight;
    uniform Light fillLight;
    uniform vec3 viewPosition;
    uniform sampler2D texture1;

    vec3 calculateLight(Light light, vec3 normal, vec3 viewDir, vec3 lightDir) {
        vec3 ambient = light.color * (0.3 * light.intensity);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * light.color * light.intensity;
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = spec * light.color * light.intensity;
        return ambient + diffuse + specular;
    }

    void main() {
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(viewPosition - FragPosition);
        vec3 keyLightDir = normalize(keyLight.position - FragPosition);
        vec3 fillLightDir = normalize(fillLight.position - FragPosition);

        vec3 keyLightEffect = calculateLight(keyLight, norm, viewDir, keyLightDir);
        vec3 fillLightEffect = calculateLight(fillLight, norm, viewDir, fillLightDir);
        vec3 totalLight = keyLightEffect + fillLightEffect;

        vec4 textureColor = texture(texture1, TexCoords);
        FragColor = vec4(totalLight, 1.0) * textureColor;
}

)glsl";


struct Light {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
};

Light keyLight = { glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.1f, 1.0f, 0.1f), 1.0f };
Light fillLight = { glm::vec3(-1.0f, 0.5f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), 0.1f };

// Function to initialize GLFW and GLAD and create a window
bool Initialize(int argc, char* argv[], GLFWwindow** window)
{
    // Ensure GLFW library is initialized
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create GLFW window
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    // Set the current context
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, ResizeWindow);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();
    if (GLEW_OK != GlewInitResult) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(GlewInitResult) << std::endl;
        glfwDestroyWindow(*window); // Destroy the created window
        glfwTerminate();            // Terminate GLFW
        return false;
    }

    std::cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    return true;
}


// Function to flip the image vertically
void flipImageVertically(unsigned char* image, int width, int height, int channels) {
    for (int j = 0; j < height / 2; ++j) {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i) {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

// Function prototypes
bool Initialize(int argc, char* argv[], GLFWwindow** window);
void ResizeWindow(GLFWwindow* window, int width, int height);
void ProcessInput(GLFWwindow* window);
void CreateMesh(GLMesh& mesh);
void DestroyMesh(GLMesh& mesh);
bool CreateTexture(const char* filename, GLuint& textureId);
void DestroyTexture(GLuint textureId);
void Render(GLMesh& mesh, GLuint& shaderProgram);
bool CreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void DestroyShaderProgram(GLuint programId);
void MousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
GLMesh CreatePlaneMesh(float planeSize = 5.0f, float planeY = -1.0f);
GLMesh CreateCylinderMesh(float baseRadius, float topRadius, float height, int radialSegments, int heightSegments);
GLMesh CreateTorusMesh(float innerRadius, float outerRadius, int radialSegments, int tubularSegments, const glm::vec4& color);
GLMesh CreatePyramidMesh(float baseSize = 1.0f, float height = 1.0f);

// Function to create a shader program
bool CreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId) {
    // Create and compile the vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vtxShaderSource, NULL);
    glCompileShader(vertexShader);
    // Check for shader compile errors

    // Create and compile the fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragShaderSource, NULL);
    glCompileShader(fragmentShader);
    // Check for shader compile errors

    // Link shaders
    programId = glCreateProgram();
    glAttachShader(programId, vertexShader);
    glAttachShader(programId, fragmentShader);
    glLinkProgram(programId);
    // Check for linking errors

    // Delete shaders as they're linked into the program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true; // Return true if everything is successful
}

int main(int argc, char* argv[]) {
    if (!Initialize(argc, argv, &gWindow)) {
        glfwTerminate(); // Ensure glfwTerminate is called on failure
        return EXIT_FAILURE;
    }

    const float pyramidHeight = 1.0f;
    const float planeY = -0.27f;
    const float pyramidBaseSize = 1.0f;
    const float additionalOffset = 1.5f;
    const float offsetFromMug = BASE_RADIUS + (pyramidBaseSize / 2.0f) + additionalOffset;
    const float pyramidYOffset = 0.1f * pyramidBaseSize + fabs(planeY);
    const float pyramidRotationAngle = 20.0f;
    const glm::vec3 pyramidRotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);

    // Camera Callbacks
    glfwSetCursorPosCallback(gWindow, MousePositionCallback);
    glfwSetScrollCallback(gWindow, MouseScrollCallback);
    glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Disable the cursor

    // Texture IDs
    GLuint cylinderTexture, torusTexture, planeTexture;

    // Create textures
    if (!CreateTexture("cat.jpg", cylinderTexture)) {
        std::cerr << "Failed to load cylinder texture" << std::endl;
        glfwTerminate();
        return -1;
    }

    if (!CreateTexture("handle.jpg", torusTexture)) {
        std::cerr << "Failed to load torus texture" << std::endl;
        glfwTerminate();
        return -1;
    }

    if (!CreateTexture("wood_image.jpg", planeTexture)) {
        std::cerr << "Failed to load plane texture" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Create meshes
    GLMesh cylinderMesh = CreateCylinderMesh(BASE_RADIUS, TOP_RADIUS, HEIGHT, RADIAL_SEGMENTS, HEIGHT_SEGMENTS);
    GLMesh torusMesh = CreateTorusMesh(INNER_RADIUS, OUTER_RADIUS, RADIAL_SEGMENTS, TUBULAR_SEGMENTS, glm::vec4(0.5f, 0.3f, 0.0f, 1.0f));
    GLMesh planeMesh = CreatePlaneMesh(5.0f, planeY);
    GLMesh pyramidMesh = CreatePyramidMesh();

    // Initialize shader program
    if (!CreateShaderProgram(vertexShaderSource, fragmentShaderSource, shaderProgram)) {
        std::cerr << "Failed to create shader program" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Set up projection matrix
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    // Get uniform locations
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint keyLightPosLoc = glGetUniformLocation(shaderProgram, "keyLight.position");
    GLuint keyLightColorLoc = glGetUniformLocation(shaderProgram, "keyLight.color");
    GLuint keyLightIntensityLoc = glGetUniformLocation(shaderProgram, "keyLight.intensity");
    GLuint fillLightPosLoc = glGetUniformLocation(shaderProgram, "fillLight.position");
    GLuint fillLightColorLoc = glGetUniformLocation(shaderProgram, "fillLight.color");
    GLuint fillLightIntensityLoc = glGetUniformLocation(shaderProgram, "fillLight.intensity");
    GLuint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPosition");

    // Set light properties
    keyLight.position = glm::vec3(1.0f, 1.0f, 1.0f);
    keyLight.color = glm::vec3(1.0f, 1.0f, 1.0f);
    keyLight.intensity = 2.0f;

    fillLight.position = glm::vec3(-1.0f, 0.5f, 1.0f);
    fillLight.color = glm::vec3(1.0f, 1.0f, 1.0f);
    fillLight.intensity = 1.0f;

    // Camera/view position
    glm::vec3 viewPosition = glm::vec3(0.0f, 1.0f, 5.0f);

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(gWindow)) {

        float currentFrame = (glfwGetTime());
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        ProcessInput(gWindow);

        // Clear the screen at the beginning of each frame
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use the shader program
        glUseProgram(shaderProgram);

        // Camera
        glm::mat4 view = gCamera.GetViewMatrix();
        glm::mat4 projection;
        if (isPerspective) {
            projection = glm::perspective(glm::radians(gCamera.Zoom),
                (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
        }
        else {
            float orthoSize = 10.0f;
            projection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 100.0f);
        }

        // Set light uniforms
        glUniform3fv(keyLightPosLoc, 1, glm::value_ptr(keyLight.position));
        glUniform3fv(keyLightColorLoc, 1, glm::value_ptr(keyLight.color));
        glUniform1f(keyLightIntensityLoc, keyLight.intensity);

        glUniform3fv(fillLightPosLoc, 1, glm::value_ptr(fillLight.position));
        glUniform3fv(fillLightColorLoc, 1, glm::value_ptr(fillLight.color));
        glUniform1f(fillLightIntensityLoc, fillLight.intensity);

        // Set view position (camera position)
        glm::vec3 viewPosition = glm::vec3(0.0f, 1.0f, 3.0f); // the camera position
        GLuint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPosition");
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(viewPosition));

        // Set the projection and view uniforms in the shader
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Transform and render the pyramid
        glm::mat4 pyramidModel = glm::mat4(1.0f);
        // Adjust the position and scale of the pyramid as needed
        pyramidModel = glm::translate(pyramidModel, glm::vec3(offsetFromMug, planeY, 0.0f));
        pyramidModel = glm::rotate(pyramidModel, glm::radians(pyramidRotationAngle), pyramidRotationAxis);
        // Scale the pyramid
        pyramidModel = glm::scale(pyramidModel, glm::vec3(pyramidBaseSize, pyramidHeight, pyramidBaseSize));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(pyramidModel));
        // No texture for the pyramid, remove if you use textures
        glBindVertexArray(pyramidMesh.vao);
        glDrawElements(GL_TRIANGLES, pyramidMesh.nIndices, GL_UNSIGNED_SHORT, 0);

        // Transformation for the plane
        glm::mat4 planeModel = glm::mat4(1.0f);
        planeModel = glm::rotate(planeModel, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate to make it horizontal
        planeModel = glm::translate(planeModel, glm::vec3(0.0f, planeY, 0.0f)); // Move it below the mug
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(planeModel));
        glBindTexture(GL_TEXTURE_2D, planeTexture);
        glBindVertexArray(planeMesh.vao);
        glDrawElements(GL_TRIANGLES, planeMesh.nIndices, GL_UNSIGNED_SHORT, 0);

        // Render the cylinder (body of the mug)
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f * HEIGHT, 0.0f)); // Center and lower the mug
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, cylinderTexture);
        glBindVertexArray(cylinderMesh.vao);
        glDrawElements(GL_TRIANGLES, cylinderMesh.nIndices, GL_UNSIGNED_SHORT, 0);

        // Render the torus (handle of the mug)
        float torusOffset = BASE_RADIUS + OUTER_RADIUS - INNER_RADIUS;
        model = glm::translate(glm::mat4(1.0f), glm::vec3(torusOffset, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate to proper orientation
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, torusTexture);
        glBindVertexArray(torusMesh.vao);
        glDrawElements(GL_TRIANGLES, torusMesh.nIndices, GL_UNSIGNED_SHORT, 0);

        // Swap buffers at the end of the frame
        glfwSwapBuffers(gWindow);
        glfwPollEvents();
    }

    // Cleanup
    DestroyTexture(cylinderTexture); // Destroy cylinder texturev
    DestroyTexture(torusTexture);    // Destroy torus texture
    DestroyShaderProgram(shaderProgram); // Destroy the shader program
    glfwTerminate(); // Cleanup and terminate GLFW

    return 0;
}

void DestroyTexture(GLuint textureId) {
    glDeleteTextures(1, &textureId);
}

// Function to process input
void ProcessInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        isPerspective = !isPerspective; // Toggle between perspective and orthographic
    }

    // Toggle between perspective and orthographic view when the 'P' key is pressed
    static bool p_key_pressed = false;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        if (!p_key_pressed) {
            isPerspective = !isPerspective;
            p_key_pressed = true;
        }
    }
    else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
        p_key_pressed = false;
    }

    float cameraSpeed = gCamera.MovementSpeed * gDeltaTime * gCameraSpeedScale;
    // Existing movement controls
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, cameraSpeed);

    // Upward and downward movement
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
}

// Function to resize the GLFW window
void ResizeWindow(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Function to create the mesh for the coffee mug
void CreateMesh(GLMesh& mesh) {
    // Vertex Data with Positions and Texture Coordinates
    GLfloat verts[] = {
        // Positions        // Texture Coordinates
        // Base
        -0.5f, 0.0f, -0.5f,  0.0f, 0.0f,
         0.5f, 0.0f, -0.5f,  1.0f, 0.0f,
         0.5f, 0.0f,  0.5f,  1.0f, 1.0f,
        -0.5f, 0.0f,  0.5f,  0.0f, 1.0f,
        // Apex
         0.0f, 1.0f,  0.0f,  0.5f, 0.5f,
    };

    GLushort indices[] = {
        // Base
        0, 1, 2,
        0, 2, 3,
        // Sides
        0, 1, 4,
        1, 2, 4,
        2, 3, 4,
        3, 0, 4
    };

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(2, mesh.vbos);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Function to destroy the mesh
void DestroyMesh(GLMesh& mesh) {
    // Cleanup VBOs and VAO
    glDeleteBuffers(2, mesh.vbos);
    glDeleteVertexArrays(1, &mesh.vao);
}

// Function to create a texture from an image file
bool CreateTexture(const char* filename, GLuint& textureId) {
    // Load the texture from file
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image) {
        flipImageVertically(image, width, height, channels); // Flip the image vertically

        // Generate a texture ID and bind to it
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Load the texture into OpenGL
        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

        // Generate mipmaps
        glGenerateMipmap(GL_TEXTURE_2D);

        // Free the image and unbind the texture
        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0);

        return true;
    }
    else {
        std::cerr << "Texture failed to load at path: " << filename << std::endl;
        stbi_image_free(image);
        return false;
    }
}

// Function to render the scene
void Render(GLMesh& mesh, GLuint& shaderProgram) {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use the shader program
    glUseProgram(shaderProgram);

    // Bind VAO and draw the mesh
    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.nIndices, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
    glfwSwapBuffers(gWindow); // Swap buffers after drawing
}

void MousePositionCallback(GLFWwindow* window, double xpos, double ypos) {
    static float lastX = WINDOW_WIDTH / 2.0f;
    static float lastY = WINDOW_HEIGHT / 2.0f;
    static bool firstMouse = true;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}

void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // Adjust the zoom of the camera
    gCamera.ProcessMouseScroll(yoffset);

    // Adjust the speed scale of the camera based on scroll
    const float speedChange = 0.1f; // Change this value to adjust the sensitivity of speed
    gCameraSpeedScale += yoffset * speedChange;
    gCameraSpeedScale = glm::max(gCameraSpeedScale, 0.1f); // Prevent the speed scale from going below a threshold
}


void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
        if (action == GLFW_PRESS)
            std::cout << "Left mouse button pressed" << std::endl;
        else
            std::cout << "Left mouse button released" << std::endl;
        break;
    case GLFW_MOUSE_BUTTON_MIDDLE:
        if (action == GLFW_PRESS)
            std::cout << "Middle mouse button pressed" << std::endl;
        else
            std::cout << "Middle mouse button released" << std::endl;
        break;
    case GLFW_MOUSE_BUTTON_RIGHT:
        if (action == GLFW_PRESS)
            std::cout << "Right mouse button pressed" << std::endl;
        else
            std::cout << "Right mouse button released" << std::endl;
        break;
    default:
        std::cout << "Unhandled mouse button event" << std::endl;
        break;
    }
}

// Function to create a pyramid mesh
GLMesh CreatePyramidMesh(float baseSize, float height) {
    GLMesh mesh;
    std::vector<GLfloat> vertices;
    std::vector<GLushort> indices;

    // Pyramid base is a square
    float halfBase = baseSize / 2.0f;

    // Push vertices for the pyramid
    auto pushVertex = [&](float x, float y, float z, float s, float t) {
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
        vertices.push_back(s);
        vertices.push_back(t);
    };

    // Base
    pushVertex(-halfBase, 0.0f, -halfBase, 0.0f, 0.0f);
    pushVertex(halfBase, 0.0f, -halfBase, 1.0f, 0.0f);
    pushVertex(halfBase, 0.0f, halfBase, 1.0f, 1.0f);
    pushVertex(-halfBase, 0.0f, halfBase, 0.0f, 1.0f);
    // Apex
    pushVertex(0.0f, height, 0.0f, 0.5f, 0.5f);

    // Push indices for the pyramid
    auto pushIndex = [&](GLushort a, GLushort b, GLushort c) {
        indices.push_back(a);
        indices.push_back(b);
        indices.push_back(c);
    };

    // Base
    pushIndex(0, 1, 2);
    pushIndex(0, 2, 3);
    // Sides
    pushIndex(0, 1, 4);
    pushIndex(1, 2, 4);
    pushIndex(2, 3, 4);
    pushIndex(3, 0, 4);

    mesh.nIndices = indices.size();

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(2, mesh.vbos);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return mesh;
}


// Function to create a plane mesh
GLMesh CreatePlaneMesh(float planeSize, float planeY) {
    GLMesh mesh;

    GLfloat verts[] = {
        // Vertex Positions      // Texture Coordinates
        -planeSize, planeY, -planeSize,  0.0f, 0.0f,
         planeSize, planeY, -planeSize,  1.0f, 0.0f,
         planeSize, planeY,  planeSize,  1.0f, 1.0f,
        -planeSize, planeY,  planeSize,  0.0f, 1.0f,
    };

    GLushort indices[] = {
        0, 1, 2, // First Triangle
        2, 3, 0  // Second Triangle
    };

    mesh.nIndices = sizeof(indices) / sizeof(GLushort);

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(2, mesh.vbos);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return mesh;
}

// Function to create a cylinder mesh
GLMesh CreateCylinderMesh(float baseRadius, float topRadius, float height, int radialSegments, int heightSegments) {
    std::vector<float> vertices;
    std::vector<GLushort> indices;

    auto pushVertex = [&](float x, float y, float z, float s, float t) {
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
        vertices.push_back(s);
        vertices.push_back(t);
    };

    for (int y = 0; y <= heightSegments; y++) {
        float currentHeight = height * static_cast<float>(y) / heightSegments;
        float currentRadius = baseRadius + static_cast<float>(y) / heightSegments * (topRadius - baseRadius);

        for (int x = 0; x <= radialSegments; x++) {
            float theta = 2.0f * glm::pi<float>() * static_cast<float>(x) / radialSegments;
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            float vx = currentRadius * cosTheta;
            float vy = currentHeight;
            float vz = currentRadius * sinTheta;

            float s = static_cast<float>(x) / radialSegments;
            float t = static_cast<float>(y) / heightSegments;

            pushVertex(vx, vy, vz, s, t);
        }
    }

    for (int y = 0; y < heightSegments; y++) {
        for (int x = 0; x < radialSegments; x++) {
            int base = y * (radialSegments + 1) + x;
            int next = base + radialSegments + 1;

            indices.push_back(base);
            indices.push_back(base + 1);
            indices.push_back(next);

            indices.push_back(next);
            indices.push_back(base + 1);
            indices.push_back(next + 1);
        }
    }

    GLMesh mesh;
    mesh.nIndices = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    glGenBuffers(1, &mesh.vbos[0]);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.vbos[1]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return mesh;
}


// Function to create a torus mesh
GLMesh CreateTorusMesh(float innerRadius, float outerRadius, int radialSegments, int tubularSegments, const glm::vec4& color) {
    std::vector<float> vertices;
    std::vector<GLushort> indices;

    auto pushVertex = [&](float x, float y, float z, float s, float t) {
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
        vertices.push_back(s);
        vertices.push_back(t);
    };

    for (int i = 0; i <= tubularSegments; i++) {
        float u = static_cast<float>(i) / tubularSegments * 2.0f * glm::pi<float>();

        glm::vec3 circleCenter = glm::vec3(cos(u) * innerRadius, sin(u) * innerRadius, 0.0f);

        for (int j = 0; j <= radialSegments; j++) {
            float v = static_cast<float>(j) / radialSegments * 2.0f * glm::pi<float>();
            glm::vec3 vertexPosition = circleCenter + outerRadius * glm::vec3(cos(v), 0.0f, sin(v));
            glm::vec3 vertexNormal = glm::normalize(vertexPosition - circleCenter);

            pushVertex(vertexPosition.x, vertexPosition.y, vertexPosition.z, static_cast<float>(i) / tubularSegments, static_cast<float>(j) / radialSegments);
        }
    }

    for (int i = 0; i < tubularSegments; i++) {
        for (int j = 0; j < radialSegments; j++) {
            int first = (i * (radialSegments + 1)) + j;
            int second = first + radialSegments + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    GLMesh mesh;
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    GLsizei stride = 5 * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    mesh.nIndices = static_cast<GLsizei>(indices.size());

    return mesh;
}

// Function to destroy a shader program
void DestroyShaderProgram(GLuint programId) {
    glDeleteProgram(programId);
}