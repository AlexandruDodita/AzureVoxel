#include "../headers/world.h"
#include <iostream>
#include <cmath>

World::World(int renderDistance) : renderDistance(renderDistance) {
}

World::~World() {
    // Smart pointers will handle cleanup
}

void World::init(int gridSize) {
    // Create a grid of chunks centered at (0,0)
    int halfGrid = gridSize / 2;
    
    // Generate chunks in a grid
    for (int x = -halfGrid; x <= halfGrid; x++) {
        for (int z = -halfGrid; z <= halfGrid; z++) {
            addChunk(x, z);
        }
    }
    
    // Build meshes after all chunks are created (important for neighbor checks)
    std::cout << "Building chunk meshes..." << std::endl;
    for (auto const& [pos, chunk] : chunks) {
        chunk->init(this); // Pass world pointer for neighbor checks
    }
    
    // Display statistics
    int totalBlocks = chunks.size() * CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;
    std::cout << "World initialized with " << chunks.size() << " chunks" << std::endl;
    std::cout << "Total blocks in loaded chunks: " << totalBlocks << std::endl;
    std::cout << "Render distance: " << renderDistance << " chunks" << std::endl;
}

void World::render(const glm::mat4& projection, const glm::mat4& view, const Camera& camera) {
    // Calculate which chunk the camera is in
    glm::ivec2 camChunkPos = worldToChunkCoords(camera.getPosition());
    
    // Determine chunks to render based on render distance
    int renderedChunks = 0;
    
    for (auto const& [pos, chunk] : chunks) {
        // Calculate Manhattan distance (faster than Euclidean) for chunk culling
        int dist = std::abs(pos.x - camChunkPos.x) + std::abs(pos.y - camChunkPos.y);
        
        // Only process chunks within render distance
        if (dist <= renderDistance) {
            // Check if the chunk needs its mesh rebuilt
            if (chunk->needsMeshRebuild()) {
                 chunk->init(this); // Rebuild mesh if necessary (passing world)
            }
            
            // Determine if this is the chunk the player is currently in
            bool isCurrentChunk = (pos == camChunkPos);
            
            if (isCurrentChunk) {
                // Render the current chunk fully by drawing all its blocks
                chunk->renderAllBlocks(projection, view);
            } else {
                // Render other chunks using their optimized surface mesh
                chunk->renderSurface(projection, view);
            }
            
            renderedChunks++;
        }
    }
    
    // Uncomment for debugging:
    // std::cout << "\rRendered " << renderedChunks << " of " << chunks.size() 
    //           << " chunks. Current: (" << camChunkPos.x << "," << camChunkPos.y << ")" << std::flush;
}

std::shared_ptr<Chunk> World::getChunkAt(int chunkX, int chunkZ) {
    glm::ivec2 pos(chunkX, chunkZ);
    auto it = chunks.find(pos);
    if (it != chunks.end()) {
        return it->second;
    }
    return nullptr;
}

void World::addChunk(int chunkX, int chunkZ) {
    glm::ivec2 pos(chunkX, chunkZ);
    
    // Don't add if chunk already exists
    if (chunks.find(pos) != chunks.end()) {
        return;
    }
    
    // Calculate world position of chunk (bottom corner)
    glm::vec3 worldPos(chunkX * chunkSize, 0, chunkZ * chunkSize);
    
    // Create the chunk (mesh will be built later in init)
    auto chunk = std::make_shared<Chunk>(worldPos);
    // chunk->init(this); // Don't init here, do it after all chunks exist
    
    // Add to chunk map
    chunks[pos] = chunk;
}

glm::ivec2 World::worldToChunkCoords(const glm::vec3& worldPos) const {
    // Convert world position to chunk coordinates (floor division)
    int chunkX = static_cast<int>(std::floor(worldPos.x / chunkSize));
    int chunkZ = static_cast<int>(std::floor(worldPos.z / chunkSize));
    return glm::ivec2(chunkX, chunkZ);
}

std::shared_ptr<Block> World::getBlockAtWorldPos(int worldX, int worldY, int worldZ) const {
    // Calculate chunk coordinates
    int chunkX = static_cast<int>(std::floor(static_cast<float>(worldX) / chunkSize));
    int chunkZ = static_cast<int>(std::floor(static_cast<float>(worldZ) / chunkSize));

    // Get the chunk
    glm::ivec2 pos(chunkX, chunkZ);
    auto it = chunks.find(pos);
    if (it == chunks.end()) {
        return nullptr; // Chunk doesn't exist
    }
    const std::shared_ptr<Chunk>& chunk = it->second;

    // Calculate local block coordinates within the chunk
    int localX = worldX - chunkX * chunkSize;
    int localY = worldY; // Y coordinate doesn't affect chunk X/Z
    int localZ = worldZ - chunkZ * chunkSize;

    // Ensure Y is within valid block range (adjust if your world has height limits)
    if (localY < 0 || localY >= CHUNK_SIZE_Y) { 
        return nullptr; 
    }

    return chunk->getBlockAtLocal(localX, localY, localZ);
} 