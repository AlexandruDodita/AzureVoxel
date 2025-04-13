#include "../headers/texture.h"
#include <iostream>

// Include stb_image.h for texture loading
#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image.h"

Texture::Texture() : textureID(0), width(0), height(0), channels(0) {
}

Texture::~Texture() {
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
    }
}

bool Texture::loadFromFile(const std::string& filepath) {
    // If we already have a texture loaded, delete it
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
    
    // Load image using stb_image
    stbi_set_flip_vertically_on_load(true); // Flip the image vertically (OpenGL expects bottom-left origin)
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        std::cerr << "Error: " << stbi_failure_reason() << std::endl;
        return false;
    }
    
    // Generate texture
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // Use nearest for Minecraft-like look
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // Upload texture data
    GLenum format = GL_RGB;
    if (channels == 1) {
        format = GL_RED;
    } else if (channels == 3) {
        format = GL_RGB;
    } else if (channels == 4) {
        format = GL_RGBA;
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Free image data
    stbi_image_free(data);
    
    return true;
}

void Texture::bind(unsigned int textureUnit) const {
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, textureID);
}

void Texture::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}

unsigned int Texture::getID() const {
    return textureID;
} 