#include "../headers/camera.h"
#include <iostream>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : position(position), worldUp(up), yaw(yaw), pitch(pitch),
      front(glm::vec3(0.0f, 0.0f, -1.0f)), movementSpeed(2.5f), mouseSensitivity(0.05f), fov(45.0f) {
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

void Camera::processKeyboard(GLFWwindow* window, float deltaTime) {
    float velocity = movementSpeed * deltaTime;
    
    // Forward
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        position += front * velocity;
    
    // Backward
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        position -= front * velocity;
    
    // Left
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        position -= right * velocity;
    
    // Right
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        position += right * velocity;
    
    // Up
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        position += up * velocity;
    
    // Down
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        position -= up * velocity;
}

void Camera::processMouseMovement(float xOffset, float yOffset, bool constrainPitch) {
    xOffset *= mouseSensitivity;
    yOffset *= mouseSensitivity;
    
    yaw += xOffset;
    pitch += yOffset;  // Note: Window class already handles the y-axis inversion so we add here
    
    if (constrainPitch) {
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;
    }
    
    updateCameraVectors();
}

void Camera::updateCameraVectors() {
    // Calculate the new front vector
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);
    
    // Recalculate the right and up vectors
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

glm::vec3 Camera::getPosition() const {
    return position;
}

glm::vec3 Camera::getFront() const {
    return front;
}

float Camera::getFov() const {
    return fov;
}

void Camera::setPosition(const glm::vec3& newPosition) {
    position = newPosition;
}

void Camera::setMovementSpeed(float speed) {
    movementSpeed = speed;
}

void Camera::setMouseSensitivity(float sensitivity) {
    mouseSensitivity = sensitivity;
}

void Camera::setFov(float newFov) {
    fov = newFov;
} 