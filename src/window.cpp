#include "../headers/window.h"
#include <iostream>

// Static variables to store input state
Window* Window::currentWindow = nullptr;

Window::Window(int width, int height, const std::string& title)
    : width(width), height(height), title(title), window(nullptr),
      lastX(width / 2.0), lastY(height / 2.0), xOffset(0.0), yOffset(0.0), firstMouse(true),
      wireframeMode(false) {
    
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }
    
    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Create window
    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
    
    // Make the window's context current
    glfwMakeContextCurrent(window);
    
    // Set callbacks
    currentWindow = this;
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    
    // Enable vsync
    glfwSwapInterval(1);
}

Window::~Window() {
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
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
