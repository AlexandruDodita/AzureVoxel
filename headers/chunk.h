#pragma once

#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include "block.h"
#include "shader.h"

// Constants for chunk dimensions
constexpr int CHUNK_SIZE_X = 16;
constexpr int CHUNK_SIZE_Y = 16;
constexpr int CHUNK_SIZE_Z = 16;

// Forward declaration
class World;

struct ChunkMesh {
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLsizei indexCount = 0;
};

// NEW: Simple struct to hold block type information during data-only phase
struct BlockInfo {
    int type = 0; // 0 for air, 1 for stone, 2 for grass, etc.
    // Add other non-OpenGL properties if needed (e.g. light level from world gen)
};

class Chunk : public std::enable_shared_from_this<Chunk> {
private:
    // Chunk position in world space (position of the first block)
    glm::vec3 position;
    
    // Store BlockInfo during the data-only phase (populated by worker thread)
    std::vector<std::vector<std::vector<BlockInfo>>> blockDataForInitialization_;
    
    // Store actual Block objects (populated by main thread in openglInitialize)
    std::vector<std::vector<std::vector<std::shared_ptr<Block>>>> blocks_;
    
    // Flag to indicate if chunk mesh needs to be rebuilt
    bool needsRebuild;
    
    bool isInitialized_; // Tracks if ensureInitialized has run
    
    // Mesh data for rendering visible faces
    ChunkMesh surfaceMesh;
    
    // Vertex and index data for mesh building
    std::vector<float> meshVertices;
    std::vector<unsigned int> meshIndices;
    
    // Check if a block exists at local chunk coordinates
    bool hasBlockAtLocal(int x, int y, int z) const;
    
    // Build the surface mesh for the chunk
    void buildSurfaceMesh(const World* world);
    
    // Helper to add a face to the mesh data vectors
    void addFace(const glm::vec3& corner, const glm::vec3& side1, const glm::vec3& side2, 
                 std::vector<float>& vertices, std::vector<unsigned int>& indices, 
                 const std::vector<float>& faceTexCoords);

    // Helper to generate the chunk's filename for saving/loading
    std::string getChunkFileName() const;

    // New method for OpenGL-dependent initialization (called by main thread)
    void openglInitialize(World* world);

    // Data-only preparation methods (called by worker thread)
    void prepareBlockData(World* world, int seed, const std::string& worldDataPath);
    void generateTerrain_DataOnly(int seed);
    bool loadFromFile_DataOnly(const std::string& directoryPath, World* world);

public:
    // Constructor
    Chunk(const glm::vec3& position);
    
    // Destructor
    ~Chunk();
    
    // Initialize the chunk (generates blocks and builds initial mesh)
    void init(const World* world);
    
    // Generate terrain for the chunk (fills the entire chunk with blocks)
    void generateTerrain(int seed);

    // Ensure the chunk is initialized (loaded or generated and meshed)
    void ensureInitialized(World* world, int seed, const std::string& worldDataPath);

    // Check if the chunk has been initialized
    bool isInitialized() const;
    
    // Render the chunk's surface mesh
    void renderSurface(const glm::mat4& projection, const glm::mat4& view) const;
    
    // Render all blocks individually (for the current player chunk)
    void renderAllBlocks(const glm::mat4& projection, const glm::mat4& view);
    
    // Get block at local chunk coordinates
    std::shared_ptr<Block> getBlockAtLocal(int x, int y, int z) const;
    
    // Set block at local chunk coordinates (marks for rebuild)
    void setBlockAtLocal(int x, int y, int z, std::shared_ptr<Block> block);
    
    // Remove block at local chunk coordinates (marks for rebuild)
    void removeBlockAtLocal(int x, int y, int z);
    
    // Get chunk position
    glm::vec3 getPosition() const;
    
    // Check if the mesh needs rebuilding
    bool needsMeshRebuild() const { return needsRebuild; }
    void markMeshRebuilt() { needsRebuild = false; }
    
    // Cleanup OpenGL resources
    void cleanupMesh();

    // Save and load chunk data
    bool saveToFile(const std::string& directoryPath) const;
    // loadFromFile is effectively replaced by _DataOnly and openglInitialize phases
    // bool loadFromFile(const std::string& directoryPath, const World* world);
};
