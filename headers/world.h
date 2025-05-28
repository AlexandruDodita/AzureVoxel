#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
// #include <glm/gtx/hash.hpp> // Removed as we are using a custom hash for ivec3
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>
#include "chunk.h"
#include "camera.h"
#include "block.h"
#include "planet.h"
#include <string>
#include <chrono>

// Enhanced thread pool for chunk operations
class ChunkThreadPool {
public:
    ChunkThreadPool(size_t numThreads);
    ~ChunkThreadPool();
    
    void enqueueTask(std::function<void()> task);
    void shutdown();
    
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queueMutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
    
    void workerFunction();
};

class World {
public:
    // Constructor might change to not take renderDistance for flat chunks, or adapt it for planets
    World(const std::string& worldName = "MyWorld", int defaultSeed = 12345);
    ~World();

    void addPlanet(const glm::vec3& position, float radius, int seed, const std::string& name);
    void update(const Camera& camera);
    void render(const glm::mat4& projection, const glm::mat4& view, const Camera& camera, bool wireframeState);

    // getBlockAtWorldPos will now iterate through planets
    std::shared_ptr<Block> getBlockAtWorldPos(const glm::vec3& worldPos) const;

    // World information
    const std::string& getWorldName() const { return worldName_; }
    const std::string& getWorldDataPath() const { return worldDataPath_; }

    // Potentially remove or adapt: getChunkAt, worldToChunkCoords if they are specific to flat world structure
    // For planet-specific chunk access, one might go through Planet::getBlockAtWorldPos or a similar Planet method.
    // std::shared_ptr<Chunk> getChunkAt(int chunkX, int chunkZ); // Might be deprecated or changed
    // glm::ivec2 worldToChunkCoords(const glm::vec3& worldPos) const; // Might be deprecated or changed

    // Enhanced threading: Add tasks to different thread pools
    void addMainThreadTask(const std::function<void()>& task);
    void processMainThreadTasks();
    
    // Chunk generation tasks (CPU intensive)
    void addChunkGenerationTask(const std::function<void()>& task);
    
    // Mesh building tasks (less CPU intensive, more frequent)
    void addMeshBuildingTask(const std::function<void()>& task);
    
    // Legacy support for existing code
    void addTaskToWorker(const std::function<void()>& task) { addChunkGenerationTask(task); }

private:
    std::vector<std::shared_ptr<Planet>> planets_;
    std::string worldName_;
    std::string worldDataPath_;
    int defaultSeed_;

    // Enhanced multi-threading system
    std::unique_ptr<ChunkThreadPool> chunkGenerationPool_;  // For terrain generation and file I/O
    std::unique_ptr<ChunkThreadPool> meshBuildingPool_;     // For mesh building and preparation
    
    // Main thread task queue (for OpenGL calls, etc.)
    std::queue<std::function<void()>> mainThreadTasks_;
    std::mutex mainThreadTasksMutex_;
    
    // Performance monitoring
    std::atomic<int> chunksGeneratedThisSecond_;
    std::atomic<int> meshesBuiltThisSecond_;
    std::chrono::steady_clock::time_point lastPerformanceReport_;
    
    void createWorldDirectories();
    void reportPerformanceMetrics();
}; 