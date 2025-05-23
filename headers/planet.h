#ifndef PLANET_H
#define PLANET_H

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp> // For glm::ivec3 hashing

#include "chunk.h" // Relies on CHUNK_SIZE constants and Chunk class
#include "camera.h" // For update method

// Forward declaration for World, if Planet needs to interact with it (e.g. for global systems)
class World;

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
    std::string getName() const { return name_; }

private:
    void generatePlanetStructure(); // Internal method to create and initialize chunks

    glm::vec3 position_; // Center of the planet in world space
    float radius_;
    int seed_;
    std::string name_;

    // Chunks belonging to this planet.
    // Keyed by their grid position relative to the planet's center (in chunk units).
    std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>> chunks_;

    int chunksInRadius_; // Number of chunks from center to surface along an axis (approximate)
    
    // Path for saving/loading planet-specific chunk data, if applicable in the future.
    // For now, planets and their chunks are procedurally generated in memory.
    // std::string dataPath_; 
};

#endif // PLANET_H 