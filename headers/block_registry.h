#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include <optional>
#include <fstream>
#include <iostream>
#include <filesystem>

// Forward declarations
class World;
class Planet;

/**
 * Core block definition structure containing immutable block properties
 */
struct BlockDefinition {
    std::string id;               // "azurevoxel:stone"
    uint16_t numeric_id;          // Runtime numeric ID
    std::string display_name;     // "Stone"
    
    // Core properties
    bool solid = true;
    bool transparent = false;
    uint8_t light_emission = 0;
    float hardness = 1.0f;
    float blast_resistance = 1.0f;
    bool flammable = false;
    
    // Texture information
    std::string default_texture = "stone";
    std::unordered_map<std::string, std::string> per_face_textures; // Optional per-face textures
    
    // Variant definitions for different contexts
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> variants;
    
    BlockDefinition() = default;
    BlockDefinition(const std::string& id, uint16_t numeric_id, const std::string& display_name)
        : id(id), numeric_id(numeric_id), display_name(display_name) {}
};

/**
 * Context-specific block variant that can override base properties
 */
struct BlockVariant {
    uint16_t base_block_id;
    std::string context_name;     // "mars", "frozen", etc.
    
    // Overridden properties (only store what changes)
    std::optional<std::string> texture_override;
    std::optional<std::string> display_name_override;
    std::optional<float> hardness_override;
    std::optional<bool> solid_override;
    
    BlockVariant() = default;
    BlockVariant(uint16_t base_id, const std::string& context) 
        : base_block_id(base_id), context_name(context) {}
};

/**
 * Optimized runtime properties cache for hot-path operations
 */
struct BlockRenderData {
    uint16_t texture_atlas_index = 0;  // Direct GPU texture index
    uint8_t cull_mask = 0xFF;          // Bit flags for face culling
    uint8_t light_level = 0;           // 0-15 light emission
    uint8_t flags = 0;                 // transparency, solid, etc.
    
    // Bit flag constants
    static constexpr uint8_t FLAG_SOLID = 0x01;
    static constexpr uint8_t FLAG_TRANSPARENT = 0x02;
    static constexpr uint8_t FLAG_LIGHT_SOURCE = 0x04;
    
    bool isSolid() const { return flags & FLAG_SOLID; }
    bool isTransparent() const { return flags & FLAG_TRANSPARENT; }
    bool isLightSource() const { return flags & FLAG_LIGHT_SOURCE; }
    
    void setSolid(bool solid) { 
        if (solid) flags |= FLAG_SOLID; 
        else flags &= ~FLAG_SOLID; 
    }
    void setTransparent(bool transparent) { 
        if (transparent) flags |= FLAG_TRANSPARENT; 
        else flags &= ~FLAG_TRANSPARENT; 
    }
    void setLightSource(bool light_source) { 
        if (light_source) flags |= FLAG_LIGHT_SOURCE; 
        else flags &= ~FLAG_LIGHT_SOURCE; 
    }
};

/**
 * Biome context for environmental block selection
 */
struct BiomeContext {
    std::string biome_id;
    float temperature = 0.0f;        // -1.0 to 1.0 (cold to hot)
    float moisture = 0.0f;           // -1.0 to 1.0 (dry to wet)
    float atmospheric_pressure = 1.0f;
    std::string preferred_materials = "default";
    
    BiomeContext() = default;
    BiomeContext(const std::string& id, float temp, float moist) 
        : biome_id(id), temperature(temp), moisture(moist) {}
};

/**
 * Planet context for planetary block overrides
 */
struct PlanetContext {
    std::string planet_id;
    float gravity_modifier = 1.0f;
    std::string atmosphere_type = "earth";
    std::string geological_composition = "standard";
    std::unordered_map<std::string, std::string> material_overrides;
    
    PlanetContext() = default;
    PlanetContext(const std::string& id) : planet_id(id) {}
};

/**
 * Combined context key for efficient lookups
 */
struct ContextKey {
    uint16_t biome_id : 8;
    uint16_t planet_id : 8;
    
    ContextKey(uint8_t biome = 0, uint8_t planet = 0) {
        biome_id = biome;
        planet_id = planet;
    }
    
    bool operator==(const ContextKey& other) const {
        return biome_id == other.biome_id && planet_id == other.planet_id;
    }
};

// Hash function for ContextKey
namespace std {
    template<>
    struct hash<ContextKey> {
        size_t operator()(const ContextKey& k) const {
            return static_cast<size_t>(k.biome_id) << 8 | static_cast<size_t>(k.planet_id);
        }
    };
}

/**
 * Main Block Registry - Singleton managing all block definitions and lookups
 */
class BlockRegistry {
public:
    static constexpr uint16_t MAX_BLOCK_TYPES = 4096;
    static constexpr uint16_t MAX_CONTEXTS = 256;
    static constexpr uint16_t INVALID_BLOCK_ID = 0xFFFF;
    
    // Singleton access
    static BlockRegistry& getInstance() {
        static BlockRegistry instance;
        return instance;
    }
    
    // Initialization
    bool initialize(const std::string& blocks_directory = "res/blocks/");
    void shutdown();
    
    // Block definition management
    bool registerBlock(const BlockDefinition& definition);
    bool registerVariant(const BlockVariant& variant);
    
    // Context management
    uint8_t registerBiome(const BiomeContext& biome);
    uint8_t registerPlanet(const PlanetContext& planet);
    
    // Hot-path block lookups (O(1) performance)
    const BlockRenderData& getRenderData(uint16_t block_id) const;
    bool isBlockSolid(uint16_t block_id) const;
    bool isBlockTransparent(uint16_t block_id) const;
    uint8_t getBlockLightLevel(uint16_t block_id) const;
    
    // Context-aware block selection
    uint16_t selectBlock(const std::string& base_block_name, 
                        const BiomeContext& biome = BiomeContext{},
                        const PlanetContext& planet = PlanetContext{}) const;
    
    uint16_t selectBlock(uint16_t base_block_id,
                        uint8_t biome_id = 0,
                        uint8_t planet_id = 0) const;
    
    // Block information queries
    const BlockDefinition* getBlockDefinition(uint16_t block_id) const;
    const BlockDefinition* getBlockDefinition(const std::string& block_name) const;
    uint16_t getBlockId(const std::string& block_name) const;
    std::string getBlockName(uint16_t block_id) const;
    
    // Texture atlas management
    uint16_t getTextureIndex(const std::string& texture_name) const;
    bool loadTextureAtlas(const std::string& atlas_path);
    
    // Branch-free face culling
    inline bool shouldRenderFace(uint16_t block_id, uint16_t neighbor_id) const {
        return (render_data_[block_id].cull_mask & render_data_[neighbor_id].flags) != 0;
    }
    
    // Debug and development tools
    void printRegistryStats() const;
    bool reloadDefinitions(const std::string& blocks_directory);
    
private:
    BlockRegistry() = default;
    ~BlockRegistry() = default;
    BlockRegistry(const BlockRegistry&) = delete;
    BlockRegistry& operator=(const BlockRegistry&) = delete;
    
    // Core data storage
    std::vector<BlockDefinition> block_definitions_;
    std::unordered_map<std::string, uint16_t> name_to_id_;
    std::unordered_map<uint16_t, std::string> id_to_name_;
    
    // Context data
    std::vector<BiomeContext> biomes_;
    std::vector<PlanetContext> planets_;
    std::unordered_map<std::string, uint8_t> biome_name_to_id_;
    std::unordered_map<std::string, uint8_t> planet_name_to_id_;
    
    // Hot-path optimization arrays
    BlockRenderData render_data_[MAX_BLOCK_TYPES];
    uint16_t context_map_[MAX_BLOCK_TYPES][MAX_CONTEXTS];  // Pre-computed context lookups
    
    // Texture management
    std::unordered_map<std::string, uint16_t> texture_name_to_index_;
    
    // Private helpers
    bool loadBlockDefinitionFile(const std::string& file_path);
    bool loadBlockDefinitionFromJSON(std::ifstream& file);
    bool loadBlockDefinitionFromText(std::ifstream& file);
    std::string parseJSONString(const std::string& json, const std::string& key, const std::string& default_value = "");
    double parseJSONNumber(const std::string& json, const std::string& key, double default_value = 0.0);
    bool parseJSONBool(const std::string& json, const std::string& key, bool default_value = false);
    void buildOptimizationTables();
    void rebuildContextMap();
    uint16_t findOrCreateVariant(uint16_t base_block_id, uint8_t biome_id, uint8_t planet_id);
    
    // Default block creation
    void createDefaultBlocks();
    
    bool initialized_ = false;
    uint16_t next_block_id_ = 1;  // 0 reserved for air
    uint8_t next_biome_id_ = 1;   // 0 reserved for default
    uint8_t next_planet_id_ = 1;  // 0 reserved for default
}; 