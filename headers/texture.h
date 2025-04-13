#pragma once

#include <GL/glew.h>
#include <string>

class Texture {
private:
    unsigned int textureID;
    int width, height, channels;

public:
    Texture();
    ~Texture();

    // Loads a texture from a file
    bool loadFromFile(const std::string& filepath);
    
    // Binds the texture to the specified texture unit
    void bind(unsigned int textureUnit = 0) const;
    
    // Unbinds any bound texture
    static void unbind();
    
    // Returns the OpenGL texture ID
    unsigned int getID() const;
}; 