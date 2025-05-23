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
    activeChunkKeys_.clear();

    if (!world_context) {
        std::cerr << "Planet::update - world_context is null for planet " << name_ << std::endl;
        return;
    }

    glm::vec3 camPos = camera.getPosition();
    float chunkSizeF = static_cast<float>(CHUNK_SIZE_X); // Assuming uniform chunk size

    glm::vec3 relativeCamPos = camPos - position_;
    glm::ivec3 cameraChunkKey(
        static_cast<int>(std::floor(relativeCamPos.x / chunkSizeF)),
        static_cast<int>(std::floor(relativeCamPos.y / chunkSizeF)),
        static_cast<int>(std::floor(relativeCamPos.z / chunkSizeF))
    );

    for (int x_offset = -chunkRenderDistance_; x_offset <= chunkRenderDistance_; ++x_offset) {
        for (int y_offset = -chunkRenderDistance_; y_offset <= chunkRenderDistance_; ++y_offset) {
            for (int z_offset = -chunkRenderDistance_; z_offset <= chunkRenderDistance_; ++z_offset) {
                glm::ivec3 currentChunkKey = cameraChunkKey + glm::ivec3(x_offset, y_offset, z_offset);

                // Spherical distance check (optional, cube check might be enough with later frustum culling)
                if (glm::length(glm::vec3(currentChunkKey - cameraChunkKey)) > static_cast<float>(chunkRenderDistance_)) {
                    continue; // Outside spherical render distance
                }

                auto it = chunks_.find(currentChunkKey);
                if (it != chunks_.end()) {
                    std::shared_ptr<Chunk> chunk_ptr = it->second;
                    activeChunkKeys_.push_back(currentChunkKey);

                    if (chunk_ptr && !chunk_ptr->isInitialized()) {
                        std::shared_ptr<Chunk> shared_chunk_ptr = chunk_ptr;
                        int planet_seed = seed_;
                        glm::vec3 planet_position = position_;
                        float planet_radius = radius_;

                        std::function<void()> generation_task = 
                            [shared_chunk_ptr, world_context, planet_seed, planet_position, planet_radius]() {
                            shared_chunk_ptr->ensureInitialized(world_context, planet_seed, planet_position, planet_radius);
                            if (shared_chunk_ptr->isInitialized()) { 
                                const_cast<World*>(world_context)->addMainThreadTask(
                                    [shared_chunk_ptr, world_context]() {
                                    if (glfwGetCurrentContext() == nullptr) {
                                        std::cerr << "Planet Worker Callback: No GL context for main thread task for chunk!" << std::endl;
                                        return;
                                    }
                                    if (shared_chunk_ptr && (!shared_chunk_ptr->isInitialized() || shared_chunk_ptr->getSurfaceMeshVAO() == 0)) {
                                        shared_chunk_ptr->openglInitialize(const_cast<World*>(world_context));
                                    }
                                });
                            }
                        };
                        const_cast<World*>(world_context)->addTaskToWorker(generation_task);
                    }
                }
            }
        }
    }
}

void Planet::render(const glm::mat4& projection, const glm::mat4& view, bool wireframeState) const {
    for (const auto& key : activeChunkKeys_) {
        auto it = chunks_.find(key);
        if (it != chunks_.end()) {
            const auto& chunk = it->second;
            if (chunk && chunk->isInitialized()) {
                chunk->renderSurface(projection, view, wireframeState);
            }
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