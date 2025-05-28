#pragma once

#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include "block.h"
#include "shader.h"
#include <optional> // For optional planet context
#include <atomic>
#include <mutex>

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

// Chunk processing states for multi-threading
enum class ChunkState {
    UNINITIALIZED,      // Just created, no data
    DATA_GENERATING,    // Terrain generation in progress (worker thread)
    DATA_READY,         // Terrain data ready, needs mesh building
    MESH_BUILDING,      // Mesh building in progress (worker thread)
    MESH_READY,         // Mesh data ready, needs OpenGL initialization
    OPENGL_INITIALIZING,// OpenGL objects being created (main thread)
    FULLY_INITIALIZED   // Ready for rendering
};

class Chunk : public std::enable_shared_from_this<Chunk> {
private:
    // Chunk position in world space (position of the first block)
    glm::vec3 position;
    
    // Thread-safe state management
    std::atomic<ChunkState> state_;
    mutable std::mutex dataMutex_;  // Protects block data access
    mutable std::mutex meshMutex_;  // Protects mesh data access
    
    // Store BlockInfo during the data-only phase (populated by worker thread)
    std::vector<std::vector<std::vector<BlockInfo>>> blockDataForInitialization_;
    
    // Store actual Block objects (populated by main thread in openglInitialize)
    std::vector<std::vector<std::vector<std::shared_ptr<Block>>>> blocks_;
    
    // Flag to indicate if chunk mesh needs to be rebuilt
    std::atomic<bool> needsRebuild_;
    
    // Mesh data for rendering visible faces
    ChunkMesh surfaceMesh;
    
    // Vertex and index data for mesh building (protected by meshMutex_)
    std::vector<float> meshVertices;
    std::vector<unsigned int> meshIndices;
    
    // Planet context (optional)
    std::optional<glm::vec3> planetCenter_;
    std::optional<float> planetRadius_;
    
    // Check if a block exists at local chunk coordinates (thread-safe)
    bool hasBlockAtLocal(int x, int y, int z) const;
    
    // Build the surface mesh for the chunk (can be called from worker thread)
    void buildSurfaceMesh(const World* world, const std::optional<glm::vec3>& planetCenter, const std::optional<float>& planetRadius);
    
    // Helper to add a face to the mesh data vectors
    void addFace(const glm::vec3& corner, const glm::vec3& side1, const glm::vec3& side2, 
                 std::vector<float>& vertices, std::vector<unsigned int>& indices, 
                 const std::vector<float>& faceTexCoords);

    // Helper to generate the chunk's filename for saving/loading
    std::string getChunkFileName() const;

    // Data-only preparation methods (called by worker thread)
    void generateTerrainDataOnly(int seed, const std::optional<glm::vec3>& planetCenter, const std::optional<float>& planetRadius);
    bool loadFromFile_DataOnly(const std::string& directoryPath, World* world);

public:
    // Constructor
    Chunk(const glm::vec3& position);
    
    // Destructor
    ~Chunk();
    
    // Multi-threaded initialization phases
    void generateDataAsync(const World* world, int seed, const std::optional<glm::vec3>& planetCenter = std::nullopt, const std::optional<float>& planetRadius = std::nullopt);
    void buildMeshAsync(const World* world);
    void initializeOpenGL(World* world);
    
    // Legacy methods for compatibility
    void init(const World* world);
    void generateTerrain(int seed, const std::optional<glm::vec3>& planetCenter, const std::optional<float>& planetRadius);
    void ensureInitialized(const World* world, int seed, const std::optional<glm::vec3>& planetCenter = std::nullopt, const std::optional<float>& planetRadius = std::nullopt);

    // State management
    ChunkState getState() const { return state_.load(); }
    bool isReadyForRendering() const { return state_.load() == ChunkState::FULLY_INITIALIZED; }
    bool isInitialized() const { return state_.load() == ChunkState::FULLY_INITIALIZED; }
    
    // Render the chunk's surface mesh
    void renderSurface(const glm::mat4& projection, const glm::mat4& view, bool wireframeState) const;
    
    // Render all blocks individually (for the current player chunk)
    void renderAllBlocks(const glm::mat4& projection, const glm::mat4& view);
    
    // Get block at local chunk coordinates (thread-safe)
    std::shared_ptr<Block> getBlockAtLocal(int x, int y, int z) const;
    
    // Set block at local chunk coordinates (marks for rebuild, thread-safe)
    void setBlockAtLocal(int x, int y, int z, std::shared_ptr<Block> block);
    
    // Remove block at local chunk coordinates (marks for rebuild, thread-safe)
    void removeBlockAtLocal(int x, int y, int z);
    
    // Get chunk position
    glm::vec3 getPosition() const;
    
    // Check if the mesh needs rebuilding
    bool needsMeshRebuild() const { return needsRebuild_.load(); }
    void markMeshRebuilt() { needsRebuild_.store(false); }
    
    // Cleanup OpenGL resources
    void cleanupMesh();

    // Save and load chunk data
    bool saveToFile(const std::string& directoryPath) const;

    // Method to set planet context
    void setPlanetContext(const glm::vec3& planetCenter, float planetRadius);

    GLuint getSurfaceMeshVAO() const { return surfaceMesh.VAO; }

    // OpenGL-specific initialization, should be called from the main thread.
    void openglInitialize(World* world);
};
