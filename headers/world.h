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

    // Worker thread and task queue for chunk initialization (can be kept for planet chunks)
    void addMainThreadTask(const std::function<void()>& task);
    void processMainThreadTasks();

    // Threading: Add tasks to queues
    void addTaskToWorker(const std::function<void()>& task);

private:
    std::vector<std::shared_ptr<Planet>> planets_;
    std::string worldName_;
    std::string worldDataPath_;
    int defaultSeed_;

    // Worker thread components (can remain similar for initializing chunks within planets)
    std::thread workerThread_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    std::queue<std::function<void()>> taskQueue_;
    std::atomic<bool> shouldTerminate_;

    // Main thread task queue (for OpenGL calls, etc.)
    std::queue<std::function<void()>> mainThreadTasks_;
    std::mutex mainThreadTasksMutex_;

    void workerThreadFunction();
    
    // void saveAllChunks() const; // This would now be saveAllPlanets or similar
    void createWorldDirectories();

    // Remove members specific to flat chunk management, e.g.:
    // std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>, IVec2Hash> chunks;
    // int renderDistance; 
    // const int chunkSize; 
}; 