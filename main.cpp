#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib> // For system
#include <iomanip> // For std::setprecision

#include "headers/window.h"
#include "headers/shader.h"
#include "headers/texture.h"
#include "headers/camera.h"
#include "headers/block.h"
#include "headers/world.h"
#include "headers/planet.h"
#include "headers/crosshair.h"

// Screen dimensions (can be const or from config)
const unsigned int SCREEN_WIDTH = 1280;
const unsigned int SCREEN_HEIGHT = 720;

// Global camera and world pointers (or managed within main/a game class)
Camera* camera = nullptr;
World* world = nullptr;
Crosshair* crosshair = nullptr;

// Frame timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Create Window object
    Window gameWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "AzureVoxel - Planets");
    if (!gameWindow.getWindow()) { // Check if window creation failed inside Window constructor
        std::cerr << "Failed to create GLFW window or initialize GLEW." << std::endl;
        glfwTerminate();
        return -1;
    }

    // Initialize GLEW (must be done after a valid GL context is created)
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate(); // Terminate GLFW if GLEW init fails
        return -1;
    }

    // Enable mouse capture for FPS camera
    gameWindow.enableMouseCapture(true);

    // Setup Dear ImGui context (if you add ImGui later)
    // ...

    // Initialize Shaders and Textures for Blocks (globally)
    Block::InitBlockShader();
    if (Block::shaderProgram == 0) {
        std::cerr << "Failed to initialize block shader program. Exiting." << std::endl;
        glfwTerminate();
        return -1;
    }
    Block::InitSpritesheet("res/textures/Spritesheet.PNG");
    if (!Block::spritesheetLoaded) {
        std::cout << "Warning: Global spritesheet res/textures/Spritesheet.PNG not loaded. Blocks may not texture correctly." << std::endl;
    }

    // OpenGL settings
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Create Camera
    camera = new Camera(glm::vec3(0.0f, 0.0f, 150.0f)); // Start further out to see a planet (e.g. radius 50 at origin)
    camera->setMovementSpeed(50.0f); // Increase speed for planetary scale

    // Create World
    world = new World("SolarSystem"); // Give your world a name

    // Add a planet to the world
    world->addPlanet(glm::vec3(0.0f, 0.0f, 0.0f), 50.0f, 123, "Terra"); // Planet at origin, radius 50
    // world->addPlanet(glm::vec3(150.0f, 0.0f, 0.0f), 25.0f, 456, "Luna"); // A smaller moon

    // Create Crosshair
    crosshair = new Crosshair(SCREEN_WIDTH, SCREEN_HEIGHT);

    // Performance metrics
    double lastTime = glfwGetTime();
    int nbFrames = 0;

    // Main game loop
    while (!gameWindow.shouldClose()) {
        // Per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Performance monitoring (FPS)
        nbFrames++;
        if (currentFrame - lastTime >= 1.0) { // If last prinf() was more than 1 sec ago
            char title[256];
            sprintf(title, "AzureVoxel - Planets | FPS: %d | Pos: (%.1f, %.1f, %.1f)", 
                    nbFrames, camera->getPosition().x, camera->getPosition().y, camera->getPosition().z);
            glfwSetWindowTitle(gameWindow.getWindow(), title);
            nbFrames = 0;
            lastTime += 1.0;
        }

        // Process input (Keyboard for camera, window events)
        camera->processKeyboard(gameWindow.getWindow(), deltaTime);
        // Mouse input is handled by callback in Window class, which updates camera via getMouseOffset
        double xOffset, yOffset;
        gameWindow.getMouseOffset(xOffset, yOffset); // This gets and resets offsets
        camera->processMouseMovement(static_cast<float>(xOffset), static_cast<float>(yOffset));
        // gameWindow.resetMouseOffset(); // Ensure offsets are reset if not done in getMouseOffset

        // Update game state
        if (world) {
            world->update(*camera);
            world->processMainThreadTasks(); // Process tasks queued by worker threads for main thread (e.g. OpenGL calls)
        }
        // crosshair->updateScreenSize(newWidth, newHeight); // If window resizing is handled

        // Render
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f); // Dark blue background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Get view and projection matrices
        glm::mat4 projection = glm::perspective(glm::radians(camera->getFov()), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 1000.0f); // Increased far plane for planets
        glm::mat4 view = camera->getViewMatrix();

        // Render world (which renders planets, which render chunks)
        if (world) {
            world->render(projection, view, *camera, gameWindow.isWireframeMode());
        }

        // Render Crosshair (2D overlay)
        if (crosshair) {
            crosshair->render();
        }

        // Swap buffers and poll IO events
        gameWindow.swapBuffers();
        glfwPollEvents();
    }

    // Cleanup
    std::cout << "Cleaning up resources..." << std::endl;
    Block::CleanupBlockShader();
    delete crosshair;
    delete world;
    delete camera;
    // Window destructor handles glfwTerminate()

    std::cout << "AzureVoxel Planet Engine shutdown complete." << std::endl;
    return 0;
}