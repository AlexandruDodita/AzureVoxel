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

Chunk::Chunk(const glm::vec3& position) : position(position), needsRebuild(true) {
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
    if (!blocks[0][0][0]) { // Only generate terrain once
         generateTerrain();
    }
    buildSurfaceMesh(world);
    needsRebuild = false;
}

// Simple noise function (replace with a proper Perlin/Simplex noise later)
float simpleNoise(int x, int y, int z) {
    // Combine inputs and use a simple hashing-like approach
    // This is NOT a good noise function, just a placeholder
    int h = x * 374761393 + y * 668265263 + z * 104729;
    h = (h ^ (h >> 13)) * 1274126177;
    return static_cast<float>((h ^ (h >> 16)) & 0x7fffffff) / static_cast<float>(0x7fffffff);
}

void Chunk::generateTerrain() {
    Block representativeBlock(glm::vec3(0.0f), glm::vec3(0.8f), 1.0f);
    representativeBlock.init();
    // For now, load a default grass-like texture for all blocks
    // We'll need a way to map noise values to different block types/textures later
    // The first texture (0,0) in the spritesheet, assuming 16x16 textures
    representativeBlock.loadTexture("res/textures/Spritesheet.PNG", 0, 0, 16, 16); 

    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int z = 0; z < CHUNK_SIZE_Z; z++) {
            // Get world coordinates for noise calculation
            int worldX = static_cast<int>(position.x) + x;
            int worldZ = static_cast<int>(position.z) + z;

            // Generate a height value using simple noise
            // Scale and offset noise to get a reasonable terrain height
            float heightNoise = simpleNoise(worldX, 0, worldZ); // Using y=0 for 2D heightmap for now
            int terrainHeight = static_cast<int>(CHUNK_SIZE_Y / 2 + heightNoise * (CHUNK_SIZE_Y / 4));
            terrainHeight = std::max(1, std::min(CHUNK_SIZE_Y -1, terrainHeight)); // Clamp height

            for (int y = 0; y < CHUNK_SIZE_Y; y++) {
                glm::vec3 blockWorldPos = position + glm::vec3(x, y, z);
                if (y < terrainHeight) {
                    auto block = std::make_shared<Block>(blockWorldPos, glm::vec3(0.5f, 0.5f, 0.5f), 1.0f);
                    block->shareTextureAndShaderFrom(representativeBlock);
                    // For simplicity, let's assume the first texture in the spritesheet (0,0) is stone
                    // and the second one (16,0) is grass, if y is the top layer.
                    if (y == terrainHeight - 1) { // Top layer, make it grass
                        // Create a new texture object for grass or ensure representative block is grass
                        // For now, we'll just load it. This is inefficient and needs a block type system.
                        block->loadTexture("res/textures/Spritesheet.PNG", 16, 0, 16, 16); // Assuming grass is at (16,0)
                    } else { // Underground, make it stone
                        block->loadTexture("res/textures/Spritesheet.PNG", 0, 0, 16, 16); // Assuming stone is at (0,0)
                    }
                    setBlockAtLocal(x, y, z, block);
                } else {
                    setBlockAtLocal(x, y, z, nullptr); // Air block
                }
            }
        }
    }
    std::cout << "Chunk at (" << position.x << "," << position.z << ") generated with noise-based terrain." << std::endl;
}

// Builds a single mesh containing only the visible faces of the blocks in this chunk.
void Chunk::buildSurfaceMesh(const World* world) {
    cleanupMesh(); // Clear old mesh data if rebuilding

    std::vector<float> meshVertices; // Stores vertex data (pos + texcoord)
    std::vector<unsigned int> meshIndices;
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

    for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
        for (int x = 0; x < CHUNK_SIZE_X; ++x) {
            for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
                if (!hasBlockAtLocal(x, y, z)) continue; // Skip air blocks

                // Calculate the absolute world position of the current block
                int worldX = static_cast<int>(position.x) + x;
                int worldY = static_cast<int>(position.y) + y;
                int worldZ = static_cast<int>(position.z) + z;

                // Check all 6 neighbors
                for (int face = 0; face < 6; ++face) {
                    int neighborWorldX = worldX + neighborOffsets[face][0];
                    int neighborWorldY = worldY + neighborOffsets[face][1];
                    int neighborWorldZ = worldZ + neighborOffsets[face][2];

                    // Check if the neighbor block exists (using the world query)
                    if (!world || !world->getBlockAtWorldPos(neighborWorldX, neighborWorldY, neighborWorldZ)) {
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

    for (int x = 0; x < CHUNK_SIZE_X; ++x) {
        for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
            for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
                // Placeholder: Save 1 if block exists, 0 if air. 
                // Later, this should be a block ID or type.
                unsigned char blockData = (blocks[x][y][z] != nullptr) ? 1 : 0;
                outFile.write(reinterpret_cast<const char*>(&blockData), sizeof(blockData));
            }
        }
    }

    outFile.close();
    std::cout << "Chunk saved to: " << filePath << std::endl;
    return true;
}

bool Chunk::loadFromFile(const std::string& directoryPath, const World* world) {
    std::string filePath = directoryPath + "/" + getChunkFileName();
    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile.is_open()) {
        // It's not an error if the file doesn't exist; it means the chunk needs to be generated.
        // std::cerr << "Info: Chunk file not found, will generate new: " << filePath << std::endl;
        return false; 
    }

    Block representativeBlock(glm::vec3(0.0f), glm::vec3(0.8f), 1.0f);
    representativeBlock.init(); // Initialize shader once
    // Default texture, will be overridden if block data specifies a type
    representativeBlock.loadTexture("res/textures/Spritesheet.PNG", 0, 0, 16, 16); 

    bool hasAnyBlocks = false;
    for (int x = 0; x < CHUNK_SIZE_X; ++x) {
        for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
            for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
                unsigned char blockData;
                inFile.read(reinterpret_cast<char*>(&blockData), sizeof(blockData));
                if (inFile.eof()) {
                    std::cerr << "Error: Unexpected end of file while loading chunk: " << filePath << std::endl;
                    inFile.close();
                    return false; // Or handle as corrupted data
                }

                if (blockData == 1) { // Placeholder: block exists
                    glm::vec3 blockWorldPos = position + glm::vec3(x, y, z);
                    auto block = std::make_shared<Block>(blockWorldPos, glm::vec3(0.5f, 0.5f, 0.5f), 1.0f);
                    block->shareTextureAndShaderFrom(representativeBlock);
                    // TODO: Load specific texture based on a more detailed blockData value later
                    // For now, just use the representative block's texture (stone)
                    block->loadTexture("res/textures/Spritesheet.PNG", 0, 0, 16, 16); // Stone
                    setBlockAtLocal(x, y, z, block);
                    hasAnyBlocks = true;
                } else {
                    setBlockAtLocal(x, y, z, nullptr); // Air
                }
            }
        }
    }

    inFile.close();
    if (hasAnyBlocks) {
      std::cout << "Chunk loaded from: " << filePath << std::endl;
    }
    needsRebuild = true; // Mesh needs to be rebuilt after loading
    // The actual mesh building will be triggered by World or init method
    // if (world) { buildSurfaceMesh(world); }
    return true;
}



