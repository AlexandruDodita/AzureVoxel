#include "../headers/chunk.h"
#include "../headers/world.h" // Include World header for neighbor checks
#include <iostream>
#include <memory>
#include <vector>
#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr
#include <cmath> // For std::abs in noise generation
#include <fstream> // For file I/O
#include <sys/stat.h> // For directory creation (Unix-like systems)
#include <sstream> // For ostringstream
#include <chrono> // For timing
#include <thread> // For std::this_thread
#include <GLFW/glfw3.h> // Required for glfwGetCurrentContext

// --- Vertex data for a single block face ---
// Order: Position (3 floats), Texture Coords (2 floats)
// We define faces relative to block center (0,0,0), size 1.0

// Vertex positions (relative to block center)
const glm::vec3 faceVertices[][4] = {
    // Back (-Z)
    { {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f} },
    // Front (+Z)
    { {-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f} },
    // Left (-X)
    { {-0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f,  0.5f} },
    // Right (+X)
    { { 0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f} },
    // Bottom (-Y)
    { {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f} },
    // Top (+Y)
    { {-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f} }
};

// Texture coordinates (standard square)
const glm::vec2 texCoords[] = {
    {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
};

Chunk::Chunk(const glm::vec3& position) : position(position), needsRebuild(true), isInitialized_(false) {
    // Initialize blockDataForInitialization_ with default BlockInfo (air)
    blockDataForInitialization_.resize(CHUNK_SIZE_X,
                                 std::vector<std::vector<BlockInfo>>(
                                     CHUNK_SIZE_Y,
                                     std::vector<BlockInfo>(CHUNK_SIZE_Z)));
    // Initialize blocks_ with nullptr
    blocks_.resize(CHUNK_SIZE_X, 
                 std::vector<std::vector<std::shared_ptr<Block>>>(
                     CHUNK_SIZE_Y,
                     std::vector<std::shared_ptr<Block>>(CHUNK_SIZE_Z, nullptr)));
}

Chunk::~Chunk() {
    cleanupMesh();
}

void Chunk::init(const World* world) {
    // This method's role changes. The primary initialization including
    // terrain generation/loading and mesh building will be handled by ensureInitialized.
    // For now, we can leave it empty or for very minimal, non-conditional setup if any.
    // The world object will call ensureInitialized with appropriate parameters.
}

// Simple noise function (replace with a proper Perlin/Simplex noise later)
float simpleNoise(int x, int y, int z, int seed) {
    // Combine inputs and use a simple hashing-like approach
    // This is NOT a good noise function, just a placeholder
    int h = x * 374761393 + y * 668265263 + z * 104729 + seed;
    h = (h ^ (h >> 13)) * 1274126177;
    return static_cast<float>((h ^ (h >> 16)) & 0x7fffffff) / static_cast<float>(0x7fffffff);
}

// --- generateTerrain_DataOnly, loadFromFile_DataOnly, openglInitialize are implemented above ---

// --- Ensure old ensureInitialized, prepareBlockData, generateTerrain, loadFromFile are removed or commented out ---

/*
// Old ensureInitialized - REMOVE/COMMENT OUT
void Chunk::ensureInitialized(World* world, int seed, const std::string& worldDataPath) {
    // ... old implementation ...
}
*/

/*
// Old prepareBlockData - REMOVE/COMMENT OUT
void Chunk::prepareBlockData(World* world, int seed, const std::string& worldDataPath) {
    // ... old implementation ...
}
*/

/* 
// Old generateTerrain - REMOVE/COMMENT OUT
void Chunk::generateTerrain(int seed) { 
    // ... old implementation ...
}
*/

/*
// Old loadFromFile - REMOVE/COMMENT OUT
bool Chunk::loadFromFile(const std::string& directoryPath, const World* world) {
    // ... old implementation ...
}
*/

// Builds a single mesh containing only the visible faces of the blocks in this chunk.
void Chunk::buildSurfaceMesh(const World* world) {
    // First, clear any existing mesh data
    cleanupMesh(); // This resets surfaceMesh.VAO to 0

    // Reserve memory based on estimated size to prevent reallocations
    const size_t estimatedFaces = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * 6 / 4;
    meshVertices.reserve(estimatedFaces * 4 * 5); // 4 vertices per face, 5 floats per vertex
    meshIndices.reserve(estimatedFaces * 6); // 6 indices per face
    
    unsigned int vertexIndexOffset = 0;

    // Define neighbor offsets for checking each face
    const int neighborOffsets[6][3] = {
        { 0,  0, -1}, // Back
        { 0,  0,  1}, // Front
        {-1,  0,  0}, // Left
        { 1,  0,  0}, // Right
        { 0, -1,  0}, // Bottom
        { 0,  1,  0}  // Top
    };

    // Loop through all block positions
    for (int x = 0; x < CHUNK_SIZE_X; ++x) {
        for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
            for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
                if (!hasBlockAtLocal(x, y, z)) {
                    continue; // Skip empty positions
                }

                // Check each face of the block
                for (int face = 0; face < 6; ++face) {
                    int nx = x + neighborOffsets[face][0];
                    int ny = y + neighborOffsets[face][1];
                    int nz = z + neighborOffsets[face][2];

                    // Check if neighbor is outside chunk borders
                    bool shouldRenderFace = false;
                    if (nx < 0 || nx >= CHUNK_SIZE_X || ny < 0 || ny >= CHUNK_SIZE_Y || nz < 0 || nz >= CHUNK_SIZE_Z) {
                        // If neighbor is in adjacent chunk, check that chunk
                        glm::ivec3 neighborWorldPos = glm::ivec3(position) + glm::ivec3(x, y, z) + glm::ivec3(neighborOffsets[face][0], neighborOffsets[face][1], neighborOffsets[face][2]);
                        glm::ivec2 neighborChunkPos = world->worldToChunkCoords(glm::vec3(neighborWorldPos));
                        
                        // Get the adjacent chunk (if any)
                        auto adjChunk = const_cast<World*>(world)->getChunkAt(neighborChunkPos.x, neighborChunkPos.y);
                        if (!adjChunk) {
                            shouldRenderFace = true; // No adjacent chunk, so render this face
                        } else {
                            // Convert world position to local position in adjacent chunk
                            glm::ivec3 localBlockPos = neighborWorldPos - glm::ivec3(adjChunk->getPosition());
                            shouldRenderFace = !adjChunk->hasBlockAtLocal(localBlockPos.x, localBlockPos.y, localBlockPos.z);
                        }
                    } else {
                        // Neighbor is within this chunk
                        shouldRenderFace = !hasBlockAtLocal(nx, ny, nz);
                    }

                    if (shouldRenderFace) {
                        // Define vertices for this face (centered at block position)
                        for (int i = 0; i < 4; ++i) {
                            float vx = x + faceVertices[face][i][0];
                            float vy = y + faceVertices[face][i][1];
                            float vz = z + faceVertices[face][i][2];
                            
                            // Add vertex position (3 floats)
                            meshVertices.push_back(vx);
                            meshVertices.push_back(vy);
                            meshVertices.push_back(vz);
                            
                            // Add texture coordinates (2 floats)
                            meshVertices.push_back(texCoords[i][0]);
                            meshVertices.push_back(texCoords[i][1]);
                        }
                        
                        // Add indices for the face (two triangles)
                        meshIndices.push_back(vertexIndexOffset);
                        meshIndices.push_back(vertexIndexOffset + 1);
                        meshIndices.push_back(vertexIndexOffset + 2);
                        meshIndices.push_back(vertexIndexOffset + 2);
                        meshIndices.push_back(vertexIndexOffset + 3);
                        meshIndices.push_back(vertexIndexOffset);
                        
                        vertexIndexOffset += 4;
                    }
                }
            }
        }
    }

    // Early exit if no vertices to render
    if (meshVertices.empty()) {
        std::cout << "No faces to render for chunk at " << position.x << "," << position.z << std::endl;
        surfaceMesh.indexCount = 0;
        surfaceMesh.VAO = 0;
        needsRebuild = false;
        return;
    }

    // Create placeholder blocks if we don't have any yet to ensure shaders are initialized
    if (Block::shaderProgram == 0) {
        std::cout << "WARNING: Block shader not initialized yet. Initializing now." << std::endl;
        Block::InitBlockShader();
    }

    // Force-clear any existing OpenGL errors before we start
    while (glGetError() != GL_NO_ERROR) {}

    // Create OpenGL resources
    GLuint vao = 0, vbo = 0, ebo = 0;
    
    // Check OpenGL context
    if (glfwGetCurrentContext() == nullptr) {
        std::cerr << "CRITICAL ERROR: No OpenGL context is current during mesh building!" << std::endl;
        return;
    }
    
    // IMPORTANT: Force synchronization with the driver before creating resources
    glFinish();
    
    // Create vertex array object first
    glGenVertexArrays(1, &vao);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR || vao == 0) {
        std::cerr << "CRITICAL ERROR: Failed to generate VAO for chunk at " << position.x << "," << position.z << std::endl;
        std::cerr << "OpenGL error after glGenVertexArrays: " << error << std::endl;
        
        // Additional debugging info
        std::cout << "DEBUG: OpenGL context: " << glfwGetCurrentContext() << std::endl;
        std::cout << "DEBUG: OpenGL version: " << glGetString(GL_VERSION) << std::endl;
        std::cout << "DEBUG: GLX_MESA_query_renderer: " << (glGetStringi != nullptr ? "supported" : "not supported") << std::endl;
        
        // Try a second time after a short delay
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Retrying VAO generation..." << std::endl;
        glGenVertexArrays(1, &vao);
        error = glGetError();
        if (error != GL_NO_ERROR || vao == 0) {
            std::cerr << "CRITICAL ERROR: Retry failed. VAO generation still failed with error: " << error << std::endl;
            return;
        } else {
            std::cout << "Retry successful! VAO: " << vao << std::endl;
        }
    }
    
    // Bind VAO first to capture all subsequent bindings
    glBindVertexArray(vao);
    
    // Create and bind vertex buffer
    glGenBuffers(1, &vbo);
    error = glGetError();
    if (error != GL_NO_ERROR || vbo == 0) {
        std::cerr << "ERROR: Failed to generate VBO." << std::endl;
        glDeleteVertexArrays(1, &vao);
        return;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(float), meshVertices.data(), GL_STATIC_DRAW);
    
    // Create and bind element buffer
    glGenBuffers(1, &ebo);
    error = glGetError();
    if (error != GL_NO_ERROR || ebo == 0) {
        std::cerr << "ERROR: Failed to generate EBO." << std::endl;
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        return;
    }
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshIndices.size() * sizeof(unsigned int), meshIndices.data(), GL_STATIC_DRAW);
    
    // Set vertex attributes while VAO is bound
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Unbind VAO to prevent accidental modification
    glBindVertexArray(0);
    
    // Force synchronization again to ensure commands are processed
    glFinish();
    
    // Check for any OpenGL errors
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error after creating mesh resources: " << error << std::endl;
        glDeleteBuffers(1, &ebo);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        return;
    }
    
    // Store mesh data for rendering
    surfaceMesh.VAO = vao;
    surfaceMesh.VBO = vbo;
    surfaceMesh.EBO = ebo;
    surfaceMesh.indexCount = meshIndices.size();
    
    std::cout << "Successfully built mesh for chunk at " << position.x << "," << position.z 
              << " with " << meshIndices.size() / 6 << " faces, VAO=" << surfaceMesh.VAO << std::endl;
    
    // Mark as no longer needing rebuild
    needsRebuild = false;
}

// Render the pre-built surface mesh using a single draw call
void Chunk::renderSurface(const glm::mat4& projection, const glm::mat4& view) const {
    if (surfaceMesh.indexCount == 0 || surfaceMesh.VAO == 0) {
        return; 
    }

    // Use the static block shader program
    if (Block::shaderProgram == 0) {
        std::cerr << "ERROR: Block shader program is not initialized. Cannot render chunk." << std::endl;
        return;
    }
    
    // Force-clear any existing OpenGL errors before rendering
    while (glGetError() != GL_NO_ERROR) {}
    
    // Use shader program
    glUseProgram(Block::shaderProgram);
    
    // Create model matrix for this chunk's position
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    
    // Set uniforms (model, view, projection)
    GLint modelLoc = glGetUniformLocation(Block::shaderProgram, "model");
    if (modelLoc != -1) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    }
    
    GLint viewLoc = glGetUniformLocation(Block::shaderProgram, "view");
    if (viewLoc != -1) {
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }
    
    GLint projLoc = glGetUniformLocation(Block::shaderProgram, "projection");
    if (projLoc != -1) {
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    
    // Set default color since we're having texture issues
    GLint blockColorLoc = glGetUniformLocation(Block::shaderProgram, "blockColor");
    if (blockColorLoc != -1) {
        // Use a different color for each chunk to make them distinct
        //float r = (static_cast<int>(position.x) * 16) % 255 / 255.0f;
        //float g = (static_cast<int>(position.z) * 16) % 255 / 255.0f;
        //float b = 0.6f; // Keep blue consistent for a better look
        glUniform3f(blockColorLoc, 1.0f, 0.0f, 0.0f); // Bright Red
    }
    
    // Disable textures for now until we fix texture issues
    GLint useTextureLoc = glGetUniformLocation(Block::shaderProgram, "useTexture");
    if (useTextureLoc != -1) {
        glUniform1i(useTextureLoc, 0); // false - don't use textures yet
    }
    
    // Bind VAO and draw
    glBindVertexArray(surfaceMesh.VAO);
    glDrawElements(GL_TRIANGLES, surfaceMesh.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Check for OpenGL errors after rendering
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error after rendering chunk at " << position.x << "," << position.z << ": " << error << std::endl;
    }
}

// Render all blocks individually (slow, use only for the current chunk)
void Chunk::renderAllBlocks(const glm::mat4& projection, const glm::mat4& view) {
    for (int x = 0; x < CHUNK_SIZE_X; ++x) {
        for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
            for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
                std::shared_ptr<Block> block = getBlockAtLocal(x, y, z);
                if (block) {
                    // Use block's individual render method
                    block->render(projection, view); 
                }
            }
        }
    }
}

// Renamed from hasBlockAt
bool Chunk::hasBlockAtLocal(int x, int y, int z) const {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return false;
    }
    return blocks_[x][y][z] != nullptr;
}

// Renamed from getBlockAt
std::shared_ptr<Block> Chunk::getBlockAtLocal(int x, int y, int z) const {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return nullptr;
    }
    return blocks_[x][y][z];
}

// Renamed from setBlockAt
void Chunk::setBlockAtLocal(int x, int y, int z, std::shared_ptr<Block> block) {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return;
    }
    if ((blocks_[x][y][z] == nullptr && block != nullptr) || (blocks_[x][y][z] != nullptr && block == nullptr)) {
        needsRebuild = true;
    }
    blocks_[x][y][z] = block;
    // Update blockDataForInitialization_ based on whether a block is present
    if (block != nullptr) {
        // For simplicity, if a block is added manually, default its type to 1 (e.g., stone).
        // A more advanced system would identify the block type more accurately.
        blockDataForInitialization_[x][y][z].type = 1; 
    } else {
        blockDataForInitialization_[x][y][z].type = 0; // Air
    }
}

// Renamed from removeBlockAt
void Chunk::removeBlockAtLocal(int x, int y, int z) {
     if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return;
    }
    if (blocks_[x][y][z] != nullptr) {
        blocks_[x][y][z] = nullptr;
        blockDataForInitialization_[x][y][z].type = 0; // Air
        needsRebuild = true;
    }
}

void Chunk::cleanupMesh() {
    if (surfaceMesh.VAO != 0) {
        glDeleteVertexArrays(1, &surfaceMesh.VAO);
        surfaceMesh.VAO = 0;
    }
    if (surfaceMesh.VBO != 0) {
        glDeleteBuffers(1, &surfaceMesh.VBO);
        surfaceMesh.VBO = 0;
    }
    if (surfaceMesh.EBO != 0) {
        glDeleteBuffers(1, &surfaceMesh.EBO);
        surfaceMesh.EBO = 0;
    }
    surfaceMesh.indexCount = 0;
    
    // Clear the vertex and index data to free memory
    meshVertices.clear();
    meshIndices.clear();
    
    // Force vector capacity to be reduced
    std::vector<float>().swap(meshVertices);
    std::vector<unsigned int>().swap(meshIndices);
}

glm::vec3 Chunk::getPosition() const {
    return position;
}

std::string Chunk::getChunkFileName() const {
    std::ostringstream oss;
    oss << "chunk_" << static_cast<int>(position.x) << "_" 
        << static_cast<int>(position.y) << "_" 
        << static_cast<int>(position.z) << ".chunk";
    return oss.str();
}

bool Chunk::saveToFile(const std::string& directoryPath) const {
    mkdir(directoryPath.c_str(), 0755);
    std::string filePath = directoryPath + "/" + getChunkFileName();
    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open file for saving chunk: " << filePath << std::endl;
        return false;
    }

    for (int x = 0; x < CHUNK_SIZE_X; ++x) {
        for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
            for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
                // Save only the type from blockDataForInitialization_
                outFile.write(reinterpret_cast<const char*>(&blockDataForInitialization_[x][y][z].type), sizeof(BlockInfo::type));
            }
        }
    }
    // std::cout << "Saved chunk data (types) to: " << filePath << std::endl; // Can be noisy
    return true;
}

bool Chunk::loadFromFile_DataOnly(const std::string& directoryPath, World* world) {
    std::string filePath = directoryPath + "/" + getChunkFileName();
    struct stat buffer;
    if (stat(filePath.c_str(), &buffer) != 0) return false; // File doesn't exist
    
    std::ifstream inFile(filePath, std::ios::binary | std::ios::ate); // Open at end to get size
    if (!inFile.is_open()) {
        std::cerr << "Error: Could not open chunk file for reading: " << filePath << std::endl;
        return false;
    }
    std::streamsize size = inFile.tellg();
    inFile.seekg(0, std::ios::beg); // Go back to the beginning

    constexpr size_t expected_size_per_block = sizeof(BlockInfo::type); // We save only type for now
    constexpr size_t total_expected_bytes = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * expected_size_per_block;

    if (size != total_expected_bytes) {
        std::cerr << "Error: File size mismatch for chunk " << filePath 
                  << ". Expected " << total_expected_bytes << " bytes, got " << size << std::endl;
        return false;
    }

    std::cout << "DataOnly: Loading chunk from file: " << filePath << std::endl;
    
    for (int x = 0; x < CHUNK_SIZE_X; ++x) {
        for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
            for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
                // Read only the type for now
                inFile.read(reinterpret_cast<char*>(&blockDataForInitialization_[x][y][z].type), sizeof(BlockInfo::type));
                if (inFile.fail()) {
                    std::cerr << "Error reading block data for chunk " << filePath << std::endl;
                    return false;
                }
            }
        }
    }
    needsRebuild = true;
    return true; 
}

void Chunk::ensureInitialized(World* world, int seed, const std::string& worldDataPath) {
    if (isInitialized_) {
        return;
    }
    // Stage 1: Prepare block data (non-OpenGL part, can run on worker thread)
    bool wasLoaded = loadFromFile_DataOnly(worldDataPath, world);
    if (!wasLoaded) {
        generateTerrain_DataOnly(seed);
    }

    // Stage 2: Queue OpenGL-dependent initialization to the main thread
    std::shared_ptr<Chunk> self = shared_from_this();
    world->addMainThreadTask([self, world, seed, worldDataPath]() { // Pass necessary params
        self->openglInitialize(world); // openglInitialize will handle its own data loading/generation if needed
    });
    // isInitialized_ will be set true at the end of openglInitialize
}

bool Chunk::isInitialized() const {
    return isInitialized_;
}

// New: Data-only terrain generation (placeholder implementation)
void Chunk::generateTerrain_DataOnly(int seed) {
    std::cout << "DataOnly: Generating terrain for chunk at " << position.x << "," << position.z << std::endl;
    std::vector<int> heightMap(CHUNK_SIZE_X * CHUNK_SIZE_Z);
    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int z = 0; z < CHUNK_SIZE_Z; z++) {
            int worldX = static_cast<int>(position.x) + x;
            int worldZ = static_cast<int>(position.z) + z;
            float heightNoise = simpleNoise(worldX, 0, worldZ, seed);
            int terrainHeight = static_cast<int>(CHUNK_SIZE_Y / 2 + heightNoise * (CHUNK_SIZE_Y / 4));
            terrainHeight = std::max(1, std::min(CHUNK_SIZE_Y - 1, terrainHeight));
            heightMap[x * CHUNK_SIZE_Z + z] = terrainHeight;
        }
    }

    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int z = 0; z < CHUNK_SIZE_Z; z++) {
            int terrainHeight = heightMap[x * CHUNK_SIZE_Z + z];
            for (int y = 0; y < CHUNK_SIZE_Y; y++) {
                if (y < terrainHeight - 1) { // Stone
                    blockDataForInitialization_[x][y][z].type = 1; // 1 for stone
                } else if (y == terrainHeight - 1) { // Grass
                    blockDataForInitialization_[x][y][z].type = 2; // 2 for grass
                } else { // Air
                    blockDataForInitialization_[x][y][z].type = 0; // 0 for air
                }
            }
        }
    }
    needsRebuild = true;
}

// New: OpenGL-dependent initialization (runs on main thread - placeholder implementation)
void Chunk::openglInitialize(World* world) {
    if (isInitialized_) return;

    // Ensure an OpenGL context is current on THIS thread
    if (glfwGetCurrentContext() == nullptr) {
        std::cerr << "CRITICAL ERROR: No OpenGL context current on main thread for openglInitialize! Chunk: " 
                  << position.x << "," << position.z << std::endl;
        return; // Cannot proceed
    }

    std::cout << "MainThread: OpenGL-Initializing chunk at " << position.x << "," << position.z << std::endl;
    
    if (Block::shaderProgram == 0) {
        std::cout << "MainThread: Initializing block shader program..." << std::endl;
        Block::InitBlockShader();
        if (Block::shaderProgram == 0) {
            std::cerr << "CRITICAL ERROR: Failed to initialize block shader program! Cannot initialize chunk." << std::endl;
            return;
        }
    }

    // Create template blocks for textures (OpenGL calls, must be on main thread)
    Block stoneBlockTemplate(glm::vec3(0.0f), glm::vec3(0.5f), 1.0f);
    if (!stoneBlockTemplate.loadTexture("res/textures/Spritesheet.PNG", 0, 0, 16, 16)) {
        std::cerr << "Warning: Failed to load stone texture for chunk " << position.x << "," << position.z << std::endl;
        // Continue without texture, or handle error as critical
    }

    Block grassBlockTemplate(glm::vec3(0.0f), glm::vec3(0.2f, 0.8f, 0.2f), 1.0f);
    grassBlockTemplate.shareTextureAndShaderFrom(stoneBlockTemplate); // Assumes stoneBlockTemplate shader is valid
    if (!grassBlockTemplate.loadTexture("res/textures/Spritesheet.PNG", 16, 0, 16, 16)) {
        std::cerr << "Warning: Failed to load grass texture for chunk " << position.x << "," << position.z << std::endl;
    }

    // Populate `this->blocks_` using `this->blockDataForInitialization_`
    for (int x = 0; x < CHUNK_SIZE_X; ++x) {
        for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
            for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
                BlockInfo info = blockDataForInitialization_[x][y][z];
                glm::vec3 blockWorldPos = this->position + glm::vec3(x, y, z);
                
                if (info.type == 1) { // Stone
                    auto block = std::make_shared<Block>(blockWorldPos, stoneBlockTemplate.getColor(), 1.0f);
                    block->shareTextureAndShaderFrom(stoneBlockTemplate);
                    this->blocks_[x][y][z] = block;
                } else if (info.type == 2) { // Grass
                    auto block = std::make_shared<Block>(blockWorldPos, grassBlockTemplate.getColor(), 1.0f);
                    block->shareTextureAndShaderFrom(grassBlockTemplate);
                    this->blocks_[x][y][z] = block;
                } else { // Air (type 0 or other)
                    this->blocks_[x][y][z] = nullptr;
                }
            }
        }
    }

    if (needsRebuild) {
        std::cout << "MainThread: Building surface mesh for chunk " << position.x << "," << position.z << std::endl;
        buildSurfaceMesh(world); 
    }
    
    // Save the fully initialized chunk state (block types) to disk AFTER successful GL init
    // This ensures that if GL init fails, we don't save corrupted state.
    saveToFile(world->getWorldDataPath()); 

    isInitialized_ = true;
    needsRebuild = false; // Mesh has been built (or attempted)
    std::cout << "Chunk at " << position.x << "," << position.z << " OpenGL-initialized. VAO=" << surfaceMesh.VAO << std::endl;
}


// ... (buildSurfaceMesh, renderSurface, and other existing methods) ...
// Remember to make renderSurface const as per header file.

// Original generateTerrain() and loadFromFile() should be removed or fully commented out
// to avoid confusion with the _DataOnly versions.

/* Commenting out old generateTerrain
void Chunk::generateTerrain(int seed) { ... old implementation ... }
*/

/* Commenting out old loadFromFile
bool Chunk::loadFromFile(const std::string& directoryPath, const World* world) { ... old implementation ... }
*/



