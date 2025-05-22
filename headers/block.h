#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "texture.h"
#include <memory>
#include <string>

class Block {
private:
    // OpenGL objects
    GLuint VAO = 0, VBO = 0, EBO = 0, texCoordVBO = 0;
    // static GLuint shaderProgram; // Made static, will be defined in .cpp
    
    // Block properties
    glm::vec3 position;
    glm::vec3 color;
    float size;
    
    // Texture (can be shared)
    Texture texture;
    bool hasTexture = false; // Initialize to false
    
    // Movement speed
    float speed = 0.05f;
    
    // Shader utility functions moved to static or part of static InitBlockShader
    // unsigned int compileShader(const char* vertexShaderSource, const char* fragmentShaderSource);
    // void checkCompileErrors(unsigned int shader_or_program_ID, std::string type);
    
public:
    static GLuint shaderProgram; // Public static for now, or use a getter

    // Static method to initialize the shared shader program
    static void InitBlockShader();
    // Static method to clean up the shared shader program
    static void CleanupBlockShader();

    Block(const glm::vec3& position, const glm::vec3& color, float size);
    ~Block();
    
    void init(); // Initializes VAO/VBO/EBO and compiles shader if not already done
    void render(const glm::mat4& projection, const glm::mat4& view); // Renders the single block
    void move(float dx, float dy, float dz);
    void setPosition(const glm::vec3& newPosition);
    
    // Texture methods
    bool loadTexture(const std::string& filepath);
    bool loadTexture(const std::string& spritesheetPath, int atlasX, int atlasY, int atlasWidth, int atlasHeight);
    // Pass by const reference to avoid overhead and indicate non-ownership transfer
    void shareTextureAndShaderFrom(const Block& other);
    
    glm::vec3 getPosition() const;
    glm::vec3 getColor() const;
    
    // Methods needed by Chunk for mesh rendering
    GLuint getShaderProgram() const { return Block::shaderProgram; } // Return static program
    bool   hasTextureState() const { return hasTexture; } // Renamed to avoid clash with Texture class
    GLuint getTextureID() const { return texture.getID(); }
    
    // Public helpers used internally and by Chunk
    void useBlockShader() const; // Activate the shader program
    void bindBlockTexture() const; // Bind the texture 
    void setShaderUniforms(const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model) const; // Set necessary uniforms
};
