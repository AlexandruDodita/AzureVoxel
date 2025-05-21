#include "../headers/world.h"
#include <iostream>
#include <cmath>
#include <sys/stat.h> // For mkdir
#include <set> // For std::set in update method

const std::string CHUNK_DATA_DIR = "chunk_data";

World::World(int renderDistance) : renderDistance(renderDistance) {
    // Create chunk_data directory if it doesn't exist
    mkdir(CHUNK_DATA_DIR.c_str(), 0755); // Unix-like
    // For Windows, you might need #include <direct.h> and _mkdir(CHUNK_DATA_DIR.c_str());

    worldSeed_ = 12345; // Example hardcoded seed
    worldDataPath_ = CHUNK_DATA_DIR;
}

World::~World() {
    saveAllChunks();
    // Smart pointers will handle chunk cleanup
}

void World::update(const Camera& camera) {
    glm::ivec2 playerChunkPos = worldToChunkCoords(camera.getPosition());
    std::set<glm::ivec2, IVec2Compare> activeChunkCoords;
    
    // Determine new active zone and load/generate chunks
    for (int x = playerChunkPos.x - renderDistance; x <= playerChunkPos.x + renderDistance; ++x) {
        for (int z = playerChunkPos.y - renderDistance; z <= playerChunkPos.y + renderDistance; ++z) {
            glm::ivec2 currentChunkKey(x, z);
            activeChunkCoords.insert(currentChunkKey);

            if (chunks.find(currentChunkKey) == chunks.end()) { // Chunk not loaded
                glm::vec3 chunkWorldPos(x * chunkSize, 0.0f, z * chunkSize);
                auto newChunk = std::make_shared<Chunk>(chunkWorldPos);
                
                // ensureInitialized will try to load, then generate and save if not found.
                // It also handles initial mesh building.
                newChunk->ensureInitialized(this, worldSeed_, worldDataPath_);
                
                chunks[currentChunkKey] = newChunk;
            }
        }
    }

    // Unload chunks outside the active zone
    for (auto it = chunks.begin(); it != chunks.end(); /* no increment */) {
        if (activeChunkCoords.find(it->first) == activeChunkCoords.end()) {
            // Chunk is outside active zone, save and unload
            // std::cout << "Unloading chunk at: " << it->first.x << ", " << it->first.y << std::endl;
            it->second->saveToFile(CHUNK_DATA_DIR); 
            it = chunks.erase(it); // Erase and get iterator to next element
        } else {
            ++it;
        }
    }
}

void World::render(const glm::mat4& projection, const glm::mat4& view, const Camera& camera) {
    // Calculate which chunk the camera is in (primarily for renderAllBlocks heuristic)
    glm::ivec2 camChunkPos = worldToChunkCoords(camera.getPosition());
    
    int renderedChunks = 0;
    
    for (auto const& [pos, chunk] : chunks) {
        // The active zone for loading is handled by World::update.
        // Here we just render all currently loaded chunks.
        // The `renderDistance` check here would be redundant if update correctly prunes chunks.
        // However, keeping a looser check or just rendering all in `this->chunks` is fine.

        // Mesh building is now handled by ensureInitialized during the update phase.
        // if (chunk->needsMeshRebuild()) {
        //     chunk->buildSurfaceMesh(this); 
        // }
            
        bool isCurrentChunk = (pos == camChunkPos);
            
        if (isCurrentChunk) {
            chunk->renderAllBlocks(projection, view); // Potentially for closer detail or debugging
        } else {
            chunk->renderSurface(projection, view);
        }
        renderedChunks++;
    }
    
    // Uncomment for debugging:
    // std::cout << "\rRendered " << renderedChunks << " of " << chunks.size() 
    //           << " chunks. Player at chunk: (" << camChunkPos.x << "," << camChunkPos.y << ")" << std::flush;
}

std::shared_ptr<Chunk> World::getChunkAt(int chunkX, int chunkZ) {
    glm::ivec2 pos(chunkX, chunkZ);
    auto it = chunks.find(pos);
    if (it != chunks.end()) {
        return it->second;
    }
    return nullptr;
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

void World::saveAllChunks() const {
    std::cout << "Saving all chunks..." << std::endl;
    for (auto const& [pos, chunk] : chunks) {
        if (chunk) {
            chunk->saveToFile(CHUNK_DATA_DIR);
        }
    }
    std::cout << "All chunks saved." << std::endl;
} 