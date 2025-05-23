#include "../headers/world.h"
#include <iostream>
#include <cmath>
#include <algorithm> // For std::sort
#include <sys/stat.h> // For mkdir
#include <set> // For std::set in update method
#include <chrono> // For std::chrono
#include <filesystem> // For std::filesystem::create_directories

const std::string CHUNK_DATA_DIR = "chunk_data";

World::World(const std::string& worldName, int defaultSeed)
    : worldName_(worldName), defaultSeed_(defaultSeed), shouldTerminate_(false) {
    worldDataPath_ = "chunk_data/" + worldName_;
    createWorldDirectories();
    
    workerThread_ = std::thread(&World::workerThreadFunction, this);
    std::cout << "World '" << worldName_ << "' initialized. Data path: " << worldDataPath_ << std::endl;
}

World::~World() {
    std::cout << "Destroying world '" << worldName_ << "'..." << std::endl;
    shouldTerminate_.store(true);
    queueCondition_.notify_one(); // Wake up worker thread if it's waiting
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    // saveAllPlanets(); // Placeholder for saving planet data if needed
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
    // Future: could queue initial chunks of this planet for generation by worker thread.
}

void World::workerThreadFunction() {
    std::cout << "Worker thread started." << std::endl;
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCondition_.wait(lock, [this] { return shouldTerminate_.load() || !taskQueue_.empty(); });
            if (shouldTerminate_.load() && taskQueue_.empty()) {
                break; 
            }
            if (taskQueue_.empty()) { // Spurious wakeup or terminate with empty queue
                continue;
            }
            task = std::move(taskQueue_.front());
            taskQueue_.pop();
        }
        try {
            if (task) {
                task(); // Execute the task (e.g., chunk data generation)
            }
        } catch (const std::exception& e) {
            std::cerr << "Worker thread caught exception: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Worker thread caught unknown exception." << std::endl;
        }
    }
    std::cout << "Worker thread stopping." << std::endl;
}

void World::addTaskToWorker(const std::function<void()>& task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        taskQueue_.push(task);
    }
    queueCondition_.notify_one();
}

std::queue<std::function<void()>> mainThreadTasks_;
std::mutex mainThreadTasksMutex_;

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
    while (!tasksToProcess.empty()) {
        auto task = tasksToProcess.front();
        tasksToProcess.pop();
        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "Main thread task caught exception: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Main thread task caught unknown exception." << std::endl;
            }
        }
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