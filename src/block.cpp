#include "../headers/block.h"
#include <iostream>

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
    
    uniform vec3 blockColor;
    uniform sampler2D blockTexture;
    uniform bool useTexture;
    
    void main() {
        if (useTexture) {
            FragColor = texture(blockTexture, TexCoord);
        } else {
            // Minecraft-like grass block color
            vec3 grassColor = vec3(0.333, 0.549, 0.333);
            FragColor = vec4(grassColor, 1.0);
        }
    }
)";

Block::Block(const glm::vec3& position, const glm::vec3& color, float size)
    : position(position), color(color), size(size), speed(0.05f), 
      VAO(0), VBO(0), EBO(0), texCoordVBO(0), shaderProgram(0), hasTexture(false) {
}

Block::~Block() {
    // Clean up OpenGL objects
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &texCoordVBO);
    glDeleteProgram(shaderProgram);
}

void Block::init() {
    // Compile shaders
    shaderProgram = compileShader(vertexShaderSource, fragmentShaderSource);

    // Set up vertex data for a cube
    float halfSize = size / 2.0f;
    float vertices[] = {
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
    };

    // Texture coordinates
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
    };

    unsigned int indices[] = {
        // Front face
        0, 1, 2, 2, 3, 0,
        // Right face
        1, 5, 6, 6, 2, 1,
        // Back face
        5, 4, 7, 7, 6, 5,
        // Left face
        4, 0, 3, 3, 7, 4,
        // Top face
        3, 2, 6, 6, 7, 3,
        // Bottom face
        4, 5, 1, 1, 0, 4
    };

    // Create VAO, VBO, EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &texCoordVBO);

    // Bind VAO
    glBindVertexArray(VAO);

    // Bind and set VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Bind and set texture coordinate VBO
    glBindBuffer(GL_ARRAY_BUFFER, texCoordVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    // Bind and set EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Block::render(const glm::mat4& projection, const glm::mat4& view) {
    // Use shader program
    glUseProgram(shaderProgram);

    // Set uniforms
    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "blockColor");
    unsigned int useTextureLoc = glGetUniformLocation(shaderProgram, "useTexture");

    // Create model matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);

    // Set uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    glUniform3fv(colorLoc, 1, &color[0]);
    
    // Set texture uniforms
    if (hasTexture) {
        glUniform1i(useTextureLoc, GL_TRUE);
        texture.bind(0); // Bind to texture unit 0
        glUniform1i(glGetUniformLocation(shaderProgram, "blockTexture"), 0);
    } else {
        glUniform1i(useTextureLoc, GL_FALSE);
    }

    // Draw cube
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    
    // Unbind texture and VAO
    if (hasTexture) {
        Texture::unbind();
    }
    glBindVertexArray(0);
}

bool Block::loadTexture(const std::string& texturePath) {
    if (texture.loadFromFile(texturePath)) {
        hasTexture = true;
        return true;
    }
    std::cerr << "Using default grass block color instead of texture." << std::endl;
    hasTexture = false;
    return false;
}

void Block::move(float dx, float dy, float dz) {
    position.x += dx * speed;
    position.y += dy * speed;
    position.z += dz * speed;
}

void Block::setPosition(const glm::vec3& newPosition) {
    position = newPosition;
}

glm::vec3 Block::getPosition() const {
    return position;
}

unsigned int Block::compileShader(const char* vertexShaderSource, const char* fragmentShaderSource) {
    // Compile vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Check for vertex shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Compile fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Check for fragment shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // Delete shaders as they're linked into the program
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}
