#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>
#include "chunk.h"
#include "camera.h"
#include "block.h"

// Hash function for glm::ivec2 to use in unordered_map
struct IVec2Hash {
    size_t operator()(const glm::ivec2& key) const {
        return std::hash<int>()(key.x) ^ (std::hash<int>()(key.y) << 1);
    }
};

// Required for std::set<glm::ivec2>
struct IVec2Compare {
    bool operator()(const glm::ivec2& a, const glm::ivec2& b) const {
        if (a.x != b.x) {
            return a.x < b.x;
        }
        return a.y < b.y;
    }
};

class World {
private:
    // Map of chunk positions to chunks
    std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>, IVec2Hash> chunks;
    
    // Maximum number of chunks to render in each direction (e.g., 3 means a 7x7 grid of chunks will be rendered)
    int renderDistance;
    
    // Size of each chunk in world units
    const int chunkSize = 16; // Must match CHUNK_SIZE_X/Z from chunk.h
    
    // Seed for world generation
    int worldSeed_;
    // Path for saving/loading chunk data
    std::string worldDataPath_;
    
    // Threading support for chunk generation
    std::thread workerThread_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    std::queue<std::function<void()>> taskQueue_;
    std::atomic<bool> shouldTerminate_;
    
    // Worker thread function to process chunk generation tasks
    void workerThreadFunction();
    
    // Add a task to the queue
    void addTask(const std::function<void()>& task);

    // Save all loaded chunks to files
    void saveAllChunks() const;

    // Main thread tasks
    std::queue<std::function<void()>> mainThreadTasks_;
    std::mutex mainThreadTasksMutex_;

public:
    // Constructor
    World(int renderDistance = 3);
    
    // Destructor
    ~World();
    
    // Initialize the world with a grid of chunks (REMOVED - world is loaded dynamically)
    // void init(int gridSize = 5); 
    
    // Update active chunks based on camera position (dynamic loading/unloading)
    void update(const Camera& camera);
    
    // Render visible chunks around the camera
    void render(const glm::mat4& projection, const glm::mat4& view, const Camera& camera);
    
    // Get chunk at the given CHUNK coordinates (if it exists)
    std::shared_ptr<Chunk> getChunkAt(int chunkX, int chunkZ);
    
    // Add a new chunk at the given CHUNK coordinates
    void addChunk(int chunkX, int chunkZ);
    
    // Convert world position to chunk coordinates
    glm::ivec2 worldToChunkCoords(const glm::vec3& worldPos) const;
    
    // Get block at specific WORLD coordinates (can cross chunk boundaries)
    std::shared_ptr<Block> getBlockAtWorldPos(int worldX, int worldY, int worldZ) const;
    std::shared_ptr<Block> getBlockAtWorldPos(const glm::vec3& worldPos) const {
        return getBlockAtWorldPos(static_cast<int>(std::floor(worldPos.x)), 
                                  static_cast<int>(std::floor(worldPos.y)), 
                                  static_cast<int>(std::floor(worldPos.z)));
    }

    // Add a task to the main thread queue
    void addMainThreadTask(const std::function<void()>& task);

    // Process tasks from the main thread queue
    void processMainThreadTasks();

    // Getter for worldDataPath_
    const std::string& getWorldDataPath() const { return worldDataPath_; }
}; 