#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib> // For system

#include "headers/window.h"
#include "headers/block.h"
#include "headers/texture.h"
#include "headers/camera.h"
#include "headers/world.h"

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

int main() {
    // Create window
    Window window(SCREEN_WIDTH, SCREEN_HEIGHT, "AzureVoxel - Press X for wireframe mode");
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
        return -1;
    }
    
    // Print OpenGL version
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Create a camera with elevated position for better view of terrain
    Camera camera(glm::vec3(0.0f, 10.0f, 0.0f));
    camera.setMouseSensitivity(0.05f); // Reduced mouse sensitivity
    
    // Generate texture before creating the world
    // std::cout << "Generating textures..." << std::endl;
    // Use path relative to the project root, not the build directory
    // system("cd .. && python3 create_texture.py");
    
    // Create a 3x3 grid of chunks (9 total) with fully filled blocks
    // Set render distance to 2 (chunks) for optimal performance
    // std::cout << "Creating world with filled chunks..." << std::endl; // Removed this line
    World world(3); // Render distance changed to 3 chunks (adjust as needed for performance/view)
    // world.init(3); // REMOVED - World is now loaded dynamically
    
    // Game loop
    while (!window.shouldClose()) {
        // Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // Calculate FPS
        frameCount++;
        if (currentFrame - lastFpsTime >= 1.0f) {
            fps = frameCount / (currentFrame - lastFpsTime);
            frameCount = 0;
            lastFpsTime = currentFrame;
        }
        
        // Process keyboard input for camera movement
        camera.processKeyboard(window.getWindow(), deltaTime);
        
        // Process mouse input for camera orientation
        double mouseX, mouseY;
        window.getMouseOffset(mouseX, mouseY);
        if (mouseX != 0.0 || mouseY != 0.0) {
            camera.processMouseMovement(mouseX, mouseY);
        }
        window.resetMouseOffset();
        
        // Create view matrix
        glm::mat4 view = camera.getViewMatrix();
        
        // Create projection matrix
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.getFov()),                     // FOV
            static_cast<float>(SCREEN_WIDTH) / SCREEN_HEIGHT,  // Aspect ratio
            0.1f,                                              // Near plane
            1000.0f                                            // Far plane (increased for larger world)
        );
        
        // Clear the screen
        glClearColor(0.2f, 0.3f, 0.8f, 1.0f);  // Sky blue color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Update the world (load/unload chunks, build meshes)
        world.update(camera);

        // Render the world (only chunks within render distance)
        world.render(projection, view, camera);
        
        // Display camera position and FPS
        std::cout << "\rCamera: [" << camera.getPosition().x << ", " 
                  << camera.getPosition().y << ", " 
                  << camera.getPosition().z << "]  FPS: " 
                  << static_cast<int>(fps) << "   " << std::flush;
        
        // Swap buffers and poll events
        window.swapBuffers();
        window.pollEvents();
    }
    
    std::cout << std::endl; // Add a newline after the loop ends
    return 0;
}