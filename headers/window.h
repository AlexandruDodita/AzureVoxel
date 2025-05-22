#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>

class Window {
private:
    GLFWwindow* window;
    int width;
    int height;
    std::string title;
    
    // Mouse position variables
    double lastX, lastY;
    double xOffset, yOffset;
    bool firstMouse;
    
    // Wireframe mode
    bool wireframeMode;
    
    // Callback functions
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    
    // Mapping from GLFWwindow to Window instance for callbacks
    static Window* currentWindow;
    
public:
    // New constructor that accepts an existing GLFWwindow
    Window(GLFWwindow* existingWindow, int width, int height);
    
    ~Window();
    
    bool shouldClose() const;
    void swapBuffers();
    void pollEvents();
    GLFWwindow* getWindow() const;
    
    // Mouse handling
    void enableMouseCapture(bool enable);
    void getMouseOffset(double& x, double& y);
    void resetMouseOffset();
    
    // Input handling
    bool isKeyPressed(int key) const;
    
    // Wireframe mode
    bool isWireframeMode() const { return wireframeMode; }
    void toggleWireframeMode();
    
    // Prevent copying
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
};
