#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "shader.h"

class Crosshair {
public:
    Crosshair(int screenWidth, int screenHeight);
    ~Crosshair();

    void render();
    void updateScreenSize(int screenWidth, int screenHeight); // In case of window resize

private:
    Shader shader_;
    GLuint VAO_, VBO_;
    glm::mat4 projection_;
    int screenWidth_, screenHeight_;

    void setupMesh();
}; 