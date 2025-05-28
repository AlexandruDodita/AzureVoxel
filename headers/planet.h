#ifndef PLANET_H
#define PLANET_H

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional> // For std::hash
#include <glm/glm.hpp>
// #include <glm/gtx/hash.hpp> // No longer attempting to use this due to compiler issues

#include "chunk.h" // Relies on CHUNK_SIZE constants and Chunk class
#include "camera.h" // For update method

// Forward declaration for World, if Planet needs to interact with it (e.g. for global systems)
class World;

// Custom hash for glm::ivec3 for std::unordered_map
struct IVec3Hash {
    std::size_t operator()(const glm::ivec3& v) const {
        std::size_t h1 = std::hash<int>()(v.x);
        std::size_t h2 = std::hash<int>()(v.y);
        std::size_t h3 = std::hash<int>()(v.z);
        // A common way to combine hashes:
        // (boost::hash_combine uses something like seed ^= hash_value(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);)
        // For simplicity, a basic combination:
        return h1 ^ (h2 << 1) ^ (h3 << 2); 
    }
};

class Planet {
public:
    Planet(glm::vec3 position, float radius, int seed, const std::string& name = "DefaultPlanet");
    ~Planet();

    void update(const Camera& camera, const World* world_context); // For LoD, loading/unloading planet-chunks, passing world context for meshing
    void render(const glm::mat4& projection, const glm::mat4& view, bool wireframeState) const;

    std::shared_ptr<Block> getBlockAtWorldPos(const glm::vec3& worldPos) const;
    // void setBlockAtWorldPos(const glm::vec3& worldPos, BlockType type); // Future enhancement

    glm::vec3 getPosition() const { return position_; }
    float getRadius() const { return radius_; }
    const std::string& getName() const { return name_; }

private:
    glm::vec3 position_; // Center of the planet in world space
    float radius_;
    int seed_;
    std::string name_;

    // Chunks belonging to this planet.
    // Keyed by their grid position relative to the planet's center (in chunk units).
    std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>, IVec3Hash> chunks_;

    int chunksInRadius_; // Number of chunks from center to surface along an axis (approximate)
    int chunkRenderDistance_ = 14; // Max render distance in chunk units (radius) - increased for testing
    int maxChunksPerFrame_ = 1; // Maximum chunks to generate per frame to prevent lag
    std::vector<glm::ivec3> activeChunkKeys_; // Chunks within render distance of camera
    
    // Path for saving/loading planet-specific chunk data, if applicable in the future.
    // For now, planets and their chunks are procedurally generated in memory.
    // std::string dataPath_; 
};

#endif // PLANET_H 