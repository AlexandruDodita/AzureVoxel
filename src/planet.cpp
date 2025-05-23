#include "headers/planet.h"
#include "headers/world.h" // For World context if needed by chunks
#include "headers/block.h" // For Block class
#include <iostream> // For debugging output
#include <cmath>    // For std::ceil, std::floor, std::sqrt

// Helper to convert world position to chunk's 3D grid key relative to planet center
glm::ivec3 worldToPlanetChunkKey(const glm::vec3& worldPos, const glm::vec3& planetCenter, float chunkSize) {
    glm::vec3 relativePos = worldPos - planetCenter;
    return glm::ivec3(
        static_cast<int>(std::floor(relativePos.x / chunkSize)),
        static_cast<int>(std::floor(relativePos.y / chunkSize)),
        static_cast<int>(std::floor(relativePos.z / chunkSize))
    );
}

Planet::Planet(glm::vec3 position, float radius, int seed, const std::string& name)
    : position_(position), radius_(radius), seed_(seed), name_(name) {
    
    // Assuming CHUNK_SIZE_X, Y, Z are uniform for simplicity here.
    // If not, pick one or use an average. Let's assume CHUNK_SIZE_X.
    float chunkSize = static_cast<float>(CHUNK_SIZE_X); 
    chunksInRadius_ = static_cast<int>(std::ceil(radius_ / chunkSize));

    std::cout << "Planet '" << name_ << "' created at (" << position.x << "," << position.y << "," << position.z
              << ") with radius " << radius_ << " and seed " << seed_ 
              << ". Chunks in radius: " << chunksInRadius_ << std::endl;
    
    generatePlanetStructure();
}

Planet::~Planet() {
    chunks_.clear(); // shared_ptrs will handle individual Chunk deallocation
    std::cout << "Planet '" << name_ << "' destroyed." << std::endl;
}

void Planet::generatePlanetStructure() {
    std::cout << "Generating structure for planet: " << name_ << std::endl;
    float chunkSizeF = static_cast<float>(CHUNK_SIZE_X); // Assuming uniform chunk size

    // Iterate in a cube of chunks around the planet's center
    for (int x = -chunksInRadius_; x <= chunksInRadius_; ++x) {
        for (int y = -chunksInRadius_; y <= chunksInRadius_; ++y) {
            for (int z = -chunksInRadius_; z <= chunksInRadius_; ++z) {
                glm::vec3 chunkCenterOffset(
                    (static_cast<float>(x) + 0.5f) * chunkSizeF,
                    (static_cast<float>(y) + 0.5f) * chunkSizeF,
                    (static_cast<float>(z) + 0.5f) * chunkSizeF
                );
                // glm::vec3 chunkWorldCenter = position_ + chunkCenterOffset; // This line should be removed or remain commented

                // A simple check: is the center of this chunk within the planet's radius?
                // More sophisticated checks (e.g., AABB-sphere intersection) could be used.
                if (glm::length(chunkCenterOffset) <= radius_ + chunkSizeF * 0.866f) { // 0.866 is sqrt(3)/2, for corner distance
                    
                    glm::vec3 chunkMinCornerWorldPos = position_ + glm::vec3(x * chunkSizeF, y * chunkSizeF, z * chunkSizeF);
                    glm::ivec3 chunkKey(x, y, z);

                    // The Chunk constructor takes the world position of its minimum corner.
                    // We'll need to modify Chunk to accept planet context (center, radius) for spherical generation.
                    // For now, we create it with its world position.
                    auto newChunk = std::make_shared<Chunk>(chunkMinCornerWorldPos);
                    
                    newChunk->setPlanetContext(position_, radius_); // Pass planet context to chunk
                    
                    chunks_[chunkKey] = newChunk;
                }
            }
        }
    }
    std::cout << "Planet '" << name_ << "' structure generated with " << chunks_.size() << " initial chunks." << std::endl;
}

void Planet::update(const Camera& camera, const World* world_context) {
    // Basic implementation: iterate through chunks and queue initialization for uninitialized ones.
    // LoD and camera-based culling/prioritization would be added here later.
    
    if (!world_context) {
        std::cerr << "Planet::update - world_context is null for planet " << name_ << std::endl;
        return;
    }

    for (auto& pair : chunks_) {
        const auto& chunk_ptr = pair.second; // Use a different name to avoid conflict
        if (chunk_ptr && !chunk_ptr->isInitialized()) {
            // Check if chunk is already being processed (not implemented here, basic for now)
            
            // Need to capture variables for the lambda, including a shared_ptr to the chunk
            // to ensure it's alive when the task executes. Also, capture planet's seed, position, and radius.
            std::shared_ptr<Chunk> shared_chunk_ptr = chunk_ptr;
            int planet_seed = seed_;
            glm::vec3 planet_position = position_;
            float planet_radius = radius_;
            
            // It's crucial that world_context is const, so we cast away constness for addTaskToWorker if it's non-const.
            // However, it's better if World methods that modify internal state but are callable from a const World pointer
            // (like adding tasks) are designed to handle this, or World is passed as non-const.
            // For now, assuming World::addTaskToWorker and World::addMainThreadTask are available on const World*
            // or that world_context can be safely const_cast if those methods are not const.
            // The World::addTaskToWorker is not const. This will require a const_cast or a change in World.
            // Let's assume for now we might need a const_cast if World isn't changed.
            // A better solution is to make World non-const in Planet::update or ensure task adding methods are const.
            
            // The task for the worker thread.
            std::function<void()> generation_task = [shared_chunk_ptr, world_context, planet_seed, planet_position, planet_radius]() {
                // This code runs on a worker thread.
                // It should prepare data that doesn't require OpenGL.
                shared_chunk_ptr->ensureInitialized(world_context, planet_seed, planet_position, planet_radius);

                // After ensureInitialized (which calls generateTerrain and buildSurfaceMesh's data parts),
                // the OpenGL parts (VAO/VBO creation) need to run on the main thread.
                // Chunk::ensureInitialized is now designed to do both if called from main thread,
                // or just data prep if called from worker.
                // For a threaded approach, ensureInitialized might need to be split or openglInitialize called separately.

                // Let's refine this. Chunk::ensureInitialized now does data *and* tries to do GL if on main thread.
                // If Planet::update is on main thread, and calls World::addTaskToWorker, then ensureInitialized on worker,
                // then the GL parts of ensureInitialized will fail or be skipped.
                // The correct pattern is:
                // 1. Worker thread: chunk->generateTerrain(), chunk->buildSurfaceMesh_DataOnly() (hypothetical)
                // 2. Main thread: chunk->buildSurfaceMesh_GL() or chunk->openglInitialize()
                // Since ensureInitialized does both, let's assume for now it's okay IF
                // the main thread later calls openglInitialize if needed, or if ensureInitialized is smart.
                // The current Chunk::ensureInitialized calls generateTerrain and buildSurfaceMesh.
                // buildSurfaceMesh itself creates VAOs. This needs to be on the main thread.

                // Revised approach for Planet::update tasking:
                // Worker task: Generate terrain data.
                // Main thread task: Create Block objects from data and build mesh.
                
                // For simplicity, let's have the worker call ensureInitialized, which populates blockDataForInitialization_
                // and then have a main thread task that calls openglInitialize (which uses blockDataForInitialization_).

                if (shared_chunk_ptr->isInitialized()) { // Check if data part is done (mesh might not be GL ready)
                     // The openglInitialize method in Chunk is designed to be called on the main thread.
                     // It uses blockDataForInitialization_ and then builds the mesh with GL calls.
                    const_cast<World*>(world_context)->addMainThreadTask([shared_chunk_ptr, world_context]() {
                        // This code runs on the main thread.
                        if (glfwGetCurrentContext() == nullptr) {
                             std::cerr << "Planet Worker Callback: No GL context for main thread task for chunk!" << std::endl;
                             return;
                        }
                        if (shared_chunk_ptr && !shared_chunk_ptr->isInitialized() || shared_chunk_ptr->getSurfaceMeshVAO() == 0) {
                             shared_chunk_ptr->openglInitialize(const_cast<World*>(world_context)); // openglInitialize builds mesh and sets initialized
                        }
                    });
                }
            };
            // Add task to worker thread. const_cast is used because addTaskToWorker is not const.
            // This is generally unsafe if World state is modified by other threads without sync.
            // The task queue itself IS thread-safe.
            const_cast<World*>(world_context)->addTaskToWorker(generation_task);
        }
    }
}

void Planet::render(const glm::mat4& projection, const glm::mat4& view, bool wireframeState) const {
    for (const auto& pair : chunks_) {
        const auto& chunk = pair.second;
        // Only render if the chunk is initialized (i.e., mesh built)
        // The chunk's own renderSurface method already checks if VAO is valid.
        // We'll need to ensure chunks belonging to planets build spherical meshes.
        if (chunk && chunk->isInitialized()) { // Ensure chunk is not null and is initialized
            chunk->renderSurface(projection, view, wireframeState);
        }
    }
}

std::shared_ptr<Block> Planet::getBlockAtWorldPos(const glm::vec3& worldPos) const {
    // Calculate the position relative to the planet's center
    glm::vec3 relativePos = worldPos - position_;

    // Determine which chunk this world position falls into, relative to the planet's chunk grid
    // Assuming CHUNK_SIZE_X, Y, Z are uniform for this calculation.
    float chunkSize = static_cast<float>(CHUNK_SIZE_X); 
    glm::ivec3 chunkKey(
        static_cast<int>(std::floor(relativePos.x / chunkSize)),
        static_cast<int>(std::floor(relativePos.y / chunkSize)),
        static_cast<int>(std::floor(relativePos.z / chunkSize))
    );

    auto it = chunks_.find(chunkKey);
    if (it != chunks_.end()) {
        const auto& chunk = it->second;
        if (chunk) {
            // Convert world position to the chunk's local coordinate system.
            // Chunk's position is its min corner in world space.
            glm::vec3 chunkMinCornerWorldPos = chunk->getPosition();
            int localX = static_cast<int>(std::floor(worldPos.x - chunkMinCornerWorldPos.x));
            int localY = static_cast<int>(std::floor(worldPos.y - chunkMinCornerWorldPos.y));
            int localZ = static_cast<int>(std::floor(worldPos.z - chunkMinCornerWorldPos.z));

            // Validate local coordinates (0 to CHUNK_SIZE-1)
            if (localX >= 0 && localX < CHUNK_SIZE_X &&
                localY >= 0 && localY < CHUNK_SIZE_Y &&
                localZ >= 0 && localZ < CHUNK_SIZE_Z) {
                return chunk->getBlockAtLocal(localX, localY, localZ);
            }
        }
    }
    return nullptr; // Or a special "air" block
} 