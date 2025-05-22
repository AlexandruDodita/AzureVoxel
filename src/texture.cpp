#include "../headers/texture.h"
#include <iostream>
#include <filesystem> // For std::filesystem::absolute -- DEBUGGING
#include <thread>
#include <GLFW/glfw3.h> // For glfwGetCurrentContext

// Include stb_image.h for texture loading
#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image.h"

Texture::Texture() : textureID(0), width(0), height(0), channels(0), isShared(false) {
}

Texture::~Texture() {
    // Only delete the texture if it's not shared
    if (textureID != 0 && !isShared) {
        glDeleteTextures(1, &textureID);
    }
}

// Copy constructor - share the texture ID
Texture::Texture(const Texture& other)
    : textureID(other.textureID), width(other.width), height(other.height), 
      channels(other.channels), isShared(true) {
}

// Assignment operator - share the texture ID
Texture& Texture::operator=(const Texture& other) {
    if (this != &other) {
        // Clean up current texture if we own it
        if (textureID != 0 && !isShared) {
            glDeleteTextures(1, &textureID);
        }
        
        // Copy properties
        textureID = other.textureID;
        width = other.width;
        height = other.height;
        channels = other.channels;
        isShared = true; // Mark as shared
    }
    return *this;
}

bool Texture::loadFromFile(const std::string& filepath) {
    // Check if OpenGL context is current
    if (glfwGetCurrentContext() == nullptr) {
        std::cerr << "ERROR::TEXTURE::NO_CONTEXT: No OpenGL context is current during texture loading!" << std::endl;
        return false;
    }
    
    // If we already have a texture loaded, delete it
    if (textureID != 0 && !isShared) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
    
    // Load image using stb_image
    stbi_set_flip_vertically_on_load(true); // Flip the image vertically (OpenGL expects bottom-left origin)
    
    std::cout << "Attempting to load texture from (absolute path): " << std::filesystem::absolute(filepath) << std::endl; // DEBUGGING
    
    // Check if file exists before loading
    if (!std::filesystem::exists(filepath)) {
        std::cerr << "ERROR::TEXTURE::FILE_NOT_FOUND: " << filepath << std::endl;
        std::cerr << "Working directory: " << std::filesystem::current_path() << std::endl;
        return false;
    }

    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
        std::cerr << "ERROR::TEXTURE::LOAD_FAILED: Could not load texture file: " << filepath << std::endl;
        std::cerr << "STB_IMAGE Error: " << stbi_failure_reason() << std::endl;
        std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
        return false;
    }
    
    std::cout << "Successfully loaded texture: " << filepath << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;
    
    // Clear any previous OpenGL errors
    while (glGetError() != GL_NO_ERROR) {}
    
    // Force synchronization with the driver before creating resources
    glFinish();
    
    // Generate texture
    GLuint newTextureID = 0;
    glGenTextures(1, &newTextureID);
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR || newTextureID == 0) {
        std::cerr << "ERROR::TEXTURE::CREATION_FAILED: Failed to generate texture (error " << error << ")" << std::endl;
        
        // Additional debugging info
        std::cout << "DEBUG: OpenGL context: " << glfwGetCurrentContext() << std::endl;
        std::cout << "DEBUG: OpenGL version: " << glGetString(GL_VERSION) << std::endl;
        
        // Try a second time after a short delay
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Retrying texture generation..." << std::endl;
        glGenTextures(1, &newTextureID);
        error = glGetError();
        if (error != GL_NO_ERROR || newTextureID == 0) {
            std::cerr << "ERROR::TEXTURE::RETRY_FAILED: Second attempt failed (error " << error << ")" << std::endl;
            stbi_image_free(data);
            return false;
        } else {
            std::cout << "Retry successful! Texture ID: " << newTextureID << std::endl;
        }
    }
    
    glBindTexture(GL_TEXTURE_2D, newTextureID);
    
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "ERROR::TEXTURE::BIND_FAILED: Failed to bind texture (error " << error << ")" << std::endl;
        glDeleteTextures(1, &newTextureID);
        stbi_image_free(data);
        return false;
    }
    
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
    
    // Upload the texture data
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "ERROR::TEXTURE::UPLOAD_FAILED: Failed to upload texture data (error " << error << ")" << std::endl;
        glDeleteTextures(1, &newTextureID);
        stbi_image_free(data);
        return false;
    }
    
    // Generate mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);
    
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "ERROR::TEXTURE::MIPMAP_FAILED: Failed to generate mipmaps (error " << error << ")" << std::endl;
        // Not fatal, continue
    }
    
    // Free image data
    stbi_image_free(data);
    
    // Assign the new texture ID
    textureID = newTextureID;
    
    // This is not a shared texture
    isShared = false;
    
    std::cout << "Texture created successfully with ID: " << textureID << std::endl;
    return true;
}

bool Texture::loadFromSpritesheet(const std::string& filepath, int atlasX, int atlasY, int atlasWidth, int atlasHeight) {
    // Check if OpenGL context is current
    if (glfwGetCurrentContext() == nullptr) {
        std::cerr << "ERROR::TEXTURE::NO_CONTEXT: No OpenGL context is current during spritesheet loading!" << std::endl;
        return false;
    }
    
    if (textureID != 0 && !isShared) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }

    stbi_set_flip_vertically_on_load(true);
    int fullWidth, fullHeight, fullChannels;
    
    std::cout << "Attempting to load spritesheet from (absolute path): " << std::filesystem::absolute(filepath) << std::endl; // DEBUGGING
    
    // Check if file exists before loading
    if (!std::filesystem::exists(filepath)) {
        std::cerr << "ERROR::TEXTURE::SPRITESHEET_FILE_NOT_FOUND: " << filepath << std::endl;
        std::cerr << "Working directory: " << std::filesystem::current_path() << std::endl;
        return false;
    }

    unsigned char* fullData = stbi_load(filepath.c_str(), &fullWidth, &fullHeight, &fullChannels, 0);

    if (!fullData) {
        std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
        std::cerr << "ERROR::TEXTURE::LOAD_FAILED: Could not load texture file: " << filepath << std::endl;
        std::cerr << "STB_IMAGE Error: " << stbi_failure_reason() << std::endl;
        std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
        return false;
    }
    
    std::cout << "Successfully loaded spritesheet: " << filepath << " (" << fullWidth << "x" << fullHeight << ", " << fullChannels << " channels)" << std::endl;
    std::cout << "Extracting region: x=" << atlasX << ", y=" << atlasY << ", w=" << atlasWidth << ", h=" << atlasHeight << std::endl;

    // Validate atlas coordinates and dimensions
    if (atlasX < 0 || atlasY < 0 || atlasWidth <= 0 || atlasHeight <= 0 ||
        atlasX + atlasWidth > fullWidth || atlasY + atlasHeight > fullHeight) {
        std::cerr << "ERROR::TEXTURE::SPRITESHEET_INVALID_REGION: Invalid region specified for spritesheet." << std::endl;
        stbi_image_free(fullData);
        return false;
    }

    // Allocate memory for the sub-image
    unsigned char* subImageData = new unsigned char[atlasWidth * atlasHeight * fullChannels];

    // Copy the sub-image data
    for (int y = 0; y < atlasHeight; ++y) {
        for (int x = 0; x < atlasWidth; ++x) {
            int fullIndex = ((atlasY + y) * fullWidth + (atlasX + x)) * fullChannels;
            int subIndex = (y * atlasWidth + x) * fullChannels;
            for (int c = 0; c < fullChannels; ++c) {
                subImageData[subIndex + c] = fullData[fullIndex + c];
            }
        }
    }

    width = atlasWidth;
    height = atlasHeight;
    channels = fullChannels;

    // Clear any previous OpenGL errors
    while (glGetError() != GL_NO_ERROR) {}
    
    // Generate texture
    GLuint newTextureID = 0;
    glGenTextures(1, &newTextureID);
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR || newTextureID == 0) {
        std::cerr << "ERROR::TEXTURE::CREATION_FAILED: Failed to generate texture for spritesheet (error " << error << ")" << std::endl;
        stbi_image_free(fullData);
        delete[] subImageData;
        return false;
    }
    
    glBindTexture(GL_TEXTURE_2D, newTextureID);
    
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "ERROR::TEXTURE::BIND_FAILED: Failed to bind texture for spritesheet (error " << error << ")" << std::endl;
        glDeleteTextures(1, &newTextureID);
        stbi_image_free(fullData);
        delete[] subImageData;
        return false;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLenum format = GL_RGB;
    if (channels == 1) {
        format = GL_RED;
    } else if (channels == 3) {
        format = GL_RGB;
    } else if (channels == 4) {
        format = GL_RGBA;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, subImageData);
    
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "ERROR::TEXTURE::UPLOAD_FAILED: Failed to upload spritesheet data (error " << error << ")" << std::endl;
        glDeleteTextures(1, &newTextureID);
        stbi_image_free(fullData);
        delete[] subImageData;
        return false;
    }
    
    glGenerateMipmap(GL_TEXTURE_2D);
    
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "ERROR::TEXTURE::MIPMAP_FAILED: Failed to generate mipmaps for spritesheet (error " << error << ")" << std::endl;
        // Not fatal, continue
    }

    stbi_image_free(fullData);
    delete[] subImageData;

    // Assign the new texture ID
    textureID = newTextureID;
    isShared = false;
    
    std::cout << "Spritesheet texture created successfully with ID: " << textureID << std::endl;
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