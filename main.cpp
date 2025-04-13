#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "headers/window.h"
#include "headers/block.h"
#include "headers/texture.h"
#include "headers/camera.h"

// Window dimensions
const unsigned int SCREEN_WIDTH = 800;
const unsigned int SCREEN_HEIGHT = 600;

// Timing
float deltaTime = 0.0f;  // Time between current frame and last frame
float lastFrame = 0.0f;  // Time of last frame

int main() {
    // Create window
    Window window(SCREEN_WIDTH, SCREEN_HEIGHT, "Voxel World");
    
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
    
    // Create a camera
    Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
    camera.setMouseSensitivity(0.05f); // Reduced mouse sensitivity
    
    // Create a block (grass block)
    Block block(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.8f, 0.8f, 0.8f), 1.0f);
    block.init();
    
    // Load grass texture
    if (!block.loadTexture("res/textures/grass_block.png")) {
        std::cerr << "Failed to load grass block texture!" << std::endl;
    }
    
    // Game loop
    while (!window.shouldClose()) {
        // Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
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
            100.0f                                             // Far plane
        );
        
        // Clear the screen
        glClearColor(0.2f, 0.3f, 0.8f, 1.0f);  // Sky blue color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Render the block
        block.render(projection, view);
        
        // Swap buffers and poll events
        window.swapBuffers();
        window.pollEvents();
    }
    
    return 0;
}