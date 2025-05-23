#include "../headers/world.h"
#include <iostream>
#include <cmath>
#include <algorithm> // For std::sort
#include <sys/stat.h> // For mkdir
#include <set> // For std::set in update method
#include <chrono> // For std::chrono

const std::string CHUNK_DATA_DIR = "chunk_data";

World::World(int renderDistance) : renderDistance(renderDistance), shouldTerminate_(false) {
    // Create chunk_data directory if it doesn't exist
    mkdir(CHUNK_DATA_DIR.c_str(), 0755); // Unix-like
    // For Windows, you might need #include <direct.h> and _mkdir(CHUNK_DATA_DIR.c_str());

    worldSeed_ = 12345; // Example hardcoded seed
    worldDataPath_ = CHUNK_DATA_DIR;
    
    // Start worker thread for chunk generation/loading
    workerThread_ = std::thread(&World::workerThreadFunction, this);
}

World::~World() {
    // Signal worker thread to terminate and wait for it
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        shouldTerminate_ = true;
    }
    queueCondition_.notify_one();
    
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    
    saveAllChunks();
    // Smart pointers will handle chunk cleanup
}

void World::workerThreadFunction() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCondition_.wait(lock, [this] { 
                return !taskQueue_.empty() || shouldTerminate_; 
            });
            
            if (shouldTerminate_) {
                break;
            }
            
            task = std::move(taskQueue_.front());
            taskQueue_.pop();
        }
        
        // Execute the task outside the lock
        task();
    }
}

void World::addTask(const std::function<void()>& task) {
    std::unique_lock<std::mutex> lock(queueMutex_);
    taskQueue_.push(task);
    lock.unlock();
    queueCondition_.notify_one();
}

void World::addMainThreadTask(const std::function<void()>& task) {
    std::unique_lock<std::mutex> lock(mainThreadTasksMutex_);
    mainThreadTasks_.push(task);
}

void World::processMainThreadTasks() {
    std::unique_lock<std::mutex> lock(mainThreadTasksMutex_);
    while (!mainThreadTasks_.empty()) {
        std::function<void()> task = mainThreadTasks_.front();
        mainThreadTasks_.pop();
        lock.unlock(); // Unlock before executing the task
        task();        // Execute the task
        lock.lock();   // Re-lock to check the loop condition
    }
}

void World::update(const Camera& camera) {
    glm::ivec2 playerChunkPos = worldToChunkCoords(camera.getPosition());
    std::set<glm::ivec2, IVec2Compare> activeChunkCoords;
    
    // Debug the player's chunk position
    static glm::ivec2 lastPlayerChunkPos(-999, -999);
    if (playerChunkPos.x != lastPlayerChunkPos.x || playerChunkPos.y != lastPlayerChunkPos.y) {
        // std::cout << "Player moved to chunk: (" << playerChunkPos.x << ", " << playerChunkPos.y << ")" << std::endl; // Keep commented for cleaner output
        lastPlayerChunkPos = playerChunkPos;
    }
    
    // Determine new active zone based on render distance
    for (int x = playerChunkPos.x - renderDistance; x <= playerChunkPos.x + renderDistance; ++x) {
        for (int z = playerChunkPos.y - renderDistance; z <= playerChunkPos.y + renderDistance; ++z) {
            glm::ivec2 currentChunkKey(x, z);
            activeChunkCoords.insert(currentChunkKey);
        }
    }
    
    // Find chunks that need to be loaded
    std::vector<glm::ivec2> chunksToLoad;
    for (const auto& key : activeChunkCoords) {
        if (chunks.find(key) == chunks.end()) {
            chunksToLoad.push_back(key);
        }
    }
    
    // Sort chunks by distance to player for priority loading
    std::sort(chunksToLoad.begin(), chunksToLoad.end(), 
        [playerChunkPos](const glm::ivec2& a, const glm::ivec2& b) {
            float distA = glm::distance(glm::vec2(playerChunkPos), glm::vec2(a));
            float distB = glm::distance(glm::vec2(playerChunkPos), glm::vec2(b));
            return distA < distB;
        });
    
    // Process up to 5 chunks at once (balance between performance and loading speed)
    const int maxChunksToProcessPerFrame = 5;
    int chunksProcessed = 0;
    
    // Load new chunks that are in the active zone but not yet loaded
    for (const auto& key : chunksToLoad) {
        if (chunksProcessed >= maxChunksToProcessPerFrame) break;
        
        // If chunk doesn't exist at all, create it
        if (chunks.find(key) == chunks.end()) {
            // Create chunk at the appropriate world position
            glm::vec3 chunkPos(key.x * CHUNK_SIZE_X, 0, key.y * CHUNK_SIZE_Z);
            auto chunk = std::make_shared<Chunk>(chunkPos);
            chunks[key] = chunk;
            
            // Queue the initialization in the background
            // This will load from file or generate terrain as needed
            addTask([this, chunk, key]() {
                chunk->ensureInitialized(this, worldSeed_, worldDataPath_);
            });
            
            chunksProcessed++;
        }
    }
    
    // Remove chunks that are outside the active zone
    std::vector<glm::ivec2> chunksToRemove;
    for (const auto& [pos, chunk] : chunks) {
        if (activeChunkCoords.find(pos) == activeChunkCoords.end()) {
            chunksToRemove.push_back(pos);
        }
    }
    
    // Unload chunks outside active zone
    for (const auto& pos : chunksToRemove) {
        if (chunks.count(pos) > 0) {
            chunks[pos]->saveToFile(worldDataPath_);
            chunks.erase(pos);
            // std::cout << "Unloaded chunk at " << pos.x << "," << pos.y << std::endl; // Keep commented
        }
    }
    
    // Show initialization progress
    int initializedCount = 0;
    for (const auto& [pos, chunk] : chunks) {
        if (chunk->isInitialized()) {
            initializedCount++;
        }
    }
    
    static int lastReportedCount = -1;
    if (initializedCount != lastReportedCount) {
        // std::cout << "Chunks initialized: " << initializedCount << "/" << chunks.size() 
        //           << " (" << (chunks.size() > 0 ? (initializedCount * 100 / chunks.size()) : 0) << "%)" << std::endl; // Keep commented
        lastReportedCount = initializedCount;
    }
}

void World::render(const glm::mat4& projection, const glm::mat4& view, const Camera& camera, bool wireframeState) {
    glm::ivec2 camChunkPos = worldToChunkCoords(camera.getPosition());
    int renderedChunks = 0;
    int totalChunks = chunks.size();

    for (auto const& [pos, chunk] : chunks) {
        if (!chunk->isInitialized()) {
            continue;
        }
        // Pass wireframeState to chunk's render method
        chunk->renderSurface(projection, view, wireframeState);
        renderedChunks++;
    }

    // Output render statistics periodically
    static int lastFrameCount = 0;
    static auto lastOutputTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastOutputTime).count();
    
    // Update once per second
    if (elapsedTime > 1000) {
        // std::cout << "\nRendering " << renderedChunks << "/" << totalChunks << " chunks"
        //           << " from camera at chunk: (" << camChunkPos.x << ", " << camChunkPos.y << ")" << std::endl;
        lastOutputTime = currentTime;
        lastFrameCount = 0;
    }
}

std::shared_ptr<Chunk> World::getChunkAt(int chunkX, int chunkZ) {
    glm::ivec2 pos(chunkX, chunkZ);
    auto it = chunks.find(pos);
    if (it != chunks.end() && it->second->isInitialized()) {
        return it->second;
    }
    return nullptr;
}

void World::addChunk(int chunkX, int chunkZ) {
    glm::ivec2 pos(chunkX, chunkZ);
    if (chunks.find(pos) != chunks.end()) {
        return; // Chunk already exists
    }
    
    glm::vec3 chunkWorldPos(chunkX * chunkSize, 0.0f, chunkZ * chunkSize);
    auto newChunk = std::make_shared<Chunk>(chunkWorldPos);
    chunks[pos] = newChunk;
    
    // Queue initialization in worker thread
    addTask([this, world=this, newChunk]() {
        newChunk->ensureInitialized(world, worldSeed_, worldDataPath_);
    });
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