#include "../headers/block.h"
#include "../headers/block_registry.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr
#include <filesystem>
#include "../headers/texture.h" // Make sure Texture header is included for Block::spritesheetTexture

// Vertex shader source code
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    out vec2 TexCoord;
    
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        TexCoord = aTexCoord;
    }
)";

// Fragment shader source code
const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    in vec2 TexCoord;
    
    uniform vec3 blockColor; // Keep for potential future use without textures
    uniform sampler2D blockTexture;
    uniform bool useTexture;
    
    void main() {
        if (useTexture) {
            vec4 texColor = texture(blockTexture, TexCoord);
            if(texColor.a < 0.1) discard;
            FragColor = texColor;
        } else {
            // Fallback color if no texture (can use blockColor uniform)
             FragColor = vec4(blockColor, 1.0);
            // vec3 defaultColor = vec3(0.5, 0.5, 0.5); // Gray fallback
            // FragColor = vec4(defaultColor, 1.0);
        }
    }
)";

// Define static members
GLuint Block::shaderProgram = 0;
Texture Block::spritesheetTexture; // Default constructor for Texture
bool Block::spritesheetLoaded = false;

// Helper function for compiling/linking (can be static inside .cpp)
// Moved from Block class to be a static helper here, or part of InitBlockShader
/*
static void checkShaderCompileErrors(unsigned int shader, std::string type) {
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
}
*/

void Block::InitBlockShader() {
    if (Block::shaderProgram != 0) {
        // Already initialized
        std::cout << "Block shader already initialized with program ID: " << Block::shaderProgram << std::endl;
        return;
    }
    
    std::cout << "Initializing block shader..." << std::endl;

    // Force-clear any existing OpenGL errors before we start
    while (glGetError() != GL_NO_ERROR) {}

    // Create vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (vertexShader == 0) {
        std::cerr << "ERROR::SHADER::VERTEX::CREATE_FAILED OpenGL error: " << glGetError() << std::endl;
        return;
    }
    
    // Compile vertex shader
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Check for compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, sizeof(infoLog), NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return;
    }

    // Create fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (fragmentShader == 0) {
        std::cerr << "ERROR::SHADER::FRAGMENT::CREATE_FAILED OpenGL error: " << glGetError() << std::endl;
        glDeleteShader(vertexShader);
        return;
    }
    
    // Compile fragment shader
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    // Check for compilation errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, sizeof(infoLog), NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return;
    }

    // Create shader program
    unsigned int program = glCreateProgram();
    if (program == 0) {
        std::cerr << "ERROR::SHADER::PROGRAM::CREATE_FAILED OpenGL error: " << glGetError() << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return;
    }
    
    // Attach shaders to program
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    
    // Link program
    glLinkProgram(program);
    
    // Check for linking errors
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);
        return;
    }
    
    // Delete the shader objects (they're now linked into the program)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Store the program handle in the static variable
    Block::shaderProgram = program;
    
    std::cout << "Block shader successfully initialized with program ID: " << Block::shaderProgram << std::endl;
    
    // Validate the program and check for any OpenGL errors
    glValidateProgram(Block::shaderProgram);
    glGetProgramiv(Block::shaderProgram, GL_VALIDATE_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(Block::shaderProgram, sizeof(infoLog), NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::VALIDATION_FAILED\n" << infoLog << std::endl;
    }
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error after shader initialization: " << error << std::endl;
    }
}

void Block::CleanupBlockShader() {
    if (Block::shaderProgram != 0) {
        glDeleteProgram(Block::shaderProgram);
        Block::shaderProgram = 0;
        std::cout << "Block shader program cleaned up." << std::endl;
    }
}

// Updated constructors to work with BlockRegistry
Block::Block(const glm::vec3& position, const glm::vec3& color, float size)
    : position(position), color(color), size(size), block_type_id(0), hasTexture(false) 
{
}

Block::Block(const glm::vec3& position, uint16_t blockType, const glm::vec3& color, float size)
    : position(position), color(color), size(size), block_type_id(blockType), hasTexture(false)
{
}

Block::~Block() {
    // Only delete GL objects if they were actually generated by this instance
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (EBO != 0) {
        glDeleteBuffers(1, &EBO);
        EBO = 0;
    }
    if (texCoordVBO != 0) {
        glDeleteBuffers(1, &texCoordVBO);
        texCoordVBO = 0;
    }
}

// Block type management
void Block::setBlockType(uint16_t typeId) {
    block_type_id = typeId;
}

// Registry-powered property queries
bool Block::isSolid() const {
    return BlockRegistry::getInstance().isBlockSolid(block_type_id);
}

bool Block::isTransparent() const {
    return BlockRegistry::getInstance().isBlockTransparent(block_type_id);
}

bool Block::isLightSource() const {
    return BlockRegistry::getInstance().getBlockLightLevel(block_type_id) > 0;
}

uint8_t Block::getLightLevel() const {
    return BlockRegistry::getInstance().getBlockLightLevel(block_type_id);
}

std::string Block::getBlockName() const {
    return BlockRegistry::getInstance().getBlockName(block_type_id);
}

// Static backward compatibility methods
bool Block::isTypeSolid(unsigned char blockType) {
    return BlockRegistry::getInstance().isBlockSolid(static_cast<uint16_t>(blockType));
}

bool Block::isTypeSolid(uint16_t blockTypeId) {
    return BlockRegistry::getInstance().isBlockSolid(blockTypeId);
}

void Block::init() {
    if (VAO != 0) return; // Already initialized

    // Simple cube vertices for individual block rendering
    // (This is less common now with chunk meshing, but still used for debugging)
    if (VAO == 0) {
        float halfSize = size / 2.0f;

        float vertices[] = {
            // Positions          
            // Front face
            -halfSize, -halfSize,  halfSize,
             halfSize, -halfSize,  halfSize,
             halfSize,  halfSize,  halfSize,
            -halfSize,  halfSize,  halfSize,
            // Back face
            -halfSize, -halfSize, -halfSize,
             halfSize, -halfSize, -halfSize,
             halfSize,  halfSize, -halfSize,
            -halfSize,  halfSize, -halfSize
            // ... (Add other faces if needed for single block rendering, 
            //      but mesh builder uses simpler face data)
        };

        float texCoords[] = {
            // Front face
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f,
            // Back face
            1.0f, 0.0f,
            0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f
            // ... (Tex coords for other faces)
        };

        unsigned int indices[] = {
            // Front face
            0, 1, 2, 2, 3, 0,
            // Back face
            4, 5, 6, 6, 7, 4
            // ... (Indices for other faces)
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glGenBuffers(1, &texCoordVBO); // Tex coords will have their own VBO

        glBindVertexArray(VAO);

        // VBO for positions
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // VBO for tex coords
        glBindBuffer(GL_ARRAY_BUFFER, texCoordVBO); // Bind the texCoordVBO
        glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW); // Upload texCoord data
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0); // Set attribute pointer for location 1
        glEnableVertexAttribArray(1); // Enable attribute location 1

        // EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

// Renders this single block instance
void Block::render(const glm::mat4& projection, const glm::mat4& view) {
    if (VAO == 0) init(); // Ensure initialized
    if (VAO == 0 || Block::shaderProgram == 0) return; // Initialization failed?

    useBlockShader();
    bindBlockTexture(); // Bind texture if it has one
    
    // Calculate model matrix specific to this block instance
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    
    setShaderUniforms(projection, view, model);

    // Draw the single block (using its own simple VAO)
    glBindVertexArray(VAO);
    // TODO: Adjust index count based on actual VAO setup in init()
    glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0); // Assuming 2 faces for example
    glBindVertexArray(0);
    Texture::unbind();
}

bool Block::loadTexture(const std::string& filepath) {
    std::cout << "Loading texture from: " << filepath << std::endl;
    
    // Always check if the file exists first
    if (!std::filesystem::exists(filepath)) {
        std::cerr << "ERROR: Texture file does not exist: " << filepath << std::endl;
        std::cerr << "Current working directory: " << std::filesystem::current_path() << std::endl;
        return false;
    }
    
    bool success = texture.loadFromFile(filepath);
    hasTexture = success;
    
    if (success) {
        std::cout << "Successfully loaded texture with ID: " << texture.getID() << std::endl;
    } else {
        std::cerr << "Failed to load texture from: " << filepath << std::endl;
    }
    
    return success;
}

bool Block::loadTexture(const std::string& spritesheetPath, int atlasX, int atlasY, int atlasWidth, int atlasHeight) {
    bool success = texture.loadFromSpritesheet(spritesheetPath, atlasX, atlasY, atlasWidth, atlasHeight);
    hasTexture = success;
    return success;
}

// Share texture AND shader program ID from another block
void Block::shareTextureAndShaderFrom(const Block& other) {
    texture = other.texture; // Texture assignment operator handles sharing
    hasTexture = other.hasTextureState(); 
    // shaderProgram = other.getShaderProgram(); // No longer needed to copy shaderProgram, it's static
}

void Block::move(const glm::vec3& offset, float deltaTime) {
    position += offset * speed * deltaTime;
}

void Block::setPosition(const glm::vec3& newPosition) {
    position = newPosition;
}

glm::vec3 Block::getPosition() const {
    return position;
}

glm::vec3 Block::getColor() const {
    return color;
}

// Activate the shader program for this block type
void Block::useBlockShader() const {
    if (Block::shaderProgram != 0) {
        glUseProgram(Block::shaderProgram);
    } // No warning here, expected that shader might be 0 if not initialized
}

// Bind the texture for this block type
void Block::bindBlockTexture() const {
    if (hasTexture) {
        texture.bind(0); // Bind to texture unit 0
    }
}

// Set common shader uniforms (projection, view, model)
void Block::setShaderUniforms(const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model) const {
     if (Block::shaderProgram == 0) {
         // This might happen if init() wasn't called or failed
         // std::cerr << "Warning: setShaderUniforms called with uninitialized shader." << std::endl;
         return;
     }
     
     glUniformMatrix4fv(glGetUniformLocation(Block::shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
     glUniformMatrix4fv(glGetUniformLocation(Block::shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
     glUniformMatrix4fv(glGetUniformLocation(Block::shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

     // Texture related uniforms
     glUniform1i(glGetUniformLocation(Block::shaderProgram, "useTexture"), hasTexture);
     if (hasTexture) {
         glUniform1i(glGetUniformLocation(Block::shaderProgram, "blockTexture"), 0); // Texture unit 0
     } else {
         // Pass the block's color if not using texture
         glUniform3fv(glGetUniformLocation(Block::shaderProgram, "blockColor"), 1, glm::value_ptr(color));
     }
}

// New static method to initialize the global spritesheet
void Block::InitSpritesheet(const std::string& path) {
    if (Block::spritesheetLoaded) {
        // std::cout << "Global spritesheet already loaded." << std::endl; // Keep console clean
        return;
    }
    if (Block::spritesheetTexture.loadFromFile(path)) {
        std::cout << "Successfully loaded global spritesheet: " << path << " with ID: " << Block::spritesheetTexture.getID() << std::endl;
        Block::spritesheetLoaded = true;
    } else {
        std::cerr << "ERROR: Failed to load global spritesheet: " << path << std::endl;
        Block::spritesheetLoaded = false; 
    }
}
