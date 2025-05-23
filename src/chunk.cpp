#include "../headers/chunk.h"
#include "../headers/world.h" // Include World header for neighbor checks
#include "../headers/block.h" // Include Block header for Block::isTypeSolid()
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
#include <glm/gtc/noise.hpp> // For glm::simplex, if used for spherical terrain

// --- Vertex data for a single block face ---
// Order: Position (3 floats), Texture Coords (2 floats)
// We define faces relative to block center (0,0,0), size 1.0

// Vertex positions (relative to block center)
const glm::vec3 faceVertices[][4] = {
    // Back (-Z) CCW from outside (looking along +Z axis)
    { { 0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f} },
    // Front (+Z) CCW from outside (looking along -Z axis)
    { {-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f} },
    // Left (-X) CCW from outside (looking along +X axis)
    { {-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f, -0.5f} },
    // Right (+X) CCW from outside (looking along -X axis)
    { { 0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f} },
    // Bottom (-Y) CCW from outside (looking along +Y axis)
    { {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f} },
    // Top (+Y) CCW from outside (looking along -Y axis)
    { {-0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f} }
};

// Texture coordinates (standard square)
const glm::vec2 texCoords[] = {
    {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
};

Chunk::Chunk(const glm::vec3& position)
    : position(position), needsRebuild(true), isInitialized_(false) {
    blocks_.resize(CHUNK_SIZE_X, std::vector<std::vector<std::shared_ptr<Block>>>(
                                   CHUNK_SIZE_Y, std::vector<std::shared_ptr<Block>>(
                                                 CHUNK_SIZE_Z, nullptr)));
    blockDataForInitialization_.resize(CHUNK_SIZE_X, std::vector<std::vector<BlockInfo>>(
                                                      CHUNK_SIZE_Y, std::vector<BlockInfo>(
                                                                    CHUNK_SIZE_Z, BlockInfo())));
    // No planet context by default
}

Chunk::~Chunk() {
    cleanupMesh();
}

void Chunk::init(const World* /*world*/) {
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

// Render the pre-built surface mesh using a single draw call
void Chunk::renderSurface(const glm::mat4& projection, const glm::mat4& view, bool wireframeState) const {
    if (surfaceMesh.indexCount == 0 || surfaceMesh.VAO == 0) {
        return; 
    }
    if (Block::shaderProgram == 0) {
        return;
    }
    while (glGetError() != GL_NO_ERROR) {} // Clear previous OpenGL errors
    
    glUseProgram(Block::shaderProgram);
    
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    
    GLint modelLoc = glGetUniformLocation(Block::shaderProgram, "model");
    if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    
    GLint viewLoc = glGetUniformLocation(Block::shaderProgram, "view");
    if (viewLoc != -1) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    
    GLint projLoc = glGetUniformLocation(Block::shaderProgram, "projection");
    if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    
    GLint useTextureLoc = glGetUniformLocation(Block::shaderProgram, "useTexture");
    GLint blockColorLoc = glGetUniformLocation(Block::shaderProgram, "blockColor");

    if (wireframeState) {
        if (useTextureLoc != -1) glUniform1i(useTextureLoc, 0); // Don't use texture in wireframe
        if (blockColorLoc != -1) {
            float r = (static_cast<int>(position.x) * 0.1f) + 0.2f;
            float g = (static_cast<int>(position.z) * 0.1f) + 0.2f;
            float b = 0.8f;
            glUniform3f(blockColorLoc, r, g, b); 
        }
    } else {
        if (useTextureLoc != -1) {
            if (Block::spritesheetLoaded && Block::spritesheetTexture.getID() != 0) {
                glUniform1i(useTextureLoc, 1); 
                glActiveTexture(GL_TEXTURE0); 
                Block::spritesheetTexture.bind(0); 
                glUniform1i(glGetUniformLocation(Block::shaderProgram, "blockTexture"), 0); 
            } else {
                glUniform1i(useTextureLoc, 0); 
                if (blockColorLoc != -1) {
                    glUniform3f(blockColorLoc, 0.5f, 0.2f, 0.8f); // Fallback purple
                }
            }
        }
    }
    
    glBindVertexArray(surfaceMesh.VAO);
    glDrawElements(GL_TRIANGLES, surfaceMesh.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
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
                    block->render(projection, view); 
                }
            }
        }
    }
}

bool Chunk::hasBlockAtLocal(int x, int y, int z) const {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return false;
    }
    return blocks_[x][y][z] != nullptr;
}

std::shared_ptr<Block> Chunk::getBlockAtLocal(int x, int y, int z) const {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return nullptr;
    }
    return blocks_[x][y][z];
}

void Chunk::setBlockAtLocal(int x, int y, int z, std::shared_ptr<Block> block) {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return;
    }
    if ((blocks_[x][y][z] == nullptr && block != nullptr) || (blocks_[x][y][z] != nullptr && block == nullptr)) {
        needsRebuild = true;
    }
    blocks_[x][y][z] = block;
    if (block != nullptr) {
        blockDataForInitialization_[x][y][z].type = 1; // Example: default to stone if added manually
    } else {
        blockDataForInitialization_[x][y][z].type = 0; // Air
    }
}

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
    
    meshVertices.clear();
    meshIndices.clear();
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
    // Ensure directory exists (platform-dependent, POSIX example)
    // For cross-platform, C++17 <filesystem> is better if available and linked
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
                outFile.write(reinterpret_cast<const char*>(&blockDataForInitialization_[x][y][z].type), sizeof(BlockInfo::type));
            }
        }
    }
    return true;
}

bool Chunk::loadFromFile_DataOnly(const std::string& directoryPath, World* /*world*/) {
    std::string filePath = directoryPath + "/" + getChunkFileName();
    struct stat buffer;
    if (stat(filePath.c_str(), &buffer) != 0) return false; 

    std::ifstream inFile(filePath, std::ios::binary | std::ios::ate); 
    if (!inFile.is_open()) {
        return false;
    }
    std::streamsize size = inFile.tellg();
    inFile.seekg(0, std::ios::beg);

    constexpr size_t expected_size_per_block = sizeof(BlockInfo::type);
    constexpr size_t total_expected_bytes = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * expected_size_per_block;

    if (size != total_expected_bytes) {
        std::cerr << "Error: File size mismatch for chunk " << filePath 
                  << ". Expected " << total_expected_bytes << " bytes, got " << size << std::endl;
        return false;
    }
    
    for (int x = 0; x < CHUNK_SIZE_X; ++x) {
        for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
            for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
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

void Chunk::ensureInitialized(const World* world, int seed, const std::optional<glm::vec3>& planetCenter, const std::optional<float>& planetRadius) {
    if (isInitialized_) {
        return;
    }

    std::optional<glm::vec3> pCenter = planetCenter.has_value() ? planetCenter : planetCenter_;
    std::optional<float> pRadius = planetRadius.has_value() ? planetRadius : planetRadius_;

    // For now, always generate terrain and build mesh. File loading for planets can be added later.
    generateTerrain(seed, pCenter, pRadius);
    // if (world) saveToFile(world->getWorldDataPath()); // Save after generation if world context exists

    buildSurfaceMesh(world, pCenter, pRadius); // This is the single, correct call to the full buildSurfaceMesh
    isInitialized_ = true;
    needsRebuild = false;
}

bool Chunk::isInitialized() const {
    return isInitialized_;
}

void Chunk::generateTerrain(int seed, const std::optional<glm::vec3>& pCenterOpt, const std::optional<float>& pRadiusOpt) {
    std::cout << "Chunk at " << position.x << "," << position.y << "," << position.z << " generateTerrain. Planet context: " << (pCenterOpt.has_value() ? "Yes" : "No") << std::endl;

    if (blockDataForInitialization_.empty() || blockDataForInitialization_.size() != CHUNK_SIZE_X ) { 
        blockDataForInitialization_.resize(CHUNK_SIZE_X,
                                 std::vector<std::vector<BlockInfo>>(
                                     CHUNK_SIZE_Y,
                                     std::vector<BlockInfo>(CHUNK_SIZE_Z)));
    }
    if (blocks_.empty() || blocks_.size() != CHUNK_SIZE_X) { 
        blocks_.resize(CHUNK_SIZE_X, std::vector<std::vector<std::shared_ptr<Block>>>(
                               CHUNK_SIZE_Y, std::vector<std::shared_ptr<Block>>(
                                             CHUNK_SIZE_Z, nullptr)));
    }

    if (!pCenterOpt.has_value() || !pRadiusOpt.has_value()) {
        // Fallback to original flat terrain generation logic
        std::cout << "Generating flat terrain for chunk at (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
    std::vector<int> heightMap(CHUNK_SIZE_X * CHUNK_SIZE_Z);
        for (int x_local = 0; x_local < CHUNK_SIZE_X; x_local++) {
            for (int z_local = 0; z_local < CHUNK_SIZE_Z; z_local++) {
                int worldX = static_cast<int>(position.x) + x_local;
                int worldZ = static_cast<int>(position.z) + z_local;
                float heightNoise = glm::simplex(glm::vec2(worldX * 0.01f, worldZ * 0.01f));
                int terrainHeight = static_cast<int>(CHUNK_SIZE_Y / 2.0f + heightNoise * (CHUNK_SIZE_Y / 4.0f));
            terrainHeight = std::max(1, std::min(CHUNK_SIZE_Y - 1, terrainHeight));
                heightMap[x_local * CHUNK_SIZE_Z + z_local] = terrainHeight;
        }
    }

        for (int x_local = 0; x_local < CHUNK_SIZE_X; ++x_local) {
            for (int z_local = 0; z_local < CHUNK_SIZE_Z; ++z_local) {
                int terrainHeight = heightMap[x_local * CHUNK_SIZE_Z + z_local];
                for (int y_local = 0; y_local < CHUNK_SIZE_Y; ++y_local) {
                    glm::vec3 blockWorldPos = position + glm::vec3(x_local, y_local, z_local);
                    if (y_local < terrainHeight -1) { // Stone
                        blocks_[x_local][y_local][z_local] = std::make_shared<Block>(blockWorldPos, glm::vec3(0.5f), 1.0f);
                        blockDataForInitialization_[x_local][y_local][z_local].type = 1;
                    } else if (y_local == terrainHeight -1 ) { // Grass on top
                        blocks_[x_local][y_local][z_local] = std::make_shared<Block>(blockWorldPos, glm::vec3(0.2f, 0.8f, 0.2f), 1.0f);
                        blockDataForInitialization_[x_local][y_local][z_local].type = 2;
                    } else {
                        blocks_[x_local][y_local][z_local] = nullptr; // Air
                        blockDataForInitialization_[x_local][y_local][z_local].type = 0;
                    }
                }
            }
        }
        return;
    }

    // Spherical generation logic
    const glm::vec3& planetCenter = pCenterOpt.value();
    const float planetRadius = pRadiusOpt.value();
    std::cout << "Generating spherical terrain for chunk. Planet R: " << planetRadius << " Center: (" << planetCenter.x << "," << planetCenter.y << "," << planetCenter.z << ")" << std::endl;
    
    // Noise parameters for variety
    float materialNoiseScale = 0.05f; // Controls the size of material patches
    float elevationNoiseScale = 0.02f; // Controls variation in "surface" height
    float oreNoiseScale = 0.1f; // For ore patches

    for (int x_local = 0; x_local < CHUNK_SIZE_X; ++x_local) {
        for (int y_local = 0; y_local < CHUNK_SIZE_Y; ++y_local) {
            for (int z_local = 0; z_local < CHUNK_SIZE_Z; ++z_local) {
                glm::vec3 blockLocalPos = glm::vec3(x_local + 0.5f, y_local + 0.5f, z_local + 0.5f);
                glm::vec3 blockWorldPos = position + blockLocalPos; 
                float distToPlanetCenter = glm::length(blockWorldPos - planetCenter);

                unsigned char blockType = 0; // Default to air

                // Determine a slightly varied "surface" radius using noise
                float elevationNoise = glm::simplex(glm::vec2(blockWorldPos.x * elevationNoiseScale, blockWorldPos.z * elevationNoiseScale) + glm::vec2(seed * 0.1f, seed * -0.1f));
                float effectivePlanetRadius = planetRadius + elevationNoise * 2.0f; // +/- 2 blocks height variation

                if (distToPlanetCenter <= effectivePlanetRadius) {
                    // Base material is stone
                    blockType = 1; // Stone

                    // Determine surface/sub-surface materials based on depth from the *effective* surface
                    float depth = effectivePlanetRadius - distToPlanetCenter;

                    // Material noise for biome-like variations
                    float materialNoiseVal = glm::simplex(glm::vec3(blockWorldPos * materialNoiseScale + glm::vec3(seed * 0.3f)));

                    if (depth < 1.5f) { // Top layer
                        if (materialNoiseVal < -0.3f) {
                            blockType = 6; // Snow (cold areas)
                        } else if (materialNoiseVal < 0.2f) {
                            blockType = 2; // Grass
                        } else {
                            blockType = 4; // Sand (hot/dry areas)
                        }
                    } else if (depth < 5.0f) { // Sub-surface layer
                         if (materialNoiseVal < -0.3f && blockType != 6) { // Avoid dirt under snow directly
                            blockType = 9; // Gravel (under cold areas if not snow)
                        } else if (materialNoiseVal < 0.2f) {
                            blockType = 3; // Dirt
                        } else { // Under sand, can still be dirt or more stone
                            blockType = 3; // Dirt (or could be more stone/sandstone if defined)
                        }
                    }
                    
                    // Occasionally, place ores deeper down
                    if (depth > 5.0f) {
                        float oreNoiseVal = glm::simplex(blockWorldPos * oreNoiseScale + glm::vec3(seed * 0.7f));
                        if (oreNoiseVal > 0.75f) { // Adjust threshold for rarity
                            blockType = 10; // Gold Ore
                        }
                    }

                    // Simple water level - place water in areas where terrain is "carved out" below water level
                    float waterLevelRadius = planetRadius * 0.7f; // Water level at 70% of planet radius
                    
                    // If this block is solid (from above logic) but is within the water level,
                    // and the original sphere would be hollow here, place water instead
                    if (distToPlanetCenter <= waterLevelRadius) {
                        // If we're within the water level, override with water
                        blockType = 5; // Water
                    }

                }

                blockDataForInitialization_[x_local][y_local][z_local].type = blockType;
                if (blockType != 0) {
                    blocks_[x_local][y_local][z_local] = std::make_shared<Block>(position + glm::vec3(x_local,y_local,z_local), glm::vec3(0.5f), 1.0f);
                } else {
                    blocks_[x_local][y_local][z_local] = nullptr; 
                }
            }
        }
    }
}

// This is the single, complete definition of buildSurfaceMesh
void Chunk::buildSurfaceMesh(const World* /*world*/, const std::optional<glm::vec3>& pCenterOpt, const std::optional<float>& pRadiusOpt) {
    cleanupMesh(); 
    meshVertices.clear();
    meshIndices.clear();
    unsigned int vertexIndexOffset = 0;

    if (blockDataForInitialization_.empty() || blockDataForInitialization_.size() != CHUNK_SIZE_X ) { 
        blockDataForInitialization_.resize(CHUNK_SIZE_X,
                                 std::vector<std::vector<BlockInfo>>(
                                     CHUNK_SIZE_Y,
                                     std::vector<BlockInfo>(CHUNK_SIZE_Z)));
    }

    const int neighborOffsets[6][3] = {
        {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}
    };

    if (!pCenterOpt.has_value() || !pRadiusOpt.has_value()) {
        // Fallback to original flat terrain meshing logic
        std::cout << "Building flat mesh for chunk at (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
        for (int x_local = 0; x_local < CHUNK_SIZE_X; ++x_local) {
            for (int y_local = 0; y_local < CHUNK_SIZE_Y; ++y_local) {
                for (int z_local = 0; z_local < CHUNK_SIZE_Z; ++z_local) {
                    if (!hasBlockAtLocal(x_local, y_local, z_local)) continue;

                    for (int face = 0; face < 6; ++face) {
                        int nx = x_local + neighborOffsets[face][0];
                        int ny = y_local + neighborOffsets[face][1];
                        int nz = z_local + neighborOffsets[face][2];
                        bool shouldRenderFace = false;

                        unsigned char currentBlockType = blockDataForInitialization_[x_local][y_local][z_local].type;
                        if (currentBlockType == 0) continue; // Should not happen if hasBlockAtLocal was true, but defensive

                        if (nx < 0 || nx >= CHUNK_SIZE_X || ny < 0 || ny >= CHUNK_SIZE_Y || nz < 0 || nz >= CHUNK_SIZE_Z) {
                            shouldRenderFace = true; 
                        } else {
                            unsigned char neighborBlockType = blockDataForInitialization_[nx][ny][nz].type;
                            if (!Block::isTypeSolid(neighborBlockType)) { // If neighbor is not solid (e.g., Air, Water, Leaves)
                                shouldRenderFace = true;
                            }
                            // Else, neighbor is solid, so don't render this face
                        }

                        if (shouldRenderFace) {
                            float uvPixelOffsetX = 0.0f, uvPixelOffsetY = 0.0f;
                            if (currentBlockType == 1) { uvPixelOffsetX = 0.0f; uvPixelOffsetY = 0.0f; }      // Stone (0,0)
                            else if (currentBlockType == 2) { uvPixelOffsetX = 80.0f; uvPixelOffsetY = 0.0f; } // Grass (1,0)
                            else if (currentBlockType == 3) { uvPixelOffsetX = 160.0f; uvPixelOffsetY = 0.0f; } // Dirt (2,0)
                            else if (currentBlockType == 4) { uvPixelOffsetX = 240.0f; uvPixelOffsetY = 0.0f; } // Sand (3,0)
                            else if (currentBlockType == 5) { uvPixelOffsetX = 80.0f; uvPixelOffsetY = 80.0f; } // Water (4,0)
                            else if (currentBlockType == 6) { uvPixelOffsetX = 0.0f; uvPixelOffsetY = 80.0f; }   // Snow (0,1)
                            else if (currentBlockType == 7) { uvPixelOffsetX = 80.0f; uvPixelOffsetY = 80.0f; } // Wood Log (1,1)
                            else if (currentBlockType == 8) { uvPixelOffsetX = 160.0f; uvPixelOffsetY = 80.0f; } // Leaves (2,1)
                            else if (currentBlockType == 9) { uvPixelOffsetX = 240.0f; uvPixelOffsetY = 00.0f; } // Gravel (3,1)
                            else if (currentBlockType == 10) { uvPixelOffsetX = 320.0f; uvPixelOffsetY = 80.0f; } // Gold Ore (4,1)
                            // Add more types as needed
                            else { /* Default or air, UVs won't matter as face isn't rendered or uses default */ }

                            for (int i = 0; i < 4; ++i) {
                                meshVertices.push_back(x_local + faceVertices[face][i][0]);
                                meshVertices.push_back(y_local + faceVertices[face][i][1]);
                                meshVertices.push_back(z_local + faceVertices[face][i][2]);
                                if (Block::spritesheetLoaded && Block::spritesheetTexture.getWidth() > 0) {
                                    meshVertices.push_back((uvPixelOffsetX + texCoords[i].x * 80.0f) / Block::spritesheetTexture.getWidth());
                                    meshVertices.push_back((uvPixelOffsetY + texCoords[i].y * 80.0f) / Block::spritesheetTexture.getHeight());
                                } else {
                                    meshVertices.push_back(texCoords[i].x); // Fallback UVs
                                    meshVertices.push_back(texCoords[i].y);
                                }
                            }
                            meshIndices.push_back(vertexIndexOffset + 0); meshIndices.push_back(vertexIndexOffset + 1); meshIndices.push_back(vertexIndexOffset + 2);
                            meshIndices.push_back(vertexIndexOffset + 2); meshIndices.push_back(vertexIndexOffset + 3); meshIndices.push_back(vertexIndexOffset + 0);
                            vertexIndexOffset += 4;
                        }
                    }
                }
            }
        }
    } else {
        // Spherical mesh generation logic
        const glm::vec3& planetCenter = pCenterOpt.value();
        const float planetRadius = pRadiusOpt.value();
        std::cout << "Building spherical mesh for chunk. Planet R: " << planetRadius << " Chunk Pos: (" << position.x << "," << position.y << "," << position.z << ")" << std::endl;

        for (int x_loc = 0; x_loc < CHUNK_SIZE_X; ++x_loc) {
            for (int y_loc = 0; y_loc < CHUNK_SIZE_Y; ++y_loc) {
                for (int z_loc = 0; z_loc < CHUNK_SIZE_Z; ++z_loc) {
                    if (!hasBlockAtLocal(x_loc, y_loc, z_loc)) continue;

                    for (int face = 0; face < 6; ++face) {
                        // Check neighbor block for culling
                        int nx_loc = x_loc + neighborOffsets[face][0];
                        int ny_loc = y_loc + neighborOffsets[face][1];
                        int nz_loc = z_loc + neighborOffsets[face][2];

                        bool shouldRenderFace = false;
                        unsigned char currentBlockType = blockDataForInitialization_[x_loc][y_loc][z_loc].type;
                        if (currentBlockType == 0) continue; // Should not happen if hasBlockAtLocal was true

                        if (nx_loc < 0 || nx_loc >= CHUNK_SIZE_X || ny_loc < 0 || ny_loc >= CHUNK_SIZE_Y || nz_loc < 0 || nz_loc >= CHUNK_SIZE_Z) {
                            glm::vec3 neighborBlockWorldCenter = position + glm::vec3(nx_loc + 0.5f, ny_loc + 0.5f, nz_loc + 0.5f);
                            if (glm::length(neighborBlockWorldCenter - planetCenter) > planetRadius) {
                                shouldRenderFace = true; // Neighbor is outside planet, effectively air
                            } else {
                                // Neighbor is inside planet radius but in another chunk.
                                // For now, assume it could be non-solid or we want to see the boundary.
                                // A more complex check would involve getting the block type from the adjacent chunk.
                                // Let's simplify: if the current block is solid and the neighbor is at the boundary, render.
                                // This also needs the type of the neighbor to be truly correct.
                                // For now, the logic will be similar to the intra-chunk check: render if neighbor *would be* non-solid.
                                // This needs a proper neighbor check across chunks for perfection.
                                shouldRenderFace = true; // Placeholder: always render inter-chunk faces for now if current is solid
                            }
                        } else {
                            unsigned char neighborBlockType = blockDataForInitialization_[nx_loc][ny_loc][nz_loc].type;
                            if (!Block::isTypeSolid(neighborBlockType)) { // If neighbor is not solid
                                shouldRenderFace = true;
                            }
                            // Else, neighbor is solid, so don't render this face
                        }

                        if (shouldRenderFace) {
                            float uvPixelOffsetX = 0.0f, uvPixelOffsetY = 0.0f;
                            if (currentBlockType == 1) { uvPixelOffsetX = 0.0f; uvPixelOffsetY = 0.0f; }      // Stone (0,0)
                            else if (currentBlockType == 2) { uvPixelOffsetX = 80.0f; uvPixelOffsetY = 0.0f; } // Grass (1,0)
                            else if (currentBlockType == 3) { uvPixelOffsetX = 160.0f; uvPixelOffsetY = 0.0f; } // Dirt (2,0)
                            else if (currentBlockType == 4) { uvPixelOffsetX = 240.0f; uvPixelOffsetY = 0.0f; } // Sand (3,0)
                            else if (currentBlockType == 5) { uvPixelOffsetX = 320.0f; uvPixelOffsetY = 0.0f; } // Water (4,0)
                            else if (currentBlockType == 6) { uvPixelOffsetX = 0.0f; uvPixelOffsetY = 80.0f; }   // Snow (0,1)
                            else if (currentBlockType == 7) { uvPixelOffsetX = 80.0f; uvPixelOffsetY = 80.0f; } // Wood Log (1,1)
                            else if (currentBlockType == 8) { uvPixelOffsetX = 160.0f; uvPixelOffsetY = 80.0f; } // Leaves (2,1)
                            else if (currentBlockType == 9) { uvPixelOffsetX = 240.0f; uvPixelOffsetY = 80.0f; } // Gravel (3,1)
                            else if (currentBlockType == 10) { uvPixelOffsetX = 320.0f; uvPixelOffsetY = 80.0f; } // Gold Ore (4,1)
                             // Add more types as needed
                            else { /* Default or air, UVs won't matter as face isn't rendered or uses default */ }

                            for (int i = 0; i < 4; ++i) {
                                glm::vec3 localFaceVertexPos = glm::vec3(x_loc + faceVertices[face][i][0],
                                                                   y_loc + faceVertices[face][i][1],
                                                                   z_loc + faceVertices[face][i][2]);
                                glm::vec3 vboVertexPos = localFaceVertexPos; // Vertices are already local to chunk origin

                                meshVertices.push_back(vboVertexPos.x);
                                meshVertices.push_back(vboVertexPos.y);
                                meshVertices.push_back(vboVertexPos.z);

                                if (Block::spritesheetLoaded && Block::spritesheetTexture.getWidth() > 0) {
                                    meshVertices.push_back((uvPixelOffsetX + texCoords[i].x * 80.0f) / Block::spritesheetTexture.getWidth());
                                    meshVertices.push_back((uvPixelOffsetY + texCoords[i].y * 80.0f) / Block::spritesheetTexture.getHeight());
                                } else {
                                    meshVertices.push_back(texCoords[i].x);
                                    meshVertices.push_back(texCoords[i].y);
                                }
                            }
                            meshIndices.push_back(vertexIndexOffset + 0); meshIndices.push_back(vertexIndexOffset + 1); meshIndices.push_back(vertexIndexOffset + 2);
                            meshIndices.push_back(vertexIndexOffset + 2); meshIndices.push_back(vertexIndexOffset + 3); meshIndices.push_back(vertexIndexOffset + 0);
                            vertexIndexOffset += 4;
                        }
                    }
                }
            }
        }
    }

    if (meshVertices.empty()) {
        surfaceMesh.indexCount = 0;
        surfaceMesh.VAO = 0;
        needsRebuild = false; 
        return;
    }

    if (Block::shaderProgram == 0) {
        std::cout << "BuildSurfaceMesh: Initializing block shader..." << std::endl;
        Block::InitBlockShader();
        if (Block::shaderProgram == 0) {
            std::cerr << "CRITICAL ERROR: Failed to init block shader in BuildSurfaceMesh." << std::endl;
            return;
        }
    }
    while (glGetError() != GL_NO_ERROR) {} 

    glGenVertexArrays(1, &surfaceMesh.VAO);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR || surfaceMesh.VAO == 0) {
        std::cerr << "CRITICAL ERROR: Failed to generate VAO for chunk. OpenGL error: " << error << " VAO ID: " << surfaceMesh.VAO << std::endl;
        surfaceMesh.VAO = 0; 
        return;
    }
    glBindVertexArray(surfaceMesh.VAO);

    glGenBuffers(1, &surfaceMesh.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, surfaceMesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(float), meshVertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &surfaceMesh.EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, surfaceMesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshIndices.size() * sizeof(unsigned int), meshIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0); 
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error after creating mesh resources for chunk: " << error << std::endl;
        cleanupMesh(); 
        return;
    }
    surfaceMesh.indexCount = meshIndices.size();
    needsRebuild = false;
    std::cout << "Mesh built for chunk. Faces: " << surfaceMesh.indexCount / 6 << ", VAO: " << surfaceMesh.VAO << std::endl;
}

void Chunk::openglInitialize(World* world) {
    if (isInitialized_ && surfaceMesh.VAO != 0) return; // Already GL initialized

    if (glfwGetCurrentContext() == nullptr) {
        std::cerr << "CRITICAL ERROR: No OpenGL context current on main thread for openglInitialize! Chunk: " 
                  << position.x << "," << position.z << std::endl;
        return;
    }

    std::cout << "MainThread: OpenGL-Initializing chunk at " << position.x << "," << position.z << std::endl;
    
    if (Block::shaderProgram == 0) {
        Block::InitBlockShader();
        if (Block::shaderProgram == 0) {
            std::cerr << "CRITICAL ERROR: Failed to initialize block shader program! Cannot initialize chunk." << std::endl;
            return;
        }
    }
    if (!Block::spritesheetLoaded) { 
        Block::InitSpritesheet("res/textures/Spritesheet.PNG"); // Path from executable location
        if (!Block::spritesheetLoaded) {
            std::cerr << "CRITICAL ERROR: Failed to load global spritesheet in openglInitialize! Chunk texturing might fail." << std::endl;
        }
    }

    for (int x = 0; x < CHUNK_SIZE_X; ++x) {
        for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
            for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
                BlockInfo info = blockDataForInitialization_[x][y][z];
                glm::vec3 blockWorldPos = this->position + glm::vec3(x, y, z);
                
                if (info.type != 0 && blocks_[x][y][z] == nullptr) { 
                    if (info.type == 1) { 
                        blocks_[x][y][z] = std::make_shared<Block>(blockWorldPos, glm::vec3(0.5f, 0.5f, 0.5f), 1.0f); 
                    } else if (info.type == 2) { 
                        blocks_[x][y][z] = std::make_shared<Block>(blockWorldPos, glm::vec3(0.2f, 0.8f, 0.2f), 1.0f);
                    } else if (info.type == 3) { 
                        blocks_[x][y][z] = std::make_shared<Block>(blockWorldPos, glm::vec3(0.6f, 0.4f, 0.2f), 1.0f);
                    } else if (info.type == 4) { // Sand
                        blocks_[x][y][z] = std::make_shared<Block>(blockWorldPos, glm::vec3(0.9f, 0.9f, 0.6f), 1.0f); // Sandy color
                    } else if (info.type == 5) { // Water
                        blocks_[x][y][z] = std::make_shared<Block>(blockWorldPos, glm::vec3(0.2f, 0.4f, 0.8f), 1.0f); // Watery color
                    } else if (info.type == 6) { // Snow
                        blocks_[x][y][z] = std::make_shared<Block>(blockWorldPos, glm::vec3(0.95f, 0.95f, 1.0f), 1.0f); // Snowy color
                    } else if (info.type == 7) { // Wood Log
                        blocks_[x][y][z] = std::make_shared<Block>(blockWorldPos, glm::vec3(0.4f, 0.3f, 0.2f), 1.0f); // Woody color
                    } else if (info.type == 8) { // Leaves
                        blocks_[x][y][z] = std::make_shared<Block>(blockWorldPos, glm::vec3(0.1f, 0.5f, 0.1f), 1.0f); // Leafy color
                    } else if (info.type == 9) { // Gravel
                        blocks_[x][y][z] = std::make_shared<Block>(blockWorldPos, glm::vec3(0.6f, 0.6f, 0.6f), 1.0f); // Gravel color
                    } else if (info.type == 10) { // Gold Ore
                        blocks_[x][y][z] = std::make_shared<Block>(blockWorldPos, glm::vec3(0.8f, 0.7f, 0.2f), 1.0f); // Goldish color
                    } else { // Default for any other new types, or if type is unknown but not 0
                        blocks_[x][y][z] = std::make_shared<Block>(blockWorldPos, glm::vec3(0.5f), 1.0f); // Default grey
                    }
                } else if (info.type == 0 && blocks_[x][y][z] != nullptr) { 
                    blocks_[x][y][z] = nullptr;
                }
            }
        }
    }

    if (needsRebuild || surfaceMesh.VAO == 0) { 
        std::cout << "MainThread: Building surface mesh for chunk " << position.x << "," << position.z << " during openglInitialize" << std::endl;
        if (planetCenter_.has_value() && planetRadius_.has_value()) {
            buildSurfaceMesh(world, planetCenter_, planetRadius_); // world can be null here, but spherical meshing might not use it extensively
        } else {
            // For flat chunks, world context is more critical for neighbor checks.
            // If world is null, buildSurfaceMesh for flat chunks might operate with limitations (e.g., render all boundary faces).
            buildSurfaceMesh(world, std::nullopt, std::nullopt); 
        }
    }
    
    if (world) { // Check if world is not null before calling save
       // saveToFile(world->getWorldDataPath()); // Save is currently commented out, but guard it anyway for the future
    }

    isInitialized_ = true;
    needsRebuild = false; 
    std::cout << "Chunk at " << position.x << "," << position.z << " OpenGL-initialized. VAO=" << surfaceMesh.VAO << std::endl;
}

void Chunk::setPlanetContext(const glm::vec3& planetCenter, float planetRadius) {
    planetCenter_ = planetCenter;
    planetRadius_ = planetRadius;
    isInitialized_ = false; // Re-initialization will be needed
    needsRebuild = true; 
}



