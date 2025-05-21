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
    // Initialize the 3D vector of blocks with nullptr
    blocks.resize(CHUNK_SIZE_X, 
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

void Chunk::generateTerrain(int seed) {
    // Create and initialize representative blocks for the two most common block types
    // This avoids repeated texture loading and shader compilation
    Block stoneBlock(glm::vec3(0.0f), glm::vec3(0.8f), 1.0f);
    stoneBlock.init();
    stoneBlock.loadTexture("res/textures/Spritesheet.PNG", 0, 0, 16, 16); // Stone texture
    
    Block grassBlock(glm::vec3(0.0f), glm::vec3(0.8f), 1.0f);
    grassBlock.shareTextureAndShaderFrom(stoneBlock); // Share shader - avoid recompilation
    grassBlock.loadTexture("res/textures/Spritesheet.PNG", 16, 0, 16, 16); // Grass texture

    // Pre-allocate memory for noise calculations to avoid reallocation
    std::vector<int> heightMap(CHUNK_SIZE_X * CHUNK_SIZE_Z);
    
    // Step 1: Calculate heightmap first (separated to improve memory locality)
    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int z = 0; z < CHUNK_SIZE_Z; z++) {
            // Get world coordinates for noise calculation
            int worldX = static_cast<int>(position.x) + x;
            int worldZ = static_cast<int>(position.z) + z;

            // Calculate and store terrain height for this x,z column
            float heightNoise = simpleNoise(worldX, 0, worldZ, seed);
            int terrainHeight = static_cast<int>(CHUNK_SIZE_Y / 2 + heightNoise * (CHUNK_SIZE_Y / 4));
            terrainHeight = std::max(1, std::min(CHUNK_SIZE_Y - 1, terrainHeight));
            heightMap[x * CHUNK_SIZE_Z + z] = terrainHeight;
        }
    }

    // Step 2: Create blocks (now with better memory locality and less branching)
    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int z = 0; z < CHUNK_SIZE_Z; z++) {
            int terrainHeight = heightMap[x * CHUNK_SIZE_Z + z];
            
            // Set all blocks above terrain height to air (nullptr)
            for (int y = terrainHeight; y < CHUNK_SIZE_Y; y++) {
                blocks[x][y][z] = nullptr;
            }
            
            // Set surface block (grass)
            if (terrainHeight > 0) {
                int y = terrainHeight - 1;
                glm::vec3 blockWorldPos = position + glm::vec3(x, y, z);
                auto block = std::make_shared<Block>(blockWorldPos, glm::vec3(0.5f), 1.0f);
                block->shareTextureAndShaderFrom(grassBlock);
                blocks[x][y][z] = block;
                
                // Set underground blocks (stone) - all in one loop with shared properties
                for (y = 0; y < terrainHeight - 1; y++) {
                    blockWorldPos = position + glm::vec3(x, y, z);
                    block = std::make_shared<Block>(blockWorldPos, glm::vec3(0.5f), 1.0f);
                    block->shareTextureAndShaderFrom(stoneBlock);
                    blocks[x][y][z] = block;
                }
            }
        }
    }
    
    needsRebuild = true;
}

// Builds a single mesh containing only the visible faces of the blocks in this chunk.
void Chunk::buildSurfaceMesh(const World* world) {
    cleanupMesh(); // Clear old mesh data if rebuilding

    // Reserve memory based on estimated size to prevent reallocations
    // Assuming ~25% of potential faces are visible (conservative estimate)
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

    // Iterate in the optimal order for memory access (x,z,y instead of y,x,z)
    for (int x = 0; x < CHUNK_SIZE_X; ++x) {
        for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
            for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
                if (!hasBlockAtLocal(x, y, z)) continue; // Skip air blocks

                // Calculate the absolute world position of the current block
                int worldX = static_cast<int>(position.x) + x;
                int worldY = static_cast<int>(position.y) + y;
                int worldZ = static_cast<int>(position.z) + z;

                // Check all 6 neighbors
                for (int face = 0; face < 6; ++face) {
                    // First check if the neighbor is within this chunk - faster than world query
                    int nx = x + neighborOffsets[face][0];
                    int ny = y + neighborOffsets[face][1];
                    int nz = z + neighborOffsets[face][2];
                    
                    bool isNeighborAir = false;
                    
                    if (nx >= 0 && nx < CHUNK_SIZE_X && 
                        ny >= 0 && ny < CHUNK_SIZE_Y && 
                        nz >= 0 && nz < CHUNK_SIZE_Z) {
                        // Neighbor is within this chunk, use direct access
                        isNeighborAir = !hasBlockAtLocal(nx, ny, nz);
                    } else {
                        // Neighbor is in adjacent chunk, use world query
                        int neighborWorldX = worldX + neighborOffsets[face][0];
                        int neighborWorldY = worldY + neighborOffsets[face][1];
                        int neighborWorldZ = worldZ + neighborOffsets[face][2];
                        isNeighborAir = !world || !world->getBlockAtWorldPos(neighborWorldX, neighborWorldY, neighborWorldZ);
                    }
                    
                    if (isNeighborAir) {
                        // Neighbor is air or outside loaded world, so this face is visible
                        // Add the 4 vertices for this face
                        for (int i = 0; i < 4; ++i) {
                            // Vertex position (local to chunk + offset within block)
                            meshVertices.push_back(static_cast<float>(x) + faceVertices[face][i].x);
                            meshVertices.push_back(static_cast<float>(y) + faceVertices[face][i].y);
                            meshVertices.push_back(static_cast<float>(z) + faceVertices[face][i].z);
                            // Texture coordinate
                            meshVertices.push_back(texCoords[i].x);
                            meshVertices.push_back(texCoords[i].y);
                        }
                        // Add the 6 indices for the 2 triangles of this face
                        meshIndices.push_back(vertexIndexOffset + 0);
                        meshIndices.push_back(vertexIndexOffset + 1);
                        meshIndices.push_back(vertexIndexOffset + 2);
                        meshIndices.push_back(vertexIndexOffset + 2);
                        meshIndices.push_back(vertexIndexOffset + 3);
                        meshIndices.push_back(vertexIndexOffset + 0);
                        
                        vertexIndexOffset += 4; // Increment offset for next face
                    }
                }
            }
        }
    }

    if (meshVertices.empty()) {
        surfaceMesh.indexCount = 0;
        return; // No visible faces, nothing to create buffers for
    }

    // --- Create OpenGL Buffers for the surface mesh --- 
    glGenVertexArrays(1, &surfaceMesh.VAO);
    glGenBuffers(1, &surfaceMesh.VBO);
    glGenBuffers(1, &surfaceMesh.EBO);

    glBindVertexArray(surfaceMesh.VAO);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, surfaceMesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(float), meshVertices.data(), GL_STATIC_DRAW);

    // EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, surfaceMesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshIndices.size() * sizeof(unsigned int), meshIndices.data(), GL_STATIC_DRAW);

    // Vertex Attributes (position + texture coords)
    // Stride is 5 floats (3 pos + 2 tex)
    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Texture Coordinate attribute (location = 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind VBO
    glBindVertexArray(0); // Unbind VAO

    surfaceMesh.indexCount = static_cast<GLsizei>(meshIndices.size());
    needsRebuild = false; // Mark mesh as up-to-date
    
    // std::cout << "Built mesh for chunk (" << position.x << "," << position.z << ") with " 
    //           << surfaceMesh.indexCount / 6 << " faces." << std::endl;
}

// Render the pre-built surface mesh using a single draw call
void Chunk::renderSurface(const glm::mat4& projection, const glm::mat4& view) {
    if (surfaceMesh.indexCount == 0 || surfaceMesh.VAO == 0) return; // Nothing to render

    // Find *any* block to get shader and texture info (they are shared)
    std::shared_ptr<Block> representativeBlock = nullptr;
    bool foundBlock = false;
    for (int x = 0; x < CHUNK_SIZE_X && !foundBlock; ++x) {
         for (int y = 0; y < CHUNK_SIZE_Y && !foundBlock; ++y) {
             for (int z = 0; z < CHUNK_SIZE_Z && !foundBlock; ++z) {
                 representativeBlock = getBlockAtLocal(x, y, z);
                 if (representativeBlock) {
                     foundBlock = true;
                 }
             }
         }
    }

    if (!representativeBlock) return; 

    GLuint shaderProg = representativeBlock->getShaderProgram();
    if (shaderProg == 0) {
         // This shouldn't happen if generateTerrain worked correctly
         // std::cerr << "Warning: Representative block has uninitialized shader in renderSurface." << std::endl;
         return; 
    }
    glUseProgram(shaderProg);

    // Set uniforms
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    glUniformMatrix4fv(glGetUniformLocation(shaderProg, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProg, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProg, "projection"), 1, GL_FALSE, &projection[0][0]);

    // Bind texture if available
    if (representativeBlock->hasTextureState()) { // Use the getter
        glUniform1i(glGetUniformLocation(shaderProg, "useTexture"), GL_TRUE);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, representativeBlock->getTextureID()); // Use the getter
        glUniform1i(glGetUniformLocation(shaderProg, "blockTexture"), 0); 
    } else {
        glUniform1i(glGetUniformLocation(shaderProg, "useTexture"), GL_FALSE);
        // Set fallback color using representativeBlock->color
        glm::vec3 fallbackColor = representativeBlock->getColor();
        glUniform3fv(glGetUniformLocation(shaderProg, "blockColor"), 1, glm::value_ptr(fallbackColor));
    }
    
    // Draw the mesh
    glBindVertexArray(surfaceMesh.VAO);
    glDrawElements(GL_TRIANGLES, surfaceMesh.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
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
    return blocks[x][y][z] != nullptr;
}

// Renamed from getBlockAt
std::shared_ptr<Block> Chunk::getBlockAtLocal(int x, int y, int z) const {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return nullptr;
    }
    return blocks[x][y][z];
}

// Renamed from setBlockAt
void Chunk::setBlockAtLocal(int x, int y, int z, std::shared_ptr<Block> block) {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return;
    }
    // Only mark for rebuild if the block state actually changes (adding/removing)
    if ((blocks[x][y][z] == nullptr && block != nullptr) || (blocks[x][y][z] != nullptr && block == nullptr)) {
        needsRebuild = true; // Mark chunk for mesh rebuild
        // TODO: Also mark neighbors if block is on boundary
    }
    blocks[x][y][z] = block;
}

// Renamed from removeBlockAt
void Chunk::removeBlockAtLocal(int x, int y, int z) {
     if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return;
    }
    if (blocks[x][y][z] != nullptr) {
        blocks[x][y][z] = nullptr;
        needsRebuild = true; // Mark chunk for mesh rebuild
        // TODO: Also mark neighbors if block is on boundary
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
    // Ensure the directory exists
    mkdir(directoryPath.c_str(), 0755); // Create directory if it doesn't exist (Unix-like)
    // For Windows, you might need #include <direct.h> and _mkdir()

    std::string filePath = directoryPath + "/" + getChunkFileName();
    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open file for saving chunk: " << filePath << std::endl;
        return false;
    }

    // Use a buffer to write all block data at once instead of separate writes
    constexpr size_t total_blocks = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;
    std::vector<unsigned char> block_data_buffer(total_blocks);
    
    size_t buffer_idx = 0;
    for (int x = 0; x < CHUNK_SIZE_X; ++x) {
        for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
            for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
                // Placeholder: Save 1 if block exists, 0 if air. 
                // Later, this should be a block ID or type.
                block_data_buffer[buffer_idx++] = (blocks[x][y][z] != nullptr) ? 1 : 0;
            }
        }
    }
    
    // Write all data at once
    outFile.write(reinterpret_cast<const char*>(block_data_buffer.data()), total_blocks);
    outFile.close();
    
    // Don't print for every chunk save - too noisy
    return true;
}

bool Chunk::loadFromFile(const std::string& directoryPath, const World* world) {
    std::string filePath = directoryPath + "/" + getChunkFileName();
    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile.is_open()) {
        // It's not an error if the file doesn't exist; it means the chunk needs to be generated.
        return false; 
    }

    Block representativeBlock(glm::vec3(0.0f), glm::vec3(0.8f), 1.0f);
    representativeBlock.init(); // Initialize shader once
    
    // Create two representative blocks - one for stone, one for grass
    Block stoneBlock(glm::vec3(0.0f), glm::vec3(0.8f), 1.0f);
    stoneBlock.shareTextureAndShaderFrom(representativeBlock);
    stoneBlock.loadTexture("res/textures/Spritesheet.PNG", 0, 0, 16, 16); // Stone texture
    
    Block grassBlock(glm::vec3(0.0f), glm::vec3(0.8f), 1.0f);
    grassBlock.shareTextureAndShaderFrom(representativeBlock);
    grassBlock.loadTexture("res/textures/Spritesheet.PNG", 16, 0, 16, 16); // Grass texture

    constexpr size_t total_blocks = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;
    std::vector<unsigned char> block_data_buffer(total_blocks);

    inFile.read(reinterpret_cast<char*>(block_data_buffer.data()), total_blocks);

    if (inFile.gcount() != total_blocks) {
        std::cerr << "Error: Could not read complete chunk data for: " << filePath 
                  << ". Read " << inFile.gcount() << " bytes, expected " << total_blocks << std::endl;
        inFile.close();
        return false; 
    }
    inFile.close(); // Close file as soon as data is in buffer

    // Process all the data in memory (more efficient)
    size_t buffer_idx = 0;
    for (int x = 0; x < CHUNK_SIZE_X; ++x) {
        for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
            for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
                unsigned char blockData = block_data_buffer[buffer_idx++];

                if (blockData == 1) {
                    glm::vec3 blockWorldPos = position + glm::vec3(x, y, z);
                    auto block = std::make_shared<Block>(blockWorldPos, glm::vec3(0.5f), 1.0f);
                    
                    // If top layer (assumes fixed landscape), use grass texture
                    if (y > 0 && buffer_idx < total_blocks && 
                        block_data_buffer[buffer_idx] == 0) { // Check if block above is air
                        block->shareTextureAndShaderFrom(grassBlock);
                    } else {
                        block->shareTextureAndShaderFrom(stoneBlock);
                    }
                    
                    blocks[x][y][z] = block;
                } else {
                    blocks[x][y][z] = nullptr; // Air
                }
            }
        }
    }

    needsRebuild = true;
    return true;
}

void Chunk::ensureInitialized(const World* world, int seed, const std::string& worldDataPath) {
    if (isInitialized_) {
        return;
    }

    // Start a timer to measure initialization time
    auto startTime = std::chrono::high_resolution_clock::now();

    bool wasLoaded = loadFromFile(worldDataPath, world);
    if (!wasLoaded) {
        generateTerrain(seed);
        saveToFile(worldDataPath);
    }

    if (needsRebuild) {
        buildSurfaceMesh(world);
    }
    
    isInitialized_ = true;
    
    // Calculate and print initialization time (but don't spam console with every chunk)
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    // Only print timing info occasionally or for very slow chunks
    if (duration > 100) { // Only print if it took longer than 100ms
        std::cout << "Chunk " << getChunkFileName() << (wasLoaded ? " loaded" : " generated") 
                  << " in " << duration << "ms" << std::endl;
    }
}

bool Chunk::isInitialized() const {
    return isInitialized_;
}



