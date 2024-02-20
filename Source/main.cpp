/*
 * 3D Coffee Mug Rendering Program
 * Author: Joshua Yancey
 * Contact: joshua.yancey@snhu.edu
 * Date: 02/01/2024
 * Version: 1.1
 *
 * Program Description:
 * This program creates a 3D-rendered coffee mug using OpenGL and GLFW. It demonstrates
 * the use of modern OpenGL shaders, texture mapping, lighting, and camera controls.
 * The program includes a vertex and fragment shader for rendering a textured mug with
 * dynamic lighting effects.
 *
 * Key Features:
 * - 3D model rendering with texture mapping
 * - Dynamic lighting and shading
 * - Camera movement and perspective control
 *
 * Revision History:
 * - Version 1.0 (10/08/2023): Initial creation
 * - Version 1.1 (02/15/2024): Performance Optimizations and Code Refactoring
 *     - Optimized glfwGetTime() calls to reduce system call overhead.
 *     - Implemented a flag to conditionally update uniform variables (view and projection matrices),
 *       reducing unnecessary GPU commands.
 *     - Enhanced projection matrix initialization to avoid redundant computations.
 *     - Additional code refactoring for improved readability and maintainability.
 *     - Added detailed comments for optimization techniques and their impact on performance.
 */

 // Include necessary libraries and headers
#include <iostream>
#include <cstdlib> // General Utilities Library
#include <GL/glew.h>     // OpenGL Extension Wrangler Library
#include <GLFW/glfw3.h>  // OpenGL Framework Library
#define STB_IMAGE_IMPLEMENTATION
#include <Headers/stb_image.h>   // Image loading library
#include <glm/glm.hpp>   // OpenGL Mathematics Library
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include "Headers/camera.h"      // Custom camera control header

// Global camera variables
float gDeltaTime = 0.0f; // Time between current frame and last frame
float gLastFrame = 0.0f; // Time of the last frame
float gCameraSpeedScale = 1.0f; // Scaling factor for camera speed
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
    #version 330 core // Specify OpenGL core profile version

    // Input vertex attributes
    layout (location = 0) in vec3 position; // Vertex position (x, y, z)
    layout (location = 1) in vec2 texCoords; // Texture coordinates (s, t)
    layout (location = 2) in vec3 normal; // Normal vector for lighting calculations

    // Outputs to the fragment shader
    out vec2 TexCoords; // Pass texture coordinates to fragment shader
    out vec3 Normal; // Pass normal vector to fragment shader
    out vec3 FragPosition; // Pass fragment position to fragment shader

    // Uniforms (shared variables)
    uniform mat4 model; // Model matrix for transforming object space to world space
    uniform mat4 view; // View matrix for transforming world space to camera space
    uniform mat4 projection; // Projection matrix for transforming camera space to clip space

    void main() {
        // Calculate fragment position in world space
        FragPosition = vec3(model * vec4(position, 1.0));
    
        // Transform normal vector by model matrix and pass to fragment shader
        Normal = mat3(transpose(inverse(model))) * normal;
    
        // Calculate final position of the vertex in clip space
        gl_Position = projection * view * model * vec4(position, 1.0);
    
        // Pass texture coordinates to fragment shader
        TexCoords = texCoords;
}

)glsl";


// Fragment Shader Source Code
const GLchar* fragmentShaderSource = R"glsl(
    #version 330 core // Specify OpenGL core profile version

    // Inputs from vertex shader
    in vec2 TexCoords; // Texture coordinates
    in vec3 Normal; // Normal vector
    in vec3 FragPosition; // Fragment position in world space

    // Output color of the fragment
    out vec4 FragColor; // Final fragment color

    // Structure to represent a light source
    struct Light {
        vec3 position; // Light position in world space
        vec3 color; // Light color
        float intensity; // Light intensity
};

    // Uniforms (shared variables)
    uniform Light keyLight; // Key light properties
    uniform Light fillLight; // Fill light properties
    uniform vec3 viewPosition; // Camera position
    uniform sampler2D texture1; // Texture sampler

    // Function to calculate lighting effect based on Phong reflection model
    vec3 calculateLight(Light light, vec3 normal, vec3 viewDir, vec3 lightDir) {
        // Ambient component
        vec3 ambient = light.color * (0.3 * light.intensity);

        // Diffuse component
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * light.color * light.intensity;

        // Specular component
        float specStrength = max(dot(viewDir, reflect(-lightDir, normal)), 0.0);
        vec3 specular = vec3(0.0);
        if (specStrength > 0.0) {
            specular = pow(specStrength, 32) * light.color * light.intensity;
    }

        // Combine components
        return ambient + diffuse + specular;
}

    void main() {
        // Normalize vectors for accurate calculations
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(viewPosition - FragPosition);
        vec3 keyLightDir = normalize(keyLight.position - FragPosition);
        vec3 fillLightDir = normalize(fillLight.position - FragPosition);

        // Calculate lighting effects from both lights
        vec3 keyLightEffect = calculateLight(keyLight, norm, viewDir, keyLightDir);
        vec3 fillLightEffect = calculateLight(fillLight, norm, viewDir, fillLightDir);

        // Combine lighting effects
        vec3 totalLight = keyLightEffect + fillLightEffect;

        // Sample texture color and apply lighting
        vec4 textureColor = texture(texture1, TexCoords);
        FragColor = vec4(totalLight, 1.0) * textureColor;
}
)glsl";


// Light structure definition
struct Light {
    glm::vec3 position; // Position of the light in 3D space
    glm::vec3 color;    // Color of the light, represented as RGB
    float intensity;    // Intensity of the light, affects overall brightness
};

// Initialize key light - primary light source in the scene
Light keyLight = {
    glm::vec3(1.0f, 1.0f, 1.0f), // Position of the key light
    glm::vec3(0.1f, 1.0f, 0.1f), // Color of the key light (greenish tint)
    1.0f                          // Intensity of the key light
};

// Initialize fill light - secondary light source to soften shadows
Light fillLight = {
    glm::vec3(-1.0f, 0.5f, 1.0f), // Position of the fill light
    glm::vec3(1.0f, 1.0f, 1.0f),  // Color of the fill light (white)
    0.1f                          // Intensity of the fill light (lower than key light)
};

// Function to initialize GLFW and GLAD and create a window
bool Initialize(int argc, char* argv[], GLFWwindow** window) {
    // Initialize GLFW and check for failure
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Configure GLFW with required OpenGL version and profile settings
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Ensure forward compatibility on macOS
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create a GLFW window and check for failure
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate(); // Terminate GLFW if window creation failed
        return false;
    }

    // Set the created window as the current context
    glfwMakeContextCurrent(*window);

    // Set callback for window resizing
    glfwSetFramebufferSizeCallback(*window, ResizeWindow);

    // Initialize GLEW and check for failure
    glewExperimental = GL_TRUE; // Enable modern GLEW features
    GLenum GlewInitResult = glewInit();
    if (GLEW_OK != GlewInitResult) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(GlewInitResult) << std::endl;
        glfwDestroyWindow(*window); // Destroy the created window
        glfwTerminate();            // Terminate GLFW
        return false;
    }

    // Output the current OpenGL version being used
    std::cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    return true; // Return true if initialization is successful
}


// Function to flip the image vertically
void flipImageVertically(unsigned char* image, int width, int height, int channels) {
    for (int j = 0; j < height / 2; ++j) {
        int index1 = j * width * channels;       // Index for the top row
        int index2 = (height - 1 - j) * width * channels; // Index for the bottom row

        for (int i = width * channels; i > 0; --i) {
            // Swap pixels between the top and bottom rows
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

// Function prototypes provide a forward declaration of functions used in the program.
// This allows these functions to be called before their actual definitions.

// Initializes GLFW and GLAD, and creates a window
bool Initialize(int argc, char* argv[], GLFWwindow** window);

// Resizes the GLFW window upon changes in window dimensions
void ResizeWindow(GLFWwindow* window, int width, int height);

// Processes user input during each frame
void ProcessInput(GLFWwindow* window);

// Creates the mesh data for rendering
void CreateMesh(GLMesh& mesh);

// Destroys the mesh data, freeing resources
void DestroyMesh(GLMesh& mesh);

// Creates a texture from a given image file
bool CreateTexture(const char* filename, GLuint& textureId);

// Destroys a given OpenGL texture
void DestroyTexture(GLuint textureId);

// Renders a given mesh using the specified shader program
void Render(GLMesh& mesh, GLuint& shaderProgram);

// Creates a shader program from vertex and fragment shader sources
bool CreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);

// Destroys a given shader program
void DestroyShaderProgram(GLuint programId);

// Callback for mouse position changes
void MousePositionCallback(GLFWwindow* window, double xpos, double ypos);

// Callback for mouse scroll events
void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// Creates a plane mesh with specified dimensions
GLMesh CreatePlaneMesh(float planeSize = 5.0f, float planeY = -1.0f);

// Creates a cylinder mesh with specified dimensions
GLMesh CreateCylinderMesh(float baseRadius, float topRadius, float height, int radialSegments, int heightSegments);

// Creates a torus (doughnut-shaped) mesh with specified dimensions
GLMesh CreateTorusMesh(float innerRadius, float outerRadius, int radialSegments, int tubularSegments, const glm::vec4& color);

// Creates a pyramid mesh with specified base size and height
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

// Main function of the application
int main(int argc, char* argv[]) {
    // Initialize GLFW, GLAD, and create a window
    // If initialization fails, terminate the program
    if (!Initialize(argc, argv, &gWindow)) {
        glfwTerminate(); // Terminate GLFW on failure
        return EXIT_FAILURE; // Exit with failure status
    }

    // Define parameters for the pyramid and plane in the scene
    const float pyramidHeight = 1.0f; // Height of the pyramid
    const float planeY = -0.27f; // Y-coordinate for the plane
    const float pyramidBaseSize = 1.0f; // Base size of the pyramid
    const float additionalOffset = 1.5f; // Additional offset from the mug for the pyramid
    const float offsetFromMug = BASE_RADIUS + (pyramidBaseSize / 2.0f) + additionalOffset; // Calculate offset from the mug
    const float pyramidRotationAngle = 20.0f; // Rotation angle for the pyramid
    const glm::vec3 pyramidRotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Rotation axis for the pyramid

    // Set camera control callbacks
    glfwSetCursorPosCallback(gWindow, MousePositionCallback); // Set callback for mouse movement
    glfwSetScrollCallback(gWindow, MouseScrollCallback); // Set callback for mouse scroll
    glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Disable the cursor for camera control

    // Texture IDs for various objects
    GLuint cylinderTexture, torusTexture, planeTexture;

    // Load textures and check for errors
    if (!CreateTexture("Textures/cat.jpg", cylinderTexture)) {
        std::cerr << "Failed to load cylinder texture" << std::endl;
        glfwTerminate();
        return -1; // Return error code if texture loading fails
    }

    if (!CreateTexture("Textures/handle.jpg", torusTexture)) {
        std::cerr << "Failed to load torus texture" << std::endl;
        glfwTerminate();
        return -1;
    }

    if (!CreateTexture("Textures/wood_image.jpg", planeTexture)) {
        std::cerr << "Failed to load plane texture" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Create mesh objects for the scene
    GLMesh cylinderMesh = CreateCylinderMesh(BASE_RADIUS, TOP_RADIUS, HEIGHT, RADIAL_SEGMENTS, HEIGHT_SEGMENTS); // Create cylinder mesh
    GLMesh torusMesh = CreateTorusMesh(INNER_RADIUS, OUTER_RADIUS, RADIAL_SEGMENTS, TUBULAR_SEGMENTS, glm::vec4(0.5f, 0.3f, 0.0f, 1.0f)); // Create torus mesh
    GLMesh planeMesh = CreatePlaneMesh(5.0f, planeY); // Create plane mesh
    GLMesh pyramidMesh = CreatePyramidMesh(); // Create pyramid mesh

    // Initialize shader program
    if (!CreateShaderProgram(vertexShaderSource, fragmentShaderSource, shaderProgram)) {
        std::cerr << "Failed to create shader program" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Setting up projection matrices for 3D rendering
    glm::mat4 projection; // Projection matrix for perspective view
    glm::mat4 orthoProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f); // Orthographic projection matrix

    // Retrieving uniform locations from the shader program
    // Uniforms are used to pass data from the application on the CPU to the shaders on the GPU
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model"); // Location of the model matrix uniform
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view"); // Location of the view matrix uniform
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection"); // Location of the projection matrix uniform
    // Uniform locations for light properties
    GLuint keyLightPosLoc = glGetUniformLocation(shaderProgram, "keyLight.position");
    GLuint keyLightColorLoc = glGetUniformLocation(shaderProgram, "keyLight.color");
    GLuint keyLightIntensityLoc = glGetUniformLocation(shaderProgram, "keyLight.intensity");
    GLuint fillLightPosLoc = glGetUniformLocation(shaderProgram, "fillLight.position");
    GLuint fillLightColorLoc = glGetUniformLocation(shaderProgram, "fillLight.color");
    GLuint fillLightIntensityLoc = glGetUniformLocation(shaderProgram, "fillLight.intensity");
    GLuint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPosition");

    // Setting light properties for the scene
    keyLight.position = glm::vec3(1.0f, 1.0f, 1.0f); // Position of the key light
    keyLight.color = glm::vec3(1.0f, 1.0f, 1.0f); // Color of the key light
    keyLight.intensity = 2.0f; // Intensity of the key light

    fillLight.position = glm::vec3(-1.0f, 0.5f, 1.0f); // Position of the fill light
    fillLight.color = glm::vec3(1.0f, 1.0f, 1.0f); // Color of the fill light
    fillLight.intensity = 1.0f; // Intensity of the fill light

    // Camera/view position in the scene
    glm::vec3 viewPosition = glm::vec3(0.0f, 1.0f, 5.0f); // Position of the camera

    // Enabling depth test for 3D rendering
    glEnable(GL_DEPTH_TEST);

    // Main rendering loop
    bool viewProjectionChanged = true; // Flag to track changes in view or projection

    while (!glfwWindowShouldClose(gWindow)) { // Loop until the window is instructed to close
        // Optimization: Limit the glfwGetTime() calls to once per loop iteration for time efficiency
        float currentFrame = glfwGetTime(); // Get current frame time
        gDeltaTime = currentFrame - gLastFrame; // Calculate time difference between frames
        gLastFrame = currentFrame; // Update last frame time

        // Process input efficiently by checking the state of the keys once per frame
        ProcessInput(gWindow);

        // Set background color and clear buffers
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Set clear color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear color and depth buffers

        // Use the shader program
        glUseProgram(shaderProgram);

        // Calculate the view matrix using the camera's parameters
        glm::mat4 view = gCamera.GetViewMatrix();
        if (isPerspective) {
            // Compute perspective projection matrix once unless the camera zoom changes
            // Time Complexity: O(1), as the computation is constant time with respect to the number of elements rendered
            projection = glm::perspective(glm::radians(gCamera.Zoom),
                (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
            viewProjectionChanged = true;
        }
        else if (viewProjectionChanged) {
            // Optimization:
            // Orthographic projection is set only once, avoiding redundant updates
            projection = orthoProjection;
            viewProjectionChanged = false; // Reset the flag
        }

        // Optimization:
        // Conditional uniform updates reduce GPU commands, enhancing rendering efficiency
        if (viewProjectionChanged) {
            glUniform3fv(viewPosLoc, 1, glm::value_ptr(gCamera.Position));
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        }

        glUniform3fv(viewPosLoc, 1, glm::value_ptr(gCamera.Position)); // Use camera position
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

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

    // Calculate the number of indices in the mesh
    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);

    // Generate a Vertex Array Object (VAO) for the mesh
    glGenVertexArrays(1, &mesh.vao);

    // Generate Vertex Buffer Objects (VBOs) for the mesh
    glGenBuffers(2, mesh.vbos);

    // Bind the VAO as the current array to be used
    glBindVertexArray(mesh.vao);

    // Bind the first VBO as an array buffer
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);

    // Load vertex data (position and texture coordinates) into the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    // Define the position attribute layout (attribute 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0); // Enable the position attribute

    // Define the texture coordinate attribute layout (attribute 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1); // Enable the texture coordinate attribute

    // Bind the second VBO as an element array buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);

    // Load index data into the element array buffer
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Unbind the VBO and VAO to prevent unintended modifications
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
    static float lastX = WINDOW_WIDTH / 2.0f; // Previous X position
    static float lastY = WINDOW_HEIGHT / 2.0f; // Previous Y position
    static bool firstMouse = true; // Flag to check if this is the first mouse event

    // Update last positions if this is the first mouse event
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    // Calculate the offset movement since the last frame
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates range from bottom to top
    lastX = xpos;
    lastY = ypos;

    // Process the mouse movement using the camera's method
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
    // Handle different mouse buttons
    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT: // Left mouse button
        if (action == GLFW_PRESS)
            std::cout << "Left mouse button pressed" << std::endl; // Log left button press
        else
            std::cout << "Left mouse button released" << std::endl; // Log left button release
        break;
    case GLFW_MOUSE_BUTTON_MIDDLE: // Middle mouse button
        if (action == GLFW_PRESS)
            std::cout << "Middle mouse button pressed" << std::endl; // Log middle button press
        else
            std::cout << "Middle mouse button released" << std::endl; // Log middle button release
        break;
    case GLFW_MOUSE_BUTTON_RIGHT: // Right mouse button
        if (action == GLFW_PRESS)
            std::cout << "Right mouse button pressed" << std::endl; // Log right button press
        else
            std::cout << "Right mouse button released" << std::endl; // Log right button release
        break;
    default:
        std::cout << "Unhandled mouse button event" << std::endl; // Log unhandled cases
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
    std::vector<float> vertices; // Vector to store vertex data
    std::vector<GLushort> indices; // Vector to store indices for drawing the cylinder

    // Lambda function to add a vertex to the vertices vector
    auto pushVertex = [&](float x, float y, float z, float s, float t) {
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
        vertices.push_back(s);
        vertices.push_back(t);
        };

    // Generate vertices for the cylinder
    for (int y = 0; y <= heightSegments; y++) {
        float currentHeight = height * static_cast<float>(y) / heightSegments;
        float currentRadius = baseRadius + static_cast<float>(y) / heightSegments * (topRadius - baseRadius);

        for (int x = 0; x <= radialSegments; x++) {
            float theta = 2.0f * glm::pi<float>() * static_cast<float>(x) / radialSegments;
            float vx = currentRadius * cos(theta);
            float vy = currentHeight;
            float vz = currentRadius * sin(theta);

            float s = static_cast<float>(x) / radialSegments;
            float t = static_cast<float>(y) / heightSegments;

            pushVertex(vx, vy, vz, s, t);
        }
    }

    // Generate indices for the cylinder
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

    // Create the mesh and set its properties
    GLMesh mesh;
    mesh.nIndices = static_cast<GLsizei>(indices.size());

    // Generate and bind the Vertex Array Object (VAO)
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    // Generate and bind Vertex Buffer Objects (VBOs)
    glGenBuffers(1, &mesh.vbos[0]);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.vbos[1]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);

    // Set vertex attribute pointers and enable them
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbind the buffers
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return mesh; // Return the created cylinder mesh
}


// Function to create a torus mesh
GLMesh CreateTorusMesh(float innerRadius, float outerRadius, int radialSegments, int tubularSegments, const glm::vec4& color) {
    std::vector<float> vertices; // Vector to store vertex data
    std::vector<GLushort> indices; // Vector to store indices for drawing the torus

    // Lambda function to add a vertex to the vertices vector
    auto pushVertex = [&](float x, float y, float z, float s, float t) {
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
        vertices.push_back(s);
        vertices.push_back(t);
        };

    // Generate vertices for the torus
    for (int i = 0; i <= tubularSegments; i++) {
        float u = static_cast<float>(i) / tubularSegments * 2.0f * glm::pi<float>();
        glm::vec3 circleCenter = glm::vec3(cos(u) * innerRadius, sin(u) * innerRadius, 0.0f);

        for (int j = 0; j <= radialSegments; j++) {
            float v = static_cast<float>(j) / radialSegments * 2.0f * glm::pi<float>();
            glm::vec3 vertexPosition = circleCenter + outerRadius * glm::vec3(cos(v), 0.0f, sin(v));

            pushVertex(vertexPosition.x, vertexPosition.y, vertexPosition.z, static_cast<float>(i) / tubularSegments, static_cast<float>(j) / radialSegments);
        }
    }

    // Generate indices for the torus
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

    // Create the mesh and set its properties
    GLMesh mesh;
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    // Generate and bind Vertex Buffer Objects (VBOs)
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Set vertex attribute pointers and enable them
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    // Bind the element buffer and load indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);

    // Unbind the VAO
    glBindVertexArray(0);
    mesh.nIndices = static_cast<GLsizei>(indices.size());

    return mesh; // Return the created torus mesh
}

// Function to destroy a shader program
void DestroyShaderProgram(GLuint programId) {
    glDeleteProgram(programId);
}