#pragma once

#include <vector>
#include <memory>
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

class Chunk {
private:
    // Chunk position in world space (position of the first block)
    glm::vec3 position;
    
    // 3D array of blocks
    std::vector<std::vector<std::vector<std::shared_ptr<Block>>>> blocks;
    
    // Flag to indicate if chunk mesh needs to be rebuilt
    bool needsRebuild;
    
    // Mesh data for rendering visible faces
    ChunkMesh surfaceMesh;
    
    // Check if a block exists at local chunk coordinates
    bool hasBlockAtLocal(int x, int y, int z) const;
    
    // Build the surface mesh for the chunk
    void buildSurfaceMesh(const World* world);
    
    // Helper to add a face to the mesh data vectors
    void addFace(const glm::vec3& corner, const glm::vec3& side1, const glm::vec3& side2, 
                 std::vector<float>& vertices, std::vector<unsigned int>& indices, 
                 const std::vector<float>& faceTexCoords);

public:
    // Constructor
    Chunk(const glm::vec3& position);
    
    // Destructor
    ~Chunk();
    
    // Initialize the chunk (generates blocks and builds initial mesh)
    void init(const World* world);
    
    // Generate terrain for the chunk (fills the entire chunk with blocks)
    void generateTerrain();
    
    // Render the chunk's surface mesh
    void renderSurface(const glm::mat4& projection, const glm::mat4& view);
    
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
};
