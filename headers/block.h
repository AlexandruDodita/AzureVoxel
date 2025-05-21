#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "texture.h"
#include <memory>

class Block {
private:
    // OpenGL objects
    GLuint VAO = 0, VBO = 0, EBO = 0, texCoordVBO = 0;
    GLuint shaderProgram = 0; // Initialize to 0
    
    // Block properties
    glm::vec3 position;
    glm::vec3 color;
    float size;
    
    // Texture (can be shared)
    Texture texture;
    bool hasTexture = false; // Initialize to false
    
    // Movement speed
    float speed = 0.05f;
    
    // Shader utility functions
    // Make static if it doesn't depend on instance members?
    unsigned int compileShader(const char* vertexShaderSource, const char* fragmentShaderSource);
    
public:
    Block(const glm::vec3& position, const glm::vec3& color, float size);
    ~Block();
    
    void init(); // Initializes VAO/VBO/EBO and compiles shader if not already done
    void render(const glm::mat4& projection, const glm::mat4& view); // Renders the single block
    void move(float dx, float dy, float dz);
    void setPosition(const glm::vec3& newPosition);
    
    // Texture methods
    bool loadTexture(const std::string& texturePath);
    bool loadTexture(const std::string& spritesheetPath, int atlasX, int atlasY, int atlasWidth, int atlasHeight);
    // Pass by const reference to avoid overhead and indicate non-ownership transfer
    void shareTextureAndShaderFrom(const Block& other);
    
    glm::vec3 getPosition() const;
    glm::vec3 getColor() const;
    
    // Methods needed by Chunk for mesh rendering
    GLuint getShaderProgram() const { return shaderProgram; }
    bool   hasTextureState() const { return hasTexture; } // Renamed to avoid clash with Texture class
    GLuint getTextureID() const { return texture.getID(); }
    
    // Public helpers used internally and by Chunk
    void useBlockShader() const; // Activate the shader program
    void bindBlockTexture() const; // Bind the texture 
    void setShaderUniforms(const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model) const; // Set necessary uniforms
};
