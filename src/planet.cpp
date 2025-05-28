#include "headers/planet.h"
#include "headers/world.h" // For World context if needed by chunks
#include "headers/block.h" // For Block class
#include <iostream> // For debugging output
#include <cmath>    // For std::ceil, std::floor, std::sqrt
#include <algorithm> // For std::sort

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
    
    // Don't generate the entire planet structure upfront for large planets
    // Instead, chunks will be generated dynamically near the player
    std::cout << "Planet '" << name_ << "' will generate chunks dynamically near the player." << std::endl;
}

Planet::~Planet() {
    chunks_.clear(); // shared_ptrs will handle individual Chunk deallocation
    std::cout << "Planet '" << name_ << "' destroyed." << std::endl;
}

void Planet::update(const Camera& camera, const World* world_context) {
    activeChunkKeys_.clear();

    if (!world_context) {
        std::cerr << "Planet::update - world_context is null for planet " << name_ << std::endl;
        return;
    }

    glm::vec3 camPos = camera.getPosition();
    float chunkSizeF = static_cast<float>(CHUNK_SIZE_X); // Assuming uniform chunk size

    // Check if player is close enough to the planet to warrant chunk generation
    float distanceToPlanetCenter = glm::length(camPos - position_);
    float planetSurfaceDistance = distanceToPlanetCenter - radius_;
    
    // Only generate chunks if player is within a reasonable distance of the planet surface
    float maxGenerationDistance = chunkSizeF * chunkRenderDistance_ * 2.0f; // 2x render distance
    if (planetSurfaceDistance > maxGenerationDistance) {
        std::cout << "Player too far from planet " << name_ << " (distance: " << planetSurfaceDistance << "), skipping chunk generation" << std::endl;
        
        // Aggressively clean up all chunks when player is very far away
        if (planetSurfaceDistance > maxGenerationDistance * 3.0f && !chunks_.empty()) {
            std::cout << "Player very far from planet " << name_ << " - cleaning up all " << chunks_.size() << " chunks" << std::endl;
            chunks_.clear();
        }
        
        return;
    }

    glm::vec3 relativeCamPos = camPos - position_;
    glm::ivec3 cameraChunkKey(
        static_cast<int>(std::floor(relativeCamPos.x / chunkSizeF)),
        static_cast<int>(std::floor(relativeCamPos.y / chunkSizeF)),
        static_cast<int>(std::floor(relativeCamPos.z / chunkSizeF))
    );

    // Collect chunks that need to be active, sorted by distance
    std::vector<std::pair<float, glm::ivec3>> chunksToCheck;
    
    for (int x_offset = -chunkRenderDistance_; x_offset <= chunkRenderDistance_; ++x_offset) {
        for (int y_offset = -chunkRenderDistance_; y_offset <= chunkRenderDistance_; ++y_offset) {
            for (int z_offset = -chunkRenderDistance_; z_offset <= chunkRenderDistance_; ++z_offset) {
                glm::ivec3 currentChunkKey = cameraChunkKey + glm::ivec3(x_offset, y_offset, z_offset);
                
                // Calculate distance for prioritization
                float distance = glm::length(glm::vec3(x_offset, y_offset, z_offset));
                if (distance <= static_cast<float>(chunkRenderDistance_)) {
                    
                    // Check if this chunk should exist within the planet's bounds
                    glm::vec3 chunkCenterOffset(
                        (static_cast<float>(currentChunkKey.x) + 0.5f) * chunkSizeF,
                        (static_cast<float>(currentChunkKey.y) + 0.5f) * chunkSizeF,
                        (static_cast<float>(currentChunkKey.z) + 0.5f) * chunkSizeF
                    );
                    
                    // Check if chunk intersects with planet (with some margin for chunk corners)
                    if (glm::length(chunkCenterOffset) <= radius_ + chunkSizeF * 1.732f) {
                        chunksToCheck.push_back({distance, currentChunkKey});
                    }
                }
            }
        }
    }
    
    // Sort by distance (closest first)
    std::sort(chunksToCheck.begin(), chunksToCheck.end(), 
              [](const std::pair<float, glm::ivec3>& a, const std::pair<float, glm::ivec3>& b) {
                  return a.first < b.first; // Compare only the distance
              });
    
    int chunksProcessedThisFrame = 0;
    int maxChunksPerFrame = 3; // Increased for better performance with threading
    
    // Process chunks in order of distance with multi-threaded pipeline
    for (const auto& [distance, chunkKey] : chunksToCheck) {
        activeChunkKeys_.push_back(chunkKey);
        
        auto it = chunks_.find(chunkKey);
        if (it == chunks_.end()) {
            // Create new chunk
            if (chunksProcessedThisFrame >= maxChunksPerFrame) {
                break; // Limit chunks processed per frame
            }
            
            glm::vec3 chunkWorldPos = position_ + glm::vec3(
                static_cast<float>(chunkKey.x) * chunkSizeF,
                static_cast<float>(chunkKey.y) * chunkSizeF,
                static_cast<float>(chunkKey.z) * chunkSizeF
            );
            
            auto chunk_ptr = std::make_shared<Chunk>(chunkWorldPos);
            chunk_ptr->setPlanetContext(position_, radius_);
            chunks_[chunkKey] = chunk_ptr;
            
            // Start data generation phase in worker thread
            std::shared_ptr<Chunk> shared_chunk_ptr = chunk_ptr;
            int planet_seed = seed_;
            glm::vec3 planet_position = position_;
            float planet_radius = radius_;

            const_cast<World*>(world_context)->addChunkGenerationTask(
                [shared_chunk_ptr, world_context, planet_seed, planet_position, planet_radius]() {
                    shared_chunk_ptr->generateDataAsync(world_context, planet_seed, planet_position, planet_radius);
                }
            );
            
            chunksProcessedThisFrame++;
            std::cout << "🚀 Started data generation for new chunk at " << chunkKey.x << "," << chunkKey.y << "," << chunkKey.z << std::endl;
            
        } else {
            // Process existing chunk through the pipeline
            auto& chunk = it->second;
            if (!chunk) continue;
            
            ChunkState state = chunk->getState();
            
            switch (state) {
                case ChunkState::DATA_READY:
                    // Start mesh building phase
                    if (chunksProcessedThisFrame < maxChunksPerFrame) {
                        std::shared_ptr<Chunk> shared_chunk_ptr = chunk;
                        const_cast<World*>(world_context)->addMeshBuildingTask(
                            [shared_chunk_ptr, world_context]() {
                                shared_chunk_ptr->buildMeshAsync(world_context);
                            }
                        );
                        chunksProcessedThisFrame++;
                        std::cout << "🔧 Started mesh building for chunk at " << chunkKey.x << "," << chunkKey.y << "," << chunkKey.z << std::endl;
                    }
                    break;
                    
                case ChunkState::MESH_READY:
                    // Queue OpenGL initialization for main thread
                    {
                        std::shared_ptr<Chunk> shared_chunk_ptr = chunk;
                        const_cast<World*>(world_context)->addMainThreadTask(
                            [shared_chunk_ptr, world_context]() {
                                if (glfwGetCurrentContext() == nullptr) {
                                    std::cerr << "Planet: No GL context for main thread OpenGL initialization!" << std::endl;
                                    return;
                                }
                                shared_chunk_ptr->initializeOpenGL(const_cast<World*>(world_context));
                            }
                        );
                        std::cout << "🎨 Queued OpenGL initialization for chunk at " << chunkKey.x << "," << chunkKey.y << "," << chunkKey.z << std::endl;
                    }
                    break;
                    
                case ChunkState::FULLY_INITIALIZED:
                    // Chunk is ready for rendering - nothing to do
                    break;
                    
                default:
                    // Chunk is still being processed - nothing to do
                    break;
            }
        }
    }
    
    if (chunksProcessedThisFrame > 0) {
        std::cout << "🌍 Processed " << chunksProcessedThisFrame << " chunks this frame for planet " << name_ << std::endl;
    }
    
    // Clean up distant chunks to prevent memory buildup
    std::vector<glm::ivec3> chunksToRemove;
    float cleanupDistance = chunkSizeF * (chunkRenderDistance_ + 2); // Remove chunks beyond render distance + buffer
    
    for (auto it = chunks_.begin(); it != chunks_.end(); ) {
        const glm::ivec3& chunkKey = it->first;
        glm::vec3 chunkCenterOffset(
            (static_cast<float>(chunkKey.x) + 0.5f) * chunkSizeF,
            (static_cast<float>(chunkKey.y) + 0.5f) * chunkSizeF,
            (static_cast<float>(chunkKey.z) + 0.5f) * chunkSizeF
        );
        glm::vec3 chunkWorldCenter = position_ + chunkCenterOffset;
        float distanceToCamera = glm::length(chunkWorldCenter - camPos);
        
        if (distanceToCamera > cleanupDistance) {
            std::cout << "🗑️ Removing distant chunk at " << chunkKey.x << "," << chunkKey.y << "," << chunkKey.z 
                      << " (distance: " << distanceToCamera << ")" << std::endl;
            it = chunks_.erase(it);
        } else {
            ++it;
        }
    }
}

void Planet::render(const glm::mat4& projection, const glm::mat4& view, bool wireframeState) const {
    int chunksRendered = 0;
    int chunksSkipped = 0;
    float chunkSizeF = static_cast<float>(CHUNK_SIZE_X);
    
    // Extract camera position from view matrix for distance culling
    glm::mat4 invView = glm::inverse(view);
    glm::vec3 cameraPos = glm::vec3(invView[3]);
    
    // Check if player is close enough to the planet to warrant rendering
    float distanceToPlanetCenter = glm::length(cameraPos - position_);
    float planetSurfaceDistance = distanceToPlanetCenter - radius_;
    float maxRenderDistance = chunkSizeF * chunkRenderDistance_ * 2.0f;
    
    if (planetSurfaceDistance > maxRenderDistance) {
        // Player is too far away, don't render anything
        return;
    }
    
    for (const auto& key : activeChunkKeys_) {
        auto it = chunks_.find(key);
        if (it != chunks_.end()) {
            const auto& chunk = it->second;
            if (chunk && chunk->isReadyForRendering() && chunk->getSurfaceMeshVAO() != 0) {
                
                // Calculate chunk center in world space
                glm::vec3 chunkCenter = position_ + glm::vec3(
                    (static_cast<float>(key.x) + 0.5f) * chunkSizeF,
                    (static_cast<float>(key.y) + 0.5f) * chunkSizeF,
                    (static_cast<float>(key.z) + 0.5f) * chunkSizeF
                );
                
                // Distance culling - only render chunks within a reasonable distance
                float distanceToCamera = glm::length(chunkCenter - cameraPos);
                float maxChunkRenderDistance = chunkSizeF * (chunkRenderDistance_ + 1); // Slightly larger than generation distance
                
                if (distanceToCamera <= maxChunkRenderDistance) {
                    chunk->renderSurface(projection, view, wireframeState);
                    chunksRendered++;
                } else {
                    chunksSkipped++;
                }
            } else {
                chunksSkipped++;
            }
        }
    }
    
    // Only log rendering info occasionally to avoid spam
    static int renderLogCounter = 0;
    if ((chunksRendered > 0 || chunksSkipped > 0) && (renderLogCounter++ % 120 == 0)) { // Log every 120 frames (2 seconds at 60fps)
        std::cout << "🎮 Planet " << name_ << " rendered " << chunksRendered 
                  << " chunks, skipped " << chunksSkipped << " chunks" << std::endl;
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