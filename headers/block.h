#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "texture.h"

class Block {
private:
    // OpenGL objects
    unsigned int VAO, VBO, EBO, texCoordVBO;
    unsigned int shaderProgram;
    
    // Block properties
    glm::vec3 position;
    glm::vec3 color;
    float size;
    
    // Texture
    Texture texture;
    bool hasTexture;
    
    // Movement speed
    float speed;
    
    // Shader utility functions
    unsigned int compileShader(const char* vertexShaderSource, const char* fragmentShaderSource);
    
public:
    Block(const glm::vec3& position, const glm::vec3& color, float size);
    ~Block();
    
    void init();
    void render(const glm::mat4& projection, const glm::mat4& view);
    void move(float dx, float dy, float dz);
    void setPosition(const glm::vec3& newPosition);
    
    // Texture methods
    bool loadTexture(const std::string& texturePath);
    
    glm::vec3 getPosition() const;
};
