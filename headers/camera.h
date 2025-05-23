#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
private:
    // Camera vectors
    glm::vec3 position;
    glm::vec3 worldUp;
    
    // Euler angles (declared before vectors that depend on them)
    float yaw;
    float pitch;

    // Camera vectors (calculated from yaw/pitch)
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    
    // Camera options
    float movementSpeed;
    float mouseSensitivity;
    float fov;
    
    // Updates the camera vectors based on the current Euler angles
    void updateCameraVectors();
    
public:
    // Constructor
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f), 
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f, float pitch = 0.0f);
    
    // Returns the view matrix calculated using the camera's updated vectors
    glm::mat4 getViewMatrix() const;
    
    // Processes input from keyboard for camera movement
    void processKeyboard(GLFWwindow* window, float deltaTime);
    
    // Processes input from mouse for camera orientation
    void processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);
    
    // Getters
    glm::vec3 getPosition() const;
    glm::vec3 getFront() const;
    float getFov() const;
    
    // Setters
    void setPosition(const glm::vec3& newPosition);
    void setMovementSpeed(float speed);
    void setMouseSensitivity(float sensitivity);
    void setFov(float newFov);
}; 