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
    // OpenGL objects for individual block rendering (less common with chunk meshing)
    GLuint VAO = 0, VBO = 0, EBO = 0, texCoordVBO = 0;
    
    // Block properties
    glm::vec3 position;
    glm::vec3 color;
    float size;
    
    // Texture (can be shared or represent a sub-texture from an atlas)
    Texture texture; // For individual block texture or as a template
    bool hasTexture = false; 
    
    float speed = 0.05f; // Example property, adjust as needed
    
public:
    // Public Static Members:
    static GLuint shaderProgram;      // Shared shader program for all blocks
    static Texture spritesheetTexture; // Global spritesheet for atlas texturing
    static bool spritesheetLoaded;    // Flag to check if the global spritesheet is loaded

    // Static method to initialize the shared shader program
    static void InitBlockShader();
    // Static method to initialize the global spritesheet texture
    static void InitSpritesheet(const std::string& path);
    // Static method to clean up the shared shader program
    static void CleanupBlockShader();

    Block(const glm::vec3& position, const glm::vec3& color = glm::vec3(0.5f), float size = 1.0f);
    ~Block();
    
    void init(); // Initializes VAO/VBO/EBO for individual block rendering
    void render(const glm::mat4& projection, const glm::mat4& view); // Renders this single block
    
    void move(const glm::vec3& offset, float deltaTime); // Consistent with documentation
    void setPosition(const glm::vec3& newPosition);
    
    // Texture methods
    bool loadTexture(const std::string& filepath); // Load a unique texture for this block instance
    // Load a specific part of a spritesheet (could be used for specific blocks if not using mesh UVs)
    bool loadTexture(const std::string& spritesheetPath, int atlasX, int atlasY, int atlasWidth, int atlasHeight);
    
    // Share texture AND shader program ID from another block (shader part is less relevant now)
    void shareTextureAndShaderFrom(const Block& other);
    
    glm::vec3 getPosition() const;
    glm::vec3 getColor() const;
    
    // Methods needed by Chunk for mesh rendering or general block info
    GLuint getShaderProgram() const { return Block::shaderProgram; } 
    bool   hasTextureState() const { return hasTexture; } 
    GLuint getTextureID() const { return texture.getID(); }
    
    // Public helpers used internally and potentially by Chunk for individual block rendering
    void useBlockShader() const; 
    void bindBlockTexture() const; 
    void setShaderUniforms(const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model) const; 
};
