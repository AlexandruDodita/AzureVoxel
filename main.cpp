#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib> // For system
#include <iomanip> // For std::setprecision

#include "headers/window.h"
#include "headers/block.h"
#include "headers/texture.h"
#include "headers/camera.h"
#include "headers/world.h"
#include "headers/crosshair.h" // Include Crosshair header

// Window dimensions
const unsigned int SCREEN_WIDTH = 800;
const unsigned int SCREEN_HEIGHT = 600;

// Timing
float deltaTime = 0.0f;  // Time between current frame and last frame
float lastFrame = 0.0f;  // Time of last frame

// Performance statistics
int frameCount = 0;
float lastFpsTime = 0.0f;
float fps = 0.0f;

Crosshair* crosshair = nullptr; // Declare crosshair pointer

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Set OpenGL version to 3.3 COMPATIBILITY profile instead of core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE); // Use compatibility profile
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    // Create window
    GLFWwindow* windowHandle = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, 
                                              "AzureVoxel - Press X for wireframe mode", 
                                              NULL, NULL);
    if (!windowHandle) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    // Make the window's context current
    glfwMakeContextCurrent(windowHandle);
    
    // Create our window object
    Window window(windowHandle, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Enable mouse capture
    window.enableMouseCapture(true);

    // Clear any errors from GLEW initialization - this is normal
    while (glGetError() != GL_NO_ERROR);

    // Check for OpenGL context
    if (glfwGetCurrentContext() == nullptr) {
        std::cerr << "CRITICAL ERROR: No valid OpenGL context is current" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Print OpenGL context info
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* vendor = glGetString(GL_VENDOR);
    std::cout << "OpenGL Vendor: " << (vendor ? reinterpret_cast<const char*>(vendor) : "Unknown") << std::endl;
    std::cout << "OpenGL Renderer: " << (renderer ? reinterpret_cast<const char*>(renderer) : "Unknown") << std::endl;
    std::cout << "OpenGL Version: " << (version ? reinterpret_cast<const char*>(version) : "Unknown") << std::endl;
    
    // Test basic OpenGL functions
    GLuint testVAO = 0;
    glGenVertexArrays(1, &testVAO);
    if (testVAO == 0) {
        std::cerr << "CRITICAL ERROR: Failed to generate test VAO. OpenGL context may be invalid." << std::endl;
        GLenum error = glGetError();
        std::cerr << "OpenGL error: " << error << std::endl;
    } else {
        std::cout << "Successfully generated test VAO: " << testVAO << std::endl;
        glDeleteVertexArrays(1, &testVAO);
    }
    
    // Initialize the static block shader program once
    Block::InitBlockShader();
    Block::InitSpritesheet("res/textures/Spritesheet.PNG"); // Load the global spritesheet

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); // Accept fragment if it's closer to the camera than the former one
    
    // For better block visibility
    glEnable(GL_CULL_FACE); // Re-enable face culling
    glCullFace(GL_BACK);    // Cull back faces
    
    // Create a camera with elevated position for better view of the world
    Camera camera(glm::vec3(0.0f, 75.0f, 0.0f)); // Start high above for better view
    camera.setMovementSpeed(25.0f); // Faster movement
    camera.setMouseSensitivity(0.1f);
    
    // Create a 3x3 grid of chunks (9 total) with fully filled blocks
    // Set render distance to 5 (chunks) for optimal performance
    World world(5); // Render distance increased to 5 chunks for better visibility
    
    // Initialize Crosshair
    crosshair = new Crosshair(SCREEN_WIDTH, SCREEN_HEIGHT);

    // For measuring frame time
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    
    // FPS counter
    int frameCount = 0;
    float fpsCountTime = 0.0f;
    int fps = 0;
    
    // Main game loop
    while (!window.shouldClose()) {
        // Calculate deltaTime
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // Calculate FPS
        frameCount++;
        fpsCountTime += deltaTime;
        if (fpsCountTime >= 1.0f) {
            fps = frameCount;
            frameCount = 0;
            fpsCountTime = 0.0f;
        }
        
        // Process input
        camera.processKeyboard(window.getWindow(), deltaTime);
        
        // Get mouse offset for camera rotation
        double xOffset, yOffset;
        window.getMouseOffset(xOffset, yOffset);
        camera.processMouseMovement(xOffset, yOffset);
        window.resetMouseOffset();
        
        // Create view matrix from camera
        glm::mat4 view = camera.getViewMatrix();
        
        // Create projection matrix
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.getFov()),
            (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
            0.1f, 1000.0f
        );
        
        // Clear the screen with a nice sky blue color
        glClearColor(0.5f, 0.7f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Check for OpenGL errors after clearing
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL error after clearing screen: " << error << std::endl;
        }
        
        // Update world (load/unload chunks around player)
        world.update(camera);
        
        // Process tasks queued for the main thread (OpenGL operations)
        world.processMainThreadTasks();
        
        // Render world, passing wireframe state from the window
        world.render(projection, view, camera, window.isWireframeMode());
        
        // Render Crosshair
        if (crosshair) {
            crosshair->render();
        }
        
        // Display camera position (for debugging) and FPS - update once per second
        static float consoleUpdateTimer = 0.0f;
        consoleUpdateTimer += deltaTime;
        if (consoleUpdateTimer >= 1.0f) {
            std::cout << "\rCamera: [" << std::fixed << std::setprecision(1) << camera.getPosition().x 
                      << ", " << camera.getPosition().y 
                      << ", " << camera.getPosition().z << "]  "
                      << "FPS: " << fps << "     "; // Added spaces to overwrite previous longer lines, \r for same line
            consoleUpdateTimer = 0.0f;
        }
        
        // Swap buffers and poll events
        window.swapBuffers();
        window.pollEvents();
    }
    
    // Cleanup
    Block::CleanupBlockShader();
    if (crosshair) {
        delete crosshair;
        crosshair = nullptr;
    }
    
    std::cout << std::endl; // Ensure cursor moves to next line on exit
    glfwTerminate(); // Terminate GLFW at the end
    return 0;
}