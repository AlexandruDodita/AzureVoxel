#include "../headers/window.h"
#include <iostream>

// Static variables to store input state
Window* Window::currentWindow = nullptr;

// New constructor that uses an existing GLFWwindow
Window::Window(GLFWwindow* existingWindow, int width, int height)
    : width(width), height(height), title("AzureVoxel"), window(existingWindow),
      lastX(width / 2.0), lastY(height / 2.0), xOffset(0.0), yOffset(0.0), firstMouse(true),
      wireframeMode(false) {
    
    // The window is already created, just make sure we have the callbacks set
    if (!window) {
        std::cerr << "Error: Null window handle provided to Window constructor" << std::endl;
        return;
    }
    
    // Set callbacks
    currentWindow = this;
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    
    std::cout << "Window object initialized with existing GLFWwindow handle" << std::endl;
}

Window::~Window() {
    if (window) {
        // Only destroy the window if we created it in our original constructor
        // Don't destroy if we received an existing window handle
        // (that would be the responsibility of main.cpp)
    }
    // Don't terminate GLFW here anymore, as we might be using a window created elsewhere
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(window);
}

void Window::swapBuffers() {
    glfwSwapBuffers(window);
}

void Window::pollEvents() {
    glfwPollEvents();
}

GLFWwindow* Window::getWindow() const {
    return window;
}

bool Window::isKeyPressed(int key) const {
    return glfwGetKey(window, key) == GLFW_PRESS;
}

void Window::enableMouseCapture(bool enable) {
    // DISCLAIMER: Mouse capture functionality is currently disabled because it can cause 
    // issues with certain window managers and mouse input systems.
    // Uncomment the following code to re-enable if needed and your system supports it.
    //
    // if (enable) {
    //     glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // } else {
    //     glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    // }
}

void Window::getMouseOffset(double& x, double& y) {
    x = xOffset;
    y = yOffset;
}

void Window::resetMouseOffset() {
    xOffset = 0.0;
    yOffset = 0.0;
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Close window on escape
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    // Toggle wireframe mode on X key
    if (key == GLFW_KEY_X && action == GLFW_PRESS && currentWindow) {
        currentWindow->toggleWireframeMode();
    }
}

void Window::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (!currentWindow) return;
    
    if (currentWindow->firstMouse) {
        currentWindow->lastX = xpos;
        currentWindow->lastY = ypos;
        currentWindow->firstMouse = false;
    }
    
    // Calculate the offset movement between the last and current frame
    currentWindow->xOffset = xpos - currentWindow->lastX;
    currentWindow->yOffset = currentWindow->lastY - ypos; // Reversed since y-coordinates range from bottom to top
    
    // Update the last position for the next frame
    currentWindow->lastX = xpos;
    currentWindow->lastY = ypos;
}

void Window::toggleWireframeMode() {
    wireframeMode = !wireframeMode;
    if (wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        std::cout << "Wireframe mode enabled" << std::endl;
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        std::cout << "Wireframe mode disabled" << std::endl;
    }
}
