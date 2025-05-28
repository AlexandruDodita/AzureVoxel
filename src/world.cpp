#include "../headers/world.h"
#include <iostream>
#include <cmath>
#include <algorithm> // For std::sort
#include <sys/stat.h> // For mkdir
#include <set> // For std::set in update method
#include <chrono> // For std::chrono
#include <filesystem> // For std::filesystem::create_directories

const std::string CHUNK_DATA_DIR = "chunk_data";

// ChunkThreadPool implementation
ChunkThreadPool::ChunkThreadPool(size_t numThreads) : stop_(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back(&ChunkThreadPool::workerFunction, this);
    }
    std::cout << "ChunkThreadPool initialized with " << numThreads << " threads." << std::endl;
}

ChunkThreadPool::~ChunkThreadPool() {
    shutdown();
}

void ChunkThreadPool::enqueueTask(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        if (stop_) return; // Don't accept new tasks if shutting down
        tasks_.push(std::move(task));
    }
    condition_.notify_one();
}

void ChunkThreadPool::shutdown() {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        stop_ = true;
    }
    condition_.notify_all();
    
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
}

void ChunkThreadPool::workerFunction() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
            
            if (stop_ && tasks_.empty()) {
                break;
            }
            
            if (tasks_.empty()) {
                continue; // Spurious wakeup
            }
            
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        
        try {
            if (task) {
                task();
            }
        } catch (const std::exception& e) {
            std::cerr << "ChunkThreadPool worker caught exception: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "ChunkThreadPool worker caught unknown exception." << std::endl;
        }
    }
}

// World implementation
World::World(const std::string& worldName, int defaultSeed)
    : worldName_(worldName), defaultSeed_(defaultSeed), 
      chunksGeneratedThisSecond_(0), meshesBuiltThisSecond_(0),
      lastPerformanceReport_(std::chrono::steady_clock::now()) {
    
    worldDataPath_ = "chunk_data/" + worldName_;
    createWorldDirectories();
    
    // Determine optimal thread counts based on hardware
    unsigned int hardwareThreads = std::thread::hardware_concurrency();
    if (hardwareThreads == 0) hardwareThreads = 4; // Fallback
    
    // Use more threads for chunk generation (CPU intensive)
    size_t chunkGenThreads = std::max(2u, hardwareThreads / 2);
    // Use fewer threads for mesh building (less CPU intensive, more frequent)
    size_t meshBuildThreads = std::max(1u, hardwareThreads / 4);
    
    chunkGenerationPool_ = std::make_unique<ChunkThreadPool>(chunkGenThreads);
    meshBuildingPool_ = std::make_unique<ChunkThreadPool>(meshBuildThreads);
    
    std::cout << "World '" << worldName_ << "' initialized with " 
              << chunkGenThreads << " chunk generation threads and " 
              << meshBuildThreads << " mesh building threads. Data path: " << worldDataPath_ << std::endl;
}

World::~World() {
    std::cout << "Destroying world '" << worldName_ << "'..." << std::endl;
    
    // Shutdown thread pools
    if (chunkGenerationPool_) {
        chunkGenerationPool_->shutdown();
    }
    if (meshBuildingPool_) {
        meshBuildingPool_->shutdown();
    }
    
    planets_.clear();
    std::cout << "World '" << worldName_ << "' destroyed." << std::endl;
}

void World::createWorldDirectories() {
    try {
        std::filesystem::create_directories(worldDataPath_);
        std::filesystem::create_directories("shaders"); // Ensure shaders dir exists at root for build if needed
        std::filesystem::create_directories("res/textures"); // Ensure res dir exists for textures
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error creating directories: " << e.what() << std::endl;
    }
}

void World::addPlanet(const glm::vec3& position, float radius, int seed, const std::string& name) {
    auto planet = std::make_shared<Planet>(position, radius, seed, name);
    planets_.push_back(planet);
    std::cout << "Added planet '" << name << "' to world." << std::endl;
}

void World::addChunkGenerationTask(const std::function<void()>& task) {
    if (chunkGenerationPool_) {
        chunkGenerationPool_->enqueueTask(task);
        chunksGeneratedThisSecond_++;
    }
}

void World::addMeshBuildingTask(const std::function<void()>& task) {
    if (meshBuildingPool_) {
        meshBuildingPool_->enqueueTask(task);
        meshesBuiltThisSecond_++;
    }
}

void World::addMainThreadTask(const std::function<void()>& task) {
    std::unique_lock<std::mutex> lock(mainThreadTasksMutex_);
    mainThreadTasks_.push(task);
}

void World::processMainThreadTasks() {
    std::queue<std::function<void()>> tasksToProcess;
    {
        std::unique_lock<std::mutex> lock(mainThreadTasksMutex_);
        if (!mainThreadTasks_.empty()) {
            tasksToProcess.swap(mainThreadTasks_);
        }
    }
    
    int tasksProcessed = 0;
    while (!tasksToProcess.empty()) {
        auto task = tasksToProcess.front();
        tasksToProcess.pop();
        if (task) {
            try {
                task();
                tasksProcessed++;
            } catch (const std::exception& e) {
                std::cerr << "Main thread task caught exception: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Main thread task caught unknown exception." << std::endl;
            }
        }
    }
    
    // Report performance metrics periodically
    reportPerformanceMetrics();
}

void World::reportPerformanceMetrics() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastPerformanceReport_);
    
    if (elapsed.count() >= 5) { // Report every 5 seconds
        int chunksGenerated = chunksGeneratedThisSecond_.exchange(0);
        int meshesBuilt = meshesBuiltThisSecond_.exchange(0);
        
        if (chunksGenerated > 0 || meshesBuilt > 0) {
            std::cout << "Performance: " << chunksGenerated << " chunks generated, " 
                      << meshesBuilt << " meshes built in last " << elapsed.count() << " seconds" << std::endl;
        }
        
        lastPerformanceReport_ = now;
    }
}

void World::update(const Camera& camera) {
    for (auto& planet : planets_) {
        if (planet) {
            planet->update(camera, this); // Pass world as context if planet needs to queue chunk tasks
        }
    }
    // Process any main thread tasks queued by worker threads (e.g., mesh creation for planet chunks)
    processMainThreadTasks();
}

void World::render(const glm::mat4& projection, const glm::mat4& view, const Camera& /*camera*/, bool wireframeState) {
    for (const auto& planet : planets_) {
        if (planet) {
            planet->render(projection, view, wireframeState);
        }
    }
    processMainThreadTasks();
}

std::shared_ptr<Block> World::getBlockAtWorldPos(const glm::vec3& worldPos) const {
    for (const auto& planet : planets_) {
        if (planet) {
            // Basic check: is worldPos within a generous bounding sphere of the planet?
            // This avoids calling getBlockAtWorldPos on distant planets unnecessarily.
            float distToPlanetCenter = glm::length(worldPos - planet->getPosition());
            // Add a margin, e.g., max chunk diagonal, to be safe.
            float planetEffectiveRadius = planet->getRadius() + CHUNK_SIZE_X * 1.732f; 
            if (distToPlanetCenter <= planetEffectiveRadius) {
                std::shared_ptr<Block> block = planet->getBlockAtWorldPos(worldPos);
                if (block) {
                    return block; // Found block in this planet
                }
            }
        }
    }
    return nullptr; // No block found in any planet at this position
}

/* Commenting out leftover flat-world save function
void World::saveAllChunks() const {
    std::cout << "Saving all chunks..." << std::endl;
    for (auto const& [pos, chunk] : chunks) { // 'chunks' member no longer exists
        if (chunk) {
            chunk->saveToFile(CHUNK_DATA_DIR); // CHUNK_DATA_DIR is also not a member, was a global
        }
    }
    std::cout << "All chunks saved." << std::endl;
}
*/ 