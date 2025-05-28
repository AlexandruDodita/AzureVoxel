#include "../headers/chunk.h"
#include "../headers/world.h" // Include World header for neighbor checks
#include "../headers/block.h" // Include Block header for Block::isTypeSolid()
#include "../headers/block_registry.h"
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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem> // For chunk data saving
#include <GL/glew.h>
#include <mutex> // For std::mutex

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
    : position(position), state_(ChunkState::UNINITIALIZED), needsRebuild_(true) {
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
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    if (state_.load() >= ChunkState::DATA_READY) {
        return blockDataForInitialization_[x][y][z].type != 0;
    }
    return blocks_[x][y][z] != nullptr;
}

std::shared_ptr<Block> Chunk::getBlockAtLocal(int x, int y, int z) const {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    return blocks_[x][y][z];
}

void Chunk::setBlockAtLocal(int x, int y, int z, std::shared_ptr<Block> block) {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    if ((blocks_[x][y][z] == nullptr && block != nullptr) || (blocks_[x][y][z] != nullptr && block == nullptr)) {
        needsRebuild_.store(true);
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
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    if (blocks_[x][y][z] != nullptr) {
        blocks_[x][y][z] = nullptr;
        blockDataForInitialization_[x][y][z].type = 0; // Air
        needsRebuild_.store(true);
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
    needsRebuild_.store(true);
    return true; 
}

void Chunk::ensureInitialized(const World* world, int seed, const std::optional<glm::vec3>& planetCenter, const std::optional<float>& planetRadius) {
    if (isInitialized()) { // MODIFIED: From isInitialized_
        return;
    }

    std::optional<glm::vec3> pCenter = planetCenter.has_value() ? planetCenter : planetCenter_;
    std::optional<float> pRadius = planetRadius.has_value() ? planetRadius : planetRadius_;

    if (blockDataForInitialization_.empty() || blockDataForInitialization_.size() != CHUNK_SIZE_X) { 
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

    bool loadedFromFile = false;
    if (world) {
        std::string worldDataPath = "chunk_data/" + world->getWorldName();
        loadedFromFile = loadFromFile_DataOnly(worldDataPath, const_cast<World*>(world));
        if (loadedFromFile) {
            std::cout << "✓ LEGACY_LOAD: Chunk " << position.x << "," << position.y << "," << position.z << " from file (ensureInitialized)" << std::endl;
            for (int x_local = 0; x_local < CHUNK_SIZE_X; ++x_local) {
                for (int y_local = 0; y_local < CHUNK_SIZE_Y; ++y_local) {
                    for (int z_local = 0; z_local < CHUNK_SIZE_Z; ++z_local) {
                        uint16_t blockTypeId = static_cast<uint16_t>(blockDataForInitialization_[x_local][y_local][z_local].type);
                        glm::vec3 blockWorldPos = position + glm::vec3(x_local, y_local, z_local);
                        if (blockTypeId != 0) {
                            blocks_[x_local][y_local][z_local] = std::make_shared<Block>(blockWorldPos, blockTypeId, glm::vec3(0.5f), 1.0f);
                        } else {
                            blocks_[x_local][y_local][z_local] = nullptr;
                        }
                    }
                }
            }
        } else {
            std::cout << "✗ LEGACY_LOAD_FAIL: Chunk " << position.x << "," << position.y << "," << position.z << " not found (ensureInitialized)" << std::endl;
        }
    }

    if (!loadedFromFile) {
        std::cout << "⚡ LEGACY_GEN: Chunk " << position.x << "," << position.y << "," << position.z << " (ensureInitialized)" << std::endl;
        generateTerrain(seed, pCenter, pRadius); // Calls old generateTerrain
        if (world) {
            std::string worldDataPath = "chunk_data/" + world->getWorldName();
            if (saveToFile(worldDataPath)) {
                std::cout << "💾 LEGACY_SAVE: Chunk " << position.x << "," << position.y << "," << position.z << " (ensureInitialized)" << std::endl;
            } else {
                std::cerr << "❌ LEGACY_SAVE_FAIL: Chunk " << position.x << "," << position.y << "," << position.z << " (ensureInitialized)" << std::endl;
            }
        }
    }

    buildSurfaceMesh(world, pCenter, pRadius);
    openglInitialize(const_cast<World*>(world)); // Call legacy GL init. It will set state if successful.
    needsRebuild_.store(false);
}

void Chunk::generateTerrain(int seed, const std::optional<glm::vec3>& pCenterOpt, const std::optional<float>& pRadiusOpt) {
    std::cout << "Chunk at " << position.x << "," << position.y << "," << position.z << " generateTerrain. Planet context: " << (pCenterOpt.has_value() ? "Yes" : "No") << std::endl;

    // Get reference to the block registry
    BlockRegistry& registry = BlockRegistry::getInstance();

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
                    
                    uint16_t blockTypeId = 0; // Default to air
                    
                    if (y_local < terrainHeight - 1) {
                        // Use registry to get stone block ID
                        blockTypeId = registry.getBlockId("azurevoxel:stone");
                    } else if (y_local == terrainHeight - 1) {
                        // Use registry to get grass block ID
                        blockTypeId = registry.getBlockId("azurevoxel:grass");
                    }
                    
                    blockDataForInitialization_[x_local][y_local][z_local].type = static_cast<unsigned char>(blockTypeId);
                    
                    if (blockTypeId != 0) {
                        blocks_[x_local][y_local][z_local] = std::make_shared<Block>(blockWorldPos, blockTypeId, glm::vec3(0.5f), 1.0f);
                    } else {
                        blocks_[x_local][y_local][z_local] = nullptr;
                    }
                }
            }
        }
        return;
    }

    // Spherical generation logic with biome-aware block selection
    const glm::vec3& planetCenter = pCenterOpt.value();
    const float planetRadius = pRadiusOpt.value();
    std::cout << "Generating spherical terrain for chunk. Planet R: " << planetRadius << " Center: (" << planetCenter.x << "," << planetCenter.y << "," << planetCenter.z << ")" << std::endl;
    
    // Noise parameters for variety
    float elevationNoiseScale = 0.02f; // Controls variation in "surface" height
    float oreNoiseScale = 0.1f; // For ore generation
    
    // Additional noise layers for diverse terrain features
    float temperatureNoiseScale = 0.03f; // Large-scale temperature variation
    float moistureNoiseScale = 0.04f; // Large-scale moisture variation
    float elevationMajorScale = 0.01f; // Large-scale elevation (mountains/valleys)
    float featureNoiseScale = 0.08f; // Small-scale features (lakes, forests)

    for (int x_local = 0; x_local < CHUNK_SIZE_X; ++x_local) {
        for (int y_local = 0; y_local < CHUNK_SIZE_Y; ++y_local) {
            for (int z_local = 0; z_local < CHUNK_SIZE_Z; ++z_local) {
                glm::vec3 blockLocalPos = glm::vec3(x_local + 0.5f, y_local + 0.5f, z_local + 0.5f);
                glm::vec3 blockWorldPos = position + blockLocalPos; 
                float distToPlanetCenter = glm::length(blockWorldPos - planetCenter);

                uint16_t blockTypeId = 0; // Default to air

                // Determine a slightly varied "surface" radius using noise
                float elevationNoise = glm::simplex(glm::vec2(blockWorldPos.x * elevationNoiseScale, blockWorldPos.z * elevationNoiseScale) + glm::vec2(seed * 0.1f, seed * -0.1f));
                float effectivePlanetRadius = planetRadius + elevationNoise * 2.0f; // +/- 2 blocks height variation

                if (distToPlanetCenter <= effectivePlanetRadius) {
                    // Multi-layer noise for sophisticated biome generation
                    float temperatureNoise = glm::simplex(glm::vec2(blockWorldPos.x * temperatureNoiseScale, blockWorldPos.z * temperatureNoiseScale) + glm::vec2(seed * 0.2f));
                    float moistureNoise = glm::simplex(glm::vec2(blockWorldPos.x * moistureNoiseScale, blockWorldPos.z * moistureNoiseScale) + glm::vec2(seed * 0.4f));
                    float elevationMajorNoise = glm::simplex(glm::vec2(blockWorldPos.x * elevationMajorScale, blockWorldPos.z * elevationMajorScale) + glm::vec2(seed * 0.6f));
                    float featureNoise = glm::simplex(glm::vec3(blockWorldPos * featureNoiseScale + glm::vec3(seed * 0.3f)));
                    
                    // Adjust effective radius based on major elevation features (mountains/valleys)
                    effectivePlanetRadius += elevationMajorNoise * 15.0f; // +/- 15 blocks for major terrain features
                    
                    // Recalculate if block is still within the adjusted surface
                    if (distToPlanetCenter > effectivePlanetRadius) {
                        blockTypeId = 0; // Air
                    } else {
                        // Determine biome based on multiple noise layers
                    BiomeContext biome;
                        
                        // Combine temperature and moisture for biome selection
                        float temperature = temperatureNoise * 0.7f + elevationMajorNoise * -0.3f; // Higher elevation = colder
                        float moisture = moistureNoise * 0.8f + featureNoise * 0.2f;
                        
                        // Determine primary biome
                        if (temperature < -0.6f) {
                            if (moisture < -0.3f) {
                                biome = BiomeContext("arctic", -0.9f, 0.1f); // Arctic
                    } else {
                                biome = BiomeContext("tundra", -0.6f, 0.4f); // Tundra
                            }
                        } else if (temperature < -0.2f) {
                            if (moisture < 0.0f) {
                                biome = BiomeContext("cold", -0.7f, 0.3f); // Cold
                            } else {
                                biome = BiomeContext("forest", 0.3f, 0.7f); // Cold forest
                            }
                        } else if (temperature < 0.3f) {
                            if (moisture < -0.4f) {
                                biome = BiomeContext("temperate", 0.2f, 0.5f); // Temperate plains
                            } else if (moisture > 0.6f) {
                                biome = BiomeContext("swamp", 0.4f, 0.9f); // Swamp
                            } else {
                                biome = BiomeContext("forest", 0.3f, 0.7f); // Temperate forest
                            }
                        } else if (temperature < 0.7f) {
                            if (moisture < -0.5f) {
                                biome = BiomeContext("desert", 0.9f, -0.8f); // Desert
                            } else {
                                biome = BiomeContext("tropical", 0.7f, 0.8f); // Tropical
                            }
                        } else {
                            if (moisture < -0.3f) {
                                biome = BiomeContext("volcanic", 1.0f, -0.5f); // Volcanic
                            } else {
                                biome = BiomeContext("hot", 0.8f, -0.3f); // Hot
                            }
                        }
                        
                        // Special mountain biome for high elevation areas
                        if (elevationMajorNoise > 0.4f) {
                            biome = BiomeContext("mountain", -0.3f, 0.2f);
                    }
                    
                    // Default planet context
                    PlanetContext planet("earth");

                    // Determine surface/sub-surface materials based on depth from the *effective* surface
                    float depth = effectivePlanetRadius - distToPlanetCenter;

                    if (depth < 1.5f) { // Top layer - use biome-aware selection
                        blockTypeId = registry.selectBlock("azurevoxel:grass", biome, planet);
                        
                            // Biome-specific surface materials
                            if (biome.biome_id == "arctic") {
                                blockTypeId = registry.getBlockId("azurevoxel:ice");
                            } else if (biome.biome_id == "desert") {
                            blockTypeId = registry.getBlockId("azurevoxel:sand");
                            } else if (biome.biome_id == "volcanic") {
                                if (featureNoise > 0.3f) {
                                    blockTypeId = registry.getBlockId("azurevoxel:lava");
                                } else {
                                    blockTypeId = registry.getBlockId("azurevoxel:obsidian");
                                }
                            } else if (biome.biome_id == "swamp") {
                                if (featureNoise > 0.2f) {
                                    blockTypeId = registry.getBlockId("azurevoxel:mud");
                                } else {
                                    blockTypeId = registry.getBlockId("azurevoxel:grass");
                                }
                            } else if (biome.biome_id == "mountain") {
                                blockTypeId = registry.getBlockId("azurevoxel:granite");
                            } else if (biome.biome_id == "forest") {
                                // Add some variety in forest floors
                                if (featureNoise > 0.4f) {
                                    blockTypeId = registry.getBlockId("azurevoxel:moss_stone");
                                } else {
                                    blockTypeId = registry.getBlockId("azurevoxel:grass");
                                }
                            } else if (biome.biome_id == "tropical") {
                                blockTypeId = registry.getBlockId("azurevoxel:moss_stone");
                            } else if (biome.biome_id == "tundra") {
                                blockTypeId = registry.getBlockId("azurevoxel:snow");
                            } else if (biome.biome_id == "cold") {
                                blockTypeId = registry.getBlockId("azurevoxel:snow");
                            } else {
                                // Default temperate
                                blockTypeId = registry.getBlockId("azurevoxel:grass");
                            }
                            
                            // Add desert cacti
                            if (biome.biome_id == "desert" && featureNoise > 0.7f && depth < 0.5f) {
                                blockTypeId = registry.getBlockId("azurevoxel:cactus");
                            }
                            
                    } else if (depth < 5.0f) { // Sub-surface layer
                            if (biome.biome_id == "arctic" || biome.biome_id == "tundra") {
                                blockTypeId = registry.getBlockId("azurevoxel:gravel");
                            } else if (biome.biome_id == "desert") {
                                blockTypeId = registry.getBlockId("azurevoxel:sandstone");
                            } else if (biome.biome_id == "volcanic") {
                                blockTypeId = registry.getBlockId("azurevoxel:basalt");
                            } else if (biome.biome_id == "swamp") {
                                blockTypeId = registry.getBlockId("azurevoxel:clay");
                            } else if (biome.biome_id == "mountain") {
                                if (featureNoise > 0.3f) {
                                    blockTypeId = registry.getBlockId("azurevoxel:granite");
                                } else {
                                    blockTypeId = registry.getBlockId("azurevoxel:stone");
                                }
                        } else {
                            blockTypeId = registry.getBlockId("azurevoxel:dirt");
                        }
                    } else {
                            // Deeper areas - mostly stone with biome variations
                            if (biome.biome_id == "volcanic") {
                                if (featureNoise > 0.5f) {
                                    blockTypeId = registry.getBlockId("azurevoxel:basalt");
                                } else {
                                    blockTypeId = registry.getBlockId("azurevoxel:obsidian");
                                }
                            } else if (biome.biome_id == "mountain") {
                                blockTypeId = registry.getBlockId("azurevoxel:granite");
                            } else {
                        blockTypeId = registry.getBlockId("azurevoxel:stone");
                            }
                        
                            // Ore generation with biome influence
                        if (depth > 8.0f) {
                            float oreNoiseVal = glm::simplex(blockWorldPos * oreNoiseScale + glm::vec3(seed * 0.7f));
                                if (oreNoiseVal > 0.75f) {
                                blockTypeId = registry.getBlockId("azurevoxel:gold_ore");
                            }
                        }
                    }

                    // Simple water level - place water in areas where terrain is "carved out" below water level
                    float waterLevelRadius = planetRadius * 0.7f; // Water level at 70% of planet radius
                    
                        // Create lakes in low-lying areas with biome influence
                        float lakeNoise = glm::simplex(glm::vec2(blockWorldPos.x * 0.01f, blockWorldPos.z * 0.01f) + glm::vec2(seed * 1.1f));
                        bool isLakeArea = (lakeNoise < -0.4f && elevationMajorNoise < -0.2f);
                        
                        // Biome-specific water features
                        if (biome.biome_id == "swamp") {
                            // Swamps have more water
                            if (featureNoise > 0.1f && depth < 2.0f) {
                        blockTypeId = registry.getBlockId("azurevoxel:water");
                            }
                        } else if (isLakeArea && depth < 3.0f) {
                            // Natural lakes in low areas
                            blockTypeId = registry.getBlockId("azurevoxel:water");
                        } else if (distToPlanetCenter <= waterLevelRadius) {
                            // Core water level
                            blockTypeId = registry.getBlockId("azurevoxel:water");
                        }
                    }
                }

                blockDataForInitialization_[x_local][y_local][z_local].type = static_cast<unsigned char>(blockTypeId);
                if (blockTypeId != 0) {
                    blocks_[x_local][y_local][z_local] = std::make_shared<Block>(position + glm::vec3(x_local,y_local,z_local), blockTypeId, glm::vec3(0.5f), 1.0f);
                } else {
                    blocks_[x_local][y_local][z_local] = nullptr; 
                }
            }
        }
    }
}

// This is the single, complete definition of buildSurfaceMesh
void Chunk::buildSurfaceMesh(const World* /*world*/, const std::optional<glm::vec3>& pCenterOpt, const std::optional<float>& pRadiusOpt) {
    std::lock_guard<std::mutex> meshLock(meshMutex_);
    std::lock_guard<std::mutex> dataLock(dataMutex_);
    
    cleanupMesh(); 
    meshVertices.clear();
    meshIndices.clear();
    unsigned int vertexIndexOffset = 0;

    // Get reference to the block registry
    BlockRegistry& registry = BlockRegistry::getInstance();

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
                    // Thread-safe block check using data directly
                    if (blockDataForInitialization_[x_local][y_local][z_local].type == 0) continue;

                    for (int face = 0; face < 6; ++face) {
                        int nx = x_local + neighborOffsets[face][0];
                        int ny = y_local + neighborOffsets[face][1];
                        int nz = z_local + neighborOffsets[face][2];
                        bool shouldRenderFace = false;

                        uint16_t currentBlockType = static_cast<uint16_t>(blockDataForInitialization_[x_local][y_local][z_local].type);
                        if (currentBlockType == 0) continue; // Should not happen if check above was true, but defensive

                        if (nx < 0 || nx >= CHUNK_SIZE_X || ny < 0 || ny >= CHUNK_SIZE_Y || nz < 0 || nz >= CHUNK_SIZE_Z) {
                            shouldRenderFace = true; 
                        } else {
                            uint16_t neighborBlockType = static_cast<uint16_t>(blockDataForInitialization_[nx][ny][nz].type);
                            shouldRenderFace = registry.shouldRenderFace(currentBlockType, neighborBlockType);
                        }

                        if (shouldRenderFace) {
                            const BlockRenderData& renderData = registry.getRenderData(currentBlockType);
                            uint16_t textureIndex = renderData.texture_atlas_index;
                            int texturesPerRow = 10;
                            float textureSize = 80.0f; 
                            int texX = textureIndex % texturesPerRow;
                            int texY = textureIndex / texturesPerRow;
                            float uvPixelOffsetX = texX * textureSize;
                            float uvPixelOffsetY = texY * textureSize;

                            for (int i = 0; i < 4; ++i) {
                                meshVertices.push_back(x_local + faceVertices[face][i][0]);
                                meshVertices.push_back(y_local + faceVertices[face][i][1]);
                                meshVertices.push_back(z_local + faceVertices[face][i][2]);
                                if (Block::spritesheetLoaded && Block::spritesheetTexture.getWidth() > 0) {
                                    meshVertices.push_back((uvPixelOffsetX + texCoords[i].x * textureSize) / Block::spritesheetTexture.getWidth());
                                    meshVertices.push_back((uvPixelOffsetY + texCoords[i].y * textureSize) / Block::spritesheetTexture.getHeight());
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
    } else {
        const glm::vec3& planetCenter = pCenterOpt.value();
        const float planetRadius = pRadiusOpt.value();
        std::cout << "Building spherical mesh for chunk. Planet R: " << planetRadius << " Chunk Pos: (" << position.x << "," << position.y << "," << position.z << ")" << std::endl;

        for (int x_loc = 0; x_loc < CHUNK_SIZE_X; ++x_loc) {
            for (int y_loc = 0; y_loc < CHUNK_SIZE_Y; ++y_loc) {
                for (int z_loc = 0; z_loc < CHUNK_SIZE_Z; ++z_loc) {
                    if (blockDataForInitialization_[x_loc][y_loc][z_loc].type == 0) continue;

                    for (int face = 0; face < 6; ++face) {
                        int nx_loc = x_loc + neighborOffsets[face][0];
                        int ny_loc = y_loc + neighborOffsets[face][1];
                        int nz_loc = z_loc + neighborOffsets[face][2];
                        bool shouldRenderFace = false;
                        uint16_t currentBlockType = static_cast<uint16_t>(blockDataForInitialization_[x_loc][y_loc][z_loc].type);
                        if (currentBlockType == 0) continue; 

                        if (nx_loc < 0 || nx_loc >= CHUNK_SIZE_X || ny_loc < 0 || ny_loc >= CHUNK_SIZE_Y || nz_loc < 0 || nz_loc >= CHUNK_SIZE_Z) {
                            glm::vec3 neighborBlockWorldCenter = position + glm::vec3(nx_loc + 0.5f, ny_loc + 0.5f, nz_loc + 0.5f);
                            if (glm::length(neighborBlockWorldCenter - planetCenter) > planetRadius) {
                                shouldRenderFace = true; 
                            } else {
                                shouldRenderFace = true; 
                            }
                        } else {
                            uint16_t neighborBlockType = static_cast<uint16_t>(blockDataForInitialization_[nx_loc][ny_loc][nz_loc].type);
                            shouldRenderFace = registry.shouldRenderFace(currentBlockType, neighborBlockType);
                        }

                        if (shouldRenderFace) {
                            const BlockRenderData& renderData = registry.getRenderData(currentBlockType);
                            uint16_t textureIndex = renderData.texture_atlas_index;
                            int texturesPerRow = 10;
                            float textureSize = 80.0f; 
                            int texX = textureIndex % texturesPerRow;
                            int texY = textureIndex / texturesPerRow;
                            float uvPixelOffsetX = texX * textureSize;
                            float uvPixelOffsetY = texY * textureSize;

                            for (int i = 0; i < 4; ++i) {
                                glm::vec3 localFaceVertexPos = glm::vec3(x_loc + faceVertices[face][i][0],
                                                                   y_loc + faceVertices[face][i][1],
                                                                   z_loc + faceVertices[face][i][2]);
                                meshVertices.push_back(localFaceVertexPos.x);
                                meshVertices.push_back(localFaceVertexPos.y);
                                meshVertices.push_back(localFaceVertexPos.z);

                                if (Block::spritesheetLoaded && Block::spritesheetTexture.getWidth() > 0) {
                                    meshVertices.push_back((uvPixelOffsetX + texCoords[i].x * textureSize) / Block::spritesheetTexture.getWidth());
                                    meshVertices.push_back((uvPixelOffsetY + texCoords[i].y * textureSize) / Block::spritesheetTexture.getHeight());
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
        needsRebuild_.store(false); 
        return;
    }
    surfaceMesh.indexCount = static_cast<GLsizei>(meshIndices.size());
    needsRebuild_.store(false);
    std::cout << "🔧 Chunk surface mesh data prepared. Vertices: " << meshVertices.size()/5 << ", Indices: " << meshIndices.size() << std::endl;
}

// Legacy OpenGL Initialize - This was called by ensureInitialized.
// The new system uses initializeOpenGL called from main thread task queue.
void Chunk::openglInitialize(World* world) {
    // This method is intended for the legacy ensureInitialized path.
    // It attempts to create OpenGL objects if they don't exist and the mesh data is ready.
    // It's distinct from the new `initializeOpenGL` which is part of the ChunkState pipeline.

    if (isInitialized() && surfaceMesh.VAO != 0) { // Check if already fully initialized by new pipeline or this legacy one
        std::cout << "INFO: Legacy openglInitialize: Chunk already fully initialized. VAO=" << surfaceMesh.VAO << std::endl;
            return;
        }

    if (glfwGetCurrentContext() == nullptr) {
        std::cerr << "CRITICAL ERROR (Legacy openglInitialize): No OpenGL context! Cannot create GL objects for chunk at "
                  << position.x << "," << position.z << std::endl;
        return; // Cannot proceed
    }
    
    std::cout << "INFO: Legacy openglInitialize: Attempting to create GL objects for chunk at " << position.x << "," << position.z << std::endl;

    // Ensure shader and spritesheet are loaded (idempotent checks)
    if (Block::shaderProgram == 0) Block::InitBlockShader();
    if (!Block::spritesheetLoaded) Block::InitSpritesheet("res/textures/Spritesheet.PNG");

    if (Block::shaderProgram == 0 || !Block::spritesheetLoaded) {
        std::cerr << "CRITICAL ERROR (Legacy openglInitialize): Shader or spritesheet failed to load." << std::endl;
        return;
    }
    
    // This legacy path assumes buildSurfaceMesh was called just before it by ensureInitialized,
    // so meshVertices and meshIndices should be populated.
    std::lock_guard<std::mutex> meshLock(meshMutex_); // Protect meshVertices and meshIndices
    if (!meshVertices.empty() && !meshIndices.empty() && surfaceMesh.VAO == 0) {
        std::cout << "LEGACY_GL_INIT: Creating OpenGL objects for chunk mesh" << std::endl;
        while (glGetError() != GL_NO_ERROR) {} // Clear previous errors

    glGenVertexArrays(1, &surfaceMesh.VAO);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR || surfaceMesh.VAO == 0) {
            std::cerr << "CRITICAL ERROR (Legacy): Failed to generate VAO. OpenGL error: " << error << std::endl;
            surfaceMesh.VAO = 0; return;
    }
    glBindVertexArray(surfaceMesh.VAO);

    glGenBuffers(1, &surfaceMesh.VBO);
    error = glGetError();
    if (error != GL_NO_ERROR || surfaceMesh.VBO == 0) {
            std::cerr << "CRITICAL ERROR (Legacy): Failed to generate VBO. OpenGL error: " << error << std::endl;
            glDeleteVertexArrays(1, &surfaceMesh.VAO); surfaceMesh.VAO = 0; return;
    }
    glBindBuffer(GL_ARRAY_BUFFER, surfaceMesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(float), meshVertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &surfaceMesh.EBO);
    error = glGetError();
    if (error != GL_NO_ERROR || surfaceMesh.EBO == 0) {
            std::cerr << "CRITICAL ERROR (Legacy): Failed to generate EBO. OpenGL error: " << error << std::endl;
            glDeleteBuffers(1, &surfaceMesh.VBO); glDeleteVertexArrays(1, &surfaceMesh.VAO);
            surfaceMesh.VAO = 0; surfaceMesh.VBO = 0; return;
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, surfaceMesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshIndices.size() * sizeof(unsigned int), meshIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
        std::cout << "LEGACY_GL_INIT: OpenGL objects created. VAO=" << surfaceMesh.VAO << std::endl;
        surfaceMesh.indexCount = static_cast<GLsizei>(meshIndices.size()); // Ensure indexCount is set
    } else if (surfaceMesh.VAO != 0) {
         std::cout << "INFO: Legacy openglInitialize: VAO already exists (" << surfaceMesh.VAO << ")." << std::endl;
    } else {
        std::cout << "WARN: Legacy openglInitialize: Mesh data empty or VAO already 0, cannot create GL objects." << std::endl;
        // If mesh data is empty, but we reached here, it means buildMeshAsync might have found no visible faces.
        // In this case, the chunk is effectively 'initialized' but has nothing to draw.
        surfaceMesh.VAO = 0; // Ensure it's 0
        surfaceMesh.indexCount = 0;
    }

    // If VAO is now valid, we can consider this chunk fully initialized *for the legacy path*.
    if (surfaceMesh.VAO != 0) {
        state_.store(ChunkState::FULLY_INITIALIZED); // MODIFIED: From isInitialized_ = true;
        needsRebuild_.store(false); 
        std::cout << "INFO: Legacy openglInitialize: Chunk at " << position.x << "," << position.z << " marked FULLY_INITIALIZED. VAO=" << surfaceMesh.VAO << std::endl;
    } else {
        std::cout << "WARN: Legacy openglInitialize: Chunk at " << position.x << "," << position.z << " failed GL setup, not fully initialized." << std::endl;
    }
}

void Chunk::setPlanetContext(const glm::vec3& planetCenter, float planetRadius) {
    planetCenter_ = planetCenter;
    planetRadius_ = planetRadius;
    state_.store(ChunkState::UNINITIALIZED); // MODIFIED: From isInitialized_ = false;
    needsRebuild_.store(true); 
}

// New multi-threaded data generation phase
void Chunk::generateDataAsync(const World* world, int seed, const std::optional<glm::vec3>& planetCenter, const std::optional<float>& planetRadius) {
    ChunkState expected = ChunkState::UNINITIALIZED;
    if (!state_.compare_exchange_strong(expected, ChunkState::DATA_GENERATING)) {
        return; 
    }
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    planetCenter_ = planetCenter;
    planetRadius_ = planetRadius;
    
    if (blockDataForInitialization_.empty() || blockDataForInitialization_.size() != CHUNK_SIZE_X) { 
        blockDataForInitialization_.resize(CHUNK_SIZE_X,
                                 std::vector<std::vector<BlockInfo>>(
                                     CHUNK_SIZE_Y,
                                     std::vector<BlockInfo>(CHUNK_SIZE_Z)));
    }
    
    bool loadedFromFile = false;
    if (world) {
        std::string worldDataPath = "chunk_data/" + world->getWorldName();
        loadedFromFile = loadFromFile_DataOnly(worldDataPath, const_cast<World*>(world));
        if (loadedFromFile) {
            std::cout << "✓ LOADED chunk " << position.x << "," << position.y << "," << position.z << " from saved file (FAST)" << std::endl;
        } else {
            std::cout << "✗ No saved file found for chunk " << position.x << "," << position.y << "," << position.z << " - will generate" << std::endl;
        }
    }

    if (!loadedFromFile) {
        std::cout << "⚡ GENERATING chunk " << position.x << "," << position.y << "," << position.z << " (SLOW)" << std::endl;
        generateTerrainDataOnly(seed, planetCenter, planetRadius);
        if (world) {
            std::string worldDataPath = "chunk_data/" + world->getWorldName();
            if (saveToFile(worldDataPath)) {
                std::cout << "💾 Saved generated chunk " << position.x << "," << position.y << "," << position.z << " to file for future use" << std::endl;
            } else {
                std::cerr << "❌ Failed to save chunk " << position.x << "," << position.y << "," << position.z << " to file." << std::endl;
            }
        }
    }
    state_.store(ChunkState::DATA_READY);
}

// New multi-threaded mesh building phase
void Chunk::buildMeshAsync(const World* world) {
    ChunkState expected = ChunkState::DATA_READY;
    if (!state_.compare_exchange_strong(expected, ChunkState::MESH_BUILDING)) {
        return; 
    }
    std::cout << "🔧 BUILDING mesh for chunk " << position.x << "," << position.y << "," << position.z << std::endl;
    buildSurfaceMesh(world, planetCenter_, planetRadius_);
    state_.store(ChunkState::MESH_READY);
}

// OpenGL initialization (main thread only - THIS IS THE NEW SYSTEM'S METHOD)
void Chunk::initializeOpenGL(World* world) {
    ChunkState expected = ChunkState::MESH_READY;
    if (!state_.compare_exchange_strong(expected, ChunkState::OPENGL_INITIALIZING)) {
        // If already initializing or initialized, or not ready for mesh, then return.
        if (state_.load() == ChunkState::OPENGL_INITIALIZING || state_.load() == ChunkState::FULLY_INITIALIZED) {
            // std::cout << "DEBUG: initializeOpenGL: Already initializing or initialized for chunk " << position.x << "," << position.z << std::endl;
        } else {
            // std::cout << "DEBUG: initializeOpenGL: Not in MESH_READY state for chunk " << position.x << "," << position.z << ". Current state: " << static_cast<int>(state_.load()) << std::endl;
        }
        return; 
    }

    if (glfwGetCurrentContext() == nullptr) {
        std::cerr << "CRITICAL ERROR (initializeOpenGL): No OpenGL context current on main thread! Chunk: " 
                  << position.x << "," << position.z << std::endl;
        state_.store(ChunkState::MESH_READY); // Revert state
        return;
    }

    std::cout << "🎨 OpenGL-Initializing chunk at " << position.x << "," << position.z << " (New Pipeline)" << std::endl;
    
    // Initialize shader and texture if needed
    if (Block::shaderProgram == 0) {
        Block::InitBlockShader();
        if (Block::shaderProgram == 0) {
            std::cerr << "CRITICAL ERROR (initializeOpenGL): Failed to initialize block shader program!" << std::endl;
            state_.store(ChunkState::MESH_READY); // Revert state
            return;
        }
    }
    if (!Block::spritesheetLoaded) { 
        Block::InitSpritesheet("res/textures/Spritesheet.PNG");
        if (!Block::spritesheetLoaded) {
            std::cerr << "CRITICAL ERROR (initializeOpenGL): Failed to load global spritesheet!" << std::endl;
            // Continue, but textures might be wrong
        }
    }

    // Create Block objects from data
    {
        std::lock_guard<std::mutex> dataLock(dataMutex_); // Protects blockDataForInitialization_
        std::lock_guard<std::mutex> blocksLock(meshMutex_); // Assuming blocks_ might also need protection if accessed elsewhere, better safe. For now, meshMutex_ also covers blocks_ creation.
    for (int x = 0; x < CHUNK_SIZE_X; ++x) {
        for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
            for (int z = 0; z < CHUNK_SIZE_Z; ++z) {
                BlockInfo info = blockDataForInitialization_[x][y][z];
                glm::vec3 blockWorldPos = this->position + glm::vec3(x, y, z);
                if (info.type != 0 && blocks_[x][y][z] == nullptr) { 
                        blocks_[x][y][z] = std::make_shared<Block>(blockWorldPos, static_cast<uint16_t>(info.type), glm::vec3(0.5f), 1.0f);
                } else if (info.type == 0 && blocks_[x][y][z] != nullptr) { 
                    blocks_[x][y][z] = nullptr;
                    }
                }
            }
        }
    }

    // Create OpenGL objects from the prepared mesh data
    {
        std::lock_guard<std::mutex> meshLock(meshMutex_); // Protects meshVertices, meshIndices, and surfaceMesh
        if (!meshVertices.empty() && !meshIndices.empty()) {
            std::cout << "🎨 Creating OpenGL objects for chunk mesh (New Pipeline)" << std::endl;
            while (glGetError() != GL_NO_ERROR) {} // Clear previous errors
            
            glGenVertexArrays(1, &surfaceMesh.VAO);
            GLenum error = glGetError();
            if (error != GL_NO_ERROR || surfaceMesh.VAO == 0) {
                std::cerr << "CRITICAL ERROR (initializeOpenGL): Failed to generate VAO. OpenGL error: " << error << " VAO ID: " << surfaceMesh.VAO << std::endl;
                surfaceMesh.VAO = 0; 
                state_.store(ChunkState::MESH_READY); // Revert state
                return;
            }
            glBindVertexArray(surfaceMesh.VAO);

            glGenBuffers(1, &surfaceMesh.VBO);
            error = glGetError();
            if (error != GL_NO_ERROR || surfaceMesh.VBO == 0) {
                std::cerr << "CRITICAL ERROR (initializeOpenGL): Failed to generate VBO. OpenGL error: " << error << " VBO ID: " << surfaceMesh.VBO << std::endl;
                glDeleteVertexArrays(1, &surfaceMesh.VAO); surfaceMesh.VAO = 0;
                surfaceMesh.VBO = 0;
                state_.store(ChunkState::MESH_READY); // Revert state
                return;
            }
            glBindBuffer(GL_ARRAY_BUFFER, surfaceMesh.VBO);
            glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(float), meshVertices.data(), GL_STATIC_DRAW);

            glGenBuffers(1, &surfaceMesh.EBO);
            error = glGetError();
            if (error != GL_NO_ERROR || surfaceMesh.EBO == 0) {
                std::cerr << "CRITICAL ERROR (initializeOpenGL): Failed to generate EBO. OpenGL error: " << error << " EBO ID: " << surfaceMesh.EBO << std::endl;
                glDeleteBuffers(1, &surfaceMesh.VBO); glDeleteVertexArrays(1, &surfaceMesh.VAO);
                surfaceMesh.VAO = 0; surfaceMesh.VBO = 0; surfaceMesh.EBO = 0;
                state_.store(ChunkState::MESH_READY); // Revert state
                return;
            }
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, surfaceMesh.EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshIndices.size() * sizeof(unsigned int), meshIndices.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            surfaceMesh.indexCount = static_cast<GLsizei>(meshIndices.size()); // Make sure indexCount is set
            std::cout << "🎨 OpenGL objects created successfully (New Pipeline). VAO=" << surfaceMesh.VAO << std::endl;
        } else if (surfaceMesh.VAO != 0) {
             std::cout << "DEBUG: initializeOpenGL: VAO already exists and mesh data is empty. This might be okay if already initialized." << std::endl;
        } else {
            std::cout << "WARN: initializeOpenGL: Mesh data (vertices/indices) is empty. Cannot create GL objects. Chunk at " << position.x << "," << position.z << std::endl;
            // If mesh data is empty, but we reached here, it means buildMeshAsync might have found no visible faces.
            // In this case, the chunk is effectively 'initialized' but has nothing to draw.
            surfaceMesh.VAO = 0; // Ensure it's 0
            surfaceMesh.indexCount = 0;
        }
    }
    
    needsRebuild_.store(false);
    state_.store(ChunkState::FULLY_INITIALIZED);
    std::cout << "✅ Chunk at " << position.x << "," << position.z << " fully initialized (New Pipeline). VAO=" << surfaceMesh.VAO << std::endl;
}

// generateTerrainDataOnly is used by the new system (generateDataAsync)
void Chunk::generateTerrainDataOnly(int seed, const std::optional<glm::vec3>& pCenterOpt, const std::optional<float>& pRadiusOpt) {
    std::cout << "Chunk at " << position.x << "," << position.y << "," << position.z << " generateTerrainDataOnly. Planet context: " << (pCenterOpt.has_value() ? "Yes" : "No") << std::endl;

    // Get reference to the block registry
    BlockRegistry& registry = BlockRegistry::getInstance();

    if (blockDataForInitialization_.empty() || blockDataForInitialization_.size() != CHUNK_SIZE_X ) { 
        blockDataForInitialization_.resize(CHUNK_SIZE_X,
                                 std::vector<std::vector<BlockInfo>>(
                                     CHUNK_SIZE_Y,
                                     std::vector<BlockInfo>(CHUNK_SIZE_Z)));
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
                    uint16_t blockTypeId = 0; // Default to air
                    
                    if (y_local < terrainHeight - 1) {
                        // Use registry to get stone block ID
                        blockTypeId = registry.getBlockId("azurevoxel:stone");
                    } else if (y_local == terrainHeight - 1) {
                        // Use registry to get grass block ID
                        blockTypeId = registry.getBlockId("azurevoxel:grass");
                    }
                    
                    blockDataForInitialization_[x_local][y_local][z_local].type = static_cast<unsigned char>(blockTypeId);
                }
            }
        }
        return;
    }

    // Spherical generation logic with biome-aware block selection
    const glm::vec3& planetCenter = pCenterOpt.value();
    const float planetRadius = pRadiusOpt.value();
    std::cout << "Generating spherical terrain for chunk. Planet R: " << planetRadius << " Center: (" << planetCenter.x << "," << planetCenter.y << "," << planetCenter.z << ")" << std::endl;
    
    // Noise parameters for variety
    float elevationNoiseScale = 0.02f; // Controls variation in "surface" height
    float oreNoiseScale = 0.1f; // For ore generation
    
    // Additional noise layers for diverse terrain features
    float temperatureNoiseScale = 0.03f; // Large-scale temperature variation
    float moistureNoiseScale = 0.04f; // Large-scale moisture variation
    float elevationMajorScale = 0.01f; // Large-scale elevation (mountains/valleys)
    float featureNoiseScale = 0.08f; // Small-scale features (lakes, forests)

    for (int x_local = 0; x_local < CHUNK_SIZE_X; ++x_local) {
        for (int y_local = 0; y_local < CHUNK_SIZE_Y; ++y_local) {
            for (int z_local = 0; z_local < CHUNK_SIZE_Z; ++z_local) {
                glm::vec3 blockLocalPos = glm::vec3(x_local + 0.5f, y_local + 0.5f, z_local + 0.5f);
                glm::vec3 blockWorldPos = position + blockLocalPos; 
                float distToPlanetCenter = glm::length(blockWorldPos - planetCenter);

                uint16_t blockTypeId = 0; // Default to air

                // Determine a slightly varied "surface" radius using noise
                float elevationNoise = glm::simplex(glm::vec2(blockWorldPos.x * elevationNoiseScale, blockWorldPos.z * elevationNoiseScale) + glm::vec2(seed * 0.1f, seed * -0.1f));
                float effectivePlanetRadius = planetRadius + elevationNoise * 2.0f; // +/- 2 blocks height variation

                if (distToPlanetCenter <= effectivePlanetRadius) {
                    // Multi-layer noise for sophisticated biome generation
                    float temperatureNoise = glm::simplex(glm::vec2(blockWorldPos.x * temperatureNoiseScale, blockWorldPos.z * temperatureNoiseScale) + glm::vec2(seed * 0.2f));
                    float moistureNoise = glm::simplex(glm::vec2(blockWorldPos.x * moistureNoiseScale, blockWorldPos.z * moistureNoiseScale) + glm::vec2(seed * 0.4f));
                    float elevationMajorNoise = glm::simplex(glm::vec2(blockWorldPos.x * elevationMajorScale, blockWorldPos.z * elevationMajorScale) + glm::vec2(seed * 0.6f));
                    float featureNoise = glm::simplex(glm::vec3(blockWorldPos * featureNoiseScale + glm::vec3(seed * 0.3f)));
                    
                    // Adjust effective radius based on major elevation features (mountains/valleys)
                    effectivePlanetRadius += elevationMajorNoise * 15.0f; // +/- 15 blocks for major terrain features
                    
                    // Recalculate if block is still within the adjusted surface
                    if (distToPlanetCenter > effectivePlanetRadius) {
                        blockTypeId = 0; // Air
                    } else {
                        // Determine biome based on multiple noise layers
                    BiomeContext biome;
                        
                        // Combine temperature and moisture for biome selection
                        float temperature = temperatureNoise * 0.7f + elevationMajorNoise * -0.3f; // Higher elevation = colder
                        float moisture = moistureNoise * 0.8f + featureNoise * 0.2f;
                        
                        // Determine primary biome
                        if (temperature < -0.6f) {
                            if (moisture < -0.3f) {
                                biome = BiomeContext("arctic", -0.9f, 0.1f); // Arctic
                    } else {
                                biome = BiomeContext("tundra", -0.6f, 0.4f); // Tundra
                            }
                        } else if (temperature < -0.2f) {
                            if (moisture < 0.0f) {
                                biome = BiomeContext("cold", -0.7f, 0.3f); // Cold
                            } else {
                                biome = BiomeContext("forest", 0.3f, 0.7f); // Cold forest
                            }
                        } else if (temperature < 0.3f) {
                            if (moisture < -0.4f) {
                                biome = BiomeContext("temperate", 0.2f, 0.5f); // Temperate plains
                            } else if (moisture > 0.6f) {
                                biome = BiomeContext("swamp", 0.4f, 0.9f); // Swamp
                            } else {
                                biome = BiomeContext("forest", 0.3f, 0.7f); // Temperate forest
                            }
                        } else if (temperature < 0.7f) {
                            if (moisture < -0.5f) {
                                biome = BiomeContext("desert", 0.9f, -0.8f); // Desert
                            } else {
                                biome = BiomeContext("tropical", 0.7f, 0.8f); // Tropical
                            }
                        } else {
                            if (moisture < -0.3f) {
                                biome = BiomeContext("volcanic", 1.0f, -0.5f); // Volcanic
                            } else {
                                biome = BiomeContext("hot", 0.8f, -0.3f); // Hot
                            }
                        }
                        
                        // Special mountain biome for high elevation areas
                        if (elevationMajorNoise > 0.4f) {
                            biome = BiomeContext("mountain", -0.3f, 0.2f);
                    }
                    
                    // Default planet context
                    PlanetContext planet("earth");

                    // Determine surface/sub-surface materials based on depth from the *effective* surface
                    float depth = effectivePlanetRadius - distToPlanetCenter;

                    if (depth < 1.5f) { // Top layer - use biome-aware selection
                        blockTypeId = registry.selectBlock("azurevoxel:grass", biome, planet);
                        
                            // Biome-specific surface materials
                            if (biome.biome_id == "arctic") {
                                blockTypeId = registry.getBlockId("azurevoxel:ice");
                            } else if (biome.biome_id == "desert") {
                            blockTypeId = registry.getBlockId("azurevoxel:sand");
                            } else if (biome.biome_id == "volcanic") {
                                if (featureNoise > 0.3f) {
                                    blockTypeId = registry.getBlockId("azurevoxel:lava");
                                } else {
                                    blockTypeId = registry.getBlockId("azurevoxel:obsidian");
                                }
                            } else if (biome.biome_id == "swamp") {
                                if (featureNoise > 0.2f) {
                                    blockTypeId = registry.getBlockId("azurevoxel:mud");
                                } else {
                                    blockTypeId = registry.getBlockId("azurevoxel:grass");
                                }
                            } else if (biome.biome_id == "mountain") {
                                blockTypeId = registry.getBlockId("azurevoxel:granite");
                            } else if (biome.biome_id == "forest") {
                                // Add some variety in forest floors
                                if (featureNoise > 0.4f) {
                                    blockTypeId = registry.getBlockId("azurevoxel:moss_stone");
                                } else {
                                    blockTypeId = registry.getBlockId("azurevoxel:grass");
                                }
                            } else if (biome.biome_id == "tropical") {
                                blockTypeId = registry.getBlockId("azurevoxel:moss_stone");
                            } else if (biome.biome_id == "tundra") {
                                blockTypeId = registry.getBlockId("azurevoxel:snow");
                            } else if (biome.biome_id == "cold") {
                                blockTypeId = registry.getBlockId("azurevoxel:snow");
                            } else {
                                // Default temperate
                                blockTypeId = registry.getBlockId("azurevoxel:grass");
                            }
                            
                            // Add desert cacti
                            if (biome.biome_id == "desert" && featureNoise > 0.7f && depth < 0.5f) {
                                blockTypeId = registry.getBlockId("azurevoxel:cactus");
                            }
                            
                    } else if (depth < 5.0f) { // Sub-surface layer
                            if (biome.biome_id == "arctic" || biome.biome_id == "tundra") {
                                blockTypeId = registry.getBlockId("azurevoxel:gravel");
                            } else if (biome.biome_id == "desert") {
                                blockTypeId = registry.getBlockId("azurevoxel:sandstone");
                            } else if (biome.biome_id == "volcanic") {
                                blockTypeId = registry.getBlockId("azurevoxel:basalt");
                            } else if (biome.biome_id == "swamp") {
                                blockTypeId = registry.getBlockId("azurevoxel:clay");
                            } else if (biome.biome_id == "mountain") {
                                if (featureNoise > 0.3f) {
                                    blockTypeId = registry.getBlockId("azurevoxel:granite");
                                } else {
                                    blockTypeId = registry.getBlockId("azurevoxel:stone");
                                }
                        } else {
                            blockTypeId = registry.getBlockId("azurevoxel:dirt");
                        }
                    } else {
                            // Deeper areas - mostly stone with biome variations
                            if (biome.biome_id == "volcanic") {
                                if (featureNoise > 0.5f) {
                                    blockTypeId = registry.getBlockId("azurevoxel:basalt");
                                } else {
                                    blockTypeId = registry.getBlockId("azurevoxel:obsidian");
                                }
                            } else if (biome.biome_id == "mountain") {
                                blockTypeId = registry.getBlockId("azurevoxel:granite");
                            } else {
                        blockTypeId = registry.getBlockId("azurevoxel:stone");
                            }
                        
                            // Ore generation with biome influence
                        if (depth > 8.0f) {
                            float oreNoiseVal = glm::simplex(blockWorldPos * oreNoiseScale + glm::vec3(seed * 0.7f));
                                if (oreNoiseVal > 0.75f) {
                                blockTypeId = registry.getBlockId("azurevoxel:gold_ore");
                            }
                        }
                    }

                    // Simple water level - place water in areas where terrain is "carved out" below water level
                    float waterLevelRadius = planetRadius * 0.7f; // Water level at 70% of planet radius
                    
                        // Create lakes in low-lying areas with biome influence
                        float lakeNoise = glm::simplex(glm::vec2(blockWorldPos.x * 0.01f, blockWorldPos.z * 0.01f) + glm::vec2(seed * 1.1f));
                        bool isLakeArea = (lakeNoise < -0.4f && elevationMajorNoise < -0.2f);
                        
                        // Biome-specific water features
                        if (biome.biome_id == "swamp") {
                            // Swamps have more water
                            if (featureNoise > 0.1f && depth < 2.0f) {
                        blockTypeId = registry.getBlockId("azurevoxel:water");
                            }
                        } else if (isLakeArea && depth < 3.0f) {
                            // Natural lakes in low areas
                            blockTypeId = registry.getBlockId("azurevoxel:water");
                        } else if (distToPlanetCenter <= waterLevelRadius) {
                            // Core water level
                            blockTypeId = registry.getBlockId("azurevoxel:water");
                        }
                    }
                }

                blockDataForInitialization_[x_local][y_local][z_local].type = static_cast<unsigned char>(blockTypeId);
                if (blockTypeId != 0) {
                    blocks_[x_local][y_local][z_local] = std::make_shared<Block>(position + glm::vec3(x_local,y_local,z_local), blockTypeId, glm::vec3(0.5f), 1.0f);
                } else {
                    blocks_[x_local][y_local][z_local] = nullptr; 
                }
            }
        }
    }
}



