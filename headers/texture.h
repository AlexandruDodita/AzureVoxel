#pragma once

#include <GL/glew.h>
#include <string>

class Texture {
private:
    unsigned int textureID;
    int width, height, channels;
    bool isShared; // Flag to indicate if texture is shared (to avoid double deletion)

public:
    Texture();
    ~Texture();
    
    // Copy constructor and assignment operator for texture sharing
    Texture(const Texture& other);
    Texture& operator=(const Texture& other);

    // Loads a texture from a file
    bool loadFromFile(const std::string& filepath);
    
    // Loads a texture from a specific region of a spritesheet
    bool loadFromSpritesheet(const std::string& filepath, int atlasX, int atlasY, int atlasWidth, int atlasHeight);
    
    // Binds the texture to the specified texture unit
    void bind(unsigned int textureUnit = 0) const;
    
    // Unbinds any bound texture
    static void unbind();
    
    // Returns the OpenGL texture ID
    unsigned int getID() const;

    // Public Methods:
    int getWidth() const { return width; }      // Getter for width
    int getHeight() const { return height; }    // Getter for height
    int getChannels() const { return channels; } // Getter for channels
}; 