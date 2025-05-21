#include "../headers/world.h"
#include <iostream>
#include <cmath>
#include <algorithm> // For std::sort
#include <sys/stat.h> // For mkdir
#include <set> // For std::set in update method

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
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        taskQueue_.push(task);
    }
    queueCondition_.notify_one();
}

void World::update(const Camera& camera) {
    glm::ivec2 playerChunkPos = worldToChunkCoords(camera.getPosition());
    std::set<glm::ivec2, IVec2Compare> activeChunkCoords;
    
    // Determine new active zone
    for (int x = playerChunkPos.x - renderDistance; x <= playerChunkPos.x + renderDistance; ++x) {
        for (int z = playerChunkPos.y - renderDistance; z <= playerChunkPos.y + renderDistance; ++z) {
            glm::ivec2 currentChunkKey(x, z);
            activeChunkCoords.insert(currentChunkKey);
        }
    }
    
    // First, prioritize chunks closest to the player
    std::vector<glm::ivec2> chunkKeys;
    for (const auto& key : activeChunkCoords) {
        if (chunks.find(key) == chunks.end()) {
            chunkKeys.push_back(key);
        }
    }
    
    // Sort chunks by distance to player for prioritized loading
    std::sort(chunkKeys.begin(), chunkKeys.end(), 
        [playerChunkPos](const glm::ivec2& a, const glm::ivec2& b) {
            float distA = glm::distance(glm::vec2(playerChunkPos), glm::vec2(a));
            float distB = glm::distance(glm::vec2(playerChunkPos), glm::vec2(b));
            return distA < distB;
        });
    
    // Process the sorted chunks
    for (const auto& key : chunkKeys) {
        // First check if it was already added by another thread
        if (chunks.find(key) != chunks.end()) {
            continue;
        }
        
        glm::vec3 chunkWorldPos(key.x * chunkSize, 0.0f, key.y * chunkSize);
        auto newChunk = std::make_shared<Chunk>(chunkWorldPos);
        chunks[key] = newChunk; // Add placeholder to map immediately
        
        // Queue initialization for background processing
        addTask([this, world=this, key, newChunk]() {
            newChunk->ensureInitialized(world, worldSeed_, worldDataPath_);
        });
    }

    // Unload chunks outside the active zone
    for (auto it = chunks.begin(); it != chunks.end(); /* no increment */) {
        if (activeChunkCoords.find(it->first) == activeChunkCoords.end()) {
            // Chunk is outside active zone, save and unload
            it->second->saveToFile(worldDataPath_); 
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
        // Skip chunks that aren't initialized yet (still being processed by worker thread)
        if (!chunk->isInitialized()) {
            continue;
        }
            
        bool isCurrentChunk = (pos == camChunkPos);
            
        if (isCurrentChunk) {
            chunk->renderAllBlocks(projection, view); // Potentially for closer detail or debugging
        } else {
            chunk->renderSurface(projection, view);
        }
        renderedChunks++;
    }
    
    // Optional debug info
    static int lastInfoTime = 0;
    int currentTime = static_cast<int>(glfwGetTime());
    if (currentTime > lastInfoTime) {
        lastInfoTime = currentTime;
        // Uncomment for periodic debugging info if needed
        // std::cout << "Rendered " << renderedChunks << " of " << chunks.size() 
        //           << " chunks. Player at: (" << camChunkPos.x << "," << camChunkPos.y << ")" << std::endl;
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