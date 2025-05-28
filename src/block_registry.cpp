#include "../headers/block_registry.h"
#include "../headers/texture.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>

/**
 * Initialize the Block Registry system
 */
bool BlockRegistry::initialize(const std::string& blocks_directory) {
    if (initialized_) {
        std::cout << "BlockRegistry already initialized." << std::endl;
        return true;
    }
    
    std::cout << "Initializing BlockRegistry..." << std::endl;
    
    // Clear all data
    block_definitions_.clear();
    name_to_id_.clear();
    id_to_name_.clear();
    biomes_.clear();
    planets_.clear();
    biome_name_to_id_.clear();
    planet_name_to_id_.clear();
    texture_name_to_index_.clear();
    
    // Initialize arrays
    for (int i = 0; i < MAX_BLOCK_TYPES; ++i) {
        render_data_[i] = BlockRenderData{};
        for (int j = 0; j < MAX_CONTEXTS; ++j) {
            context_map_[i][j] = INVALID_BLOCK_ID;
        }
    }
    
    // Create default blocks first (hardcoded for backward compatibility)
    createDefaultBlocks();
    
    // Try to load block definitions from files if directory exists
    if (std::filesystem::exists(blocks_directory)) {
        std::cout << "Loading block definitions from: " << blocks_directory << std::endl;
        
        for (const auto& entry : std::filesystem::directory_iterator(blocks_directory)) {
            if (entry.path().extension() == ".json") {
                loadBlockDefinitionFile(entry.path().string());
            }
        }
    } else {
        std::cout << "Block definitions directory not found: " << blocks_directory << std::endl;
        std::cout << "Using default block definitions only." << std::endl;
    }
    
    // Register default biomes and planets
    BiomeContext temperate_biome("temperate", 0.2f, 0.5f);
    BiomeContext cold_biome("cold", -0.7f, 0.3f);
    BiomeContext hot_biome("hot", 0.8f, -0.3f);
    BiomeContext water_biome("water", 0.0f, 1.0f);
    
    // Add more diverse biomes for better terrain generation
    BiomeContext arctic_biome("arctic", -0.9f, 0.1f);
    BiomeContext desert_biome("desert", 0.9f, -0.8f);
    BiomeContext tropical_biome("tropical", 0.7f, 0.8f);
    BiomeContext mountain_biome("mountain", -0.3f, 0.2f);
    BiomeContext forest_biome("forest", 0.3f, 0.7f);
    BiomeContext swamp_biome("swamp", 0.4f, 0.9f);
    BiomeContext volcanic_biome("volcanic", 1.0f, -0.5f);
    BiomeContext tundra_biome("tundra", -0.6f, 0.4f);
    
    registerBiome(BiomeContext{}); // Default biome (index 0)
    registerBiome(temperate_biome);
    registerBiome(cold_biome);
    registerBiome(hot_biome);
    registerBiome(water_biome);
    registerBiome(arctic_biome);
    registerBiome(desert_biome);
    registerBiome(tropical_biome);
    registerBiome(mountain_biome);
    registerBiome(forest_biome);
    registerBiome(swamp_biome);
    registerBiome(volcanic_biome);
    registerBiome(tundra_biome);
    
    PlanetContext earth_planet("earth");
    PlanetContext mars_planet("mars");
    mars_planet.atmosphere_type = "thin";
    mars_planet.geological_composition = "iron_rich";
    
    registerPlanet(PlanetContext{}); // Default planet (index 0)
    registerPlanet(earth_planet);
    registerPlanet(mars_planet);
    
    // Build optimization tables
    buildOptimizationTables();
    
    initialized_ = true;
    std::cout << "BlockRegistry initialized with " << block_definitions_.size() << " blocks, " 
              << biomes_.size() << " biomes, " << planets_.size() << " planets." << std::endl;
    
    return true;
}

/**
 * Shutdown and cleanup resources
 */
void BlockRegistry::shutdown() {
    if (!initialized_) return;
    
    std::cout << "Shutting down BlockRegistry..." << std::endl;
    
    block_definitions_.clear();
    name_to_id_.clear();
    id_to_name_.clear();
    biomes_.clear();
    planets_.clear();
    biome_name_to_id_.clear();
    planet_name_to_id_.clear();
    texture_name_to_index_.clear();
    
    initialized_ = false;
}

/**
 * Create default blocks for backward compatibility
 */
void BlockRegistry::createDefaultBlocks() {
    // Air (0) - special case, always first
    BlockDefinition air("azurevoxel:air", 0, "Air");
    air.solid = false;
    air.transparent = true;
    air.default_texture = "air";
    registerBlock(air);
    
    // Stone (1)
    BlockDefinition stone("azurevoxel:stone", 1, "Stone");
    stone.hardness = 3.5f;
    stone.blast_resistance = 30.0f;
    stone.default_texture = "stone";
    registerBlock(stone);
    
    // Grass (2)
    BlockDefinition grass("azurevoxel:grass", 2, "Grass");
    grass.hardness = 1.0f;
    grass.default_texture = "grass";
    registerBlock(grass);
    
    // Dirt (3)
    BlockDefinition dirt("azurevoxel:dirt", 3, "Dirt");
    dirt.hardness = 1.2f;
    dirt.default_texture = "dirt";
    registerBlock(dirt);
    
    // Sand (4)
    BlockDefinition sand("azurevoxel:sand", 4, "Sand");
    sand.hardness = 1.0f;
    sand.default_texture = "sand";
    registerBlock(sand);
    
    // Water (5)
    BlockDefinition water("azurevoxel:water", 5, "Water");
    water.solid = false;
    water.transparent = true;
    water.default_texture = "water";
    registerBlock(water);
    
    // Snow (6)
    BlockDefinition snow("azurevoxel:snow", 6, "Snow");
    snow.hardness = 0.5f;
    snow.default_texture = "snow";
    registerBlock(snow);
    
    // Wood Log (7)
    BlockDefinition wood("azurevoxel:wood_log", 7, "Wood Log");
    wood.hardness = 2.0f;
    wood.flammable = true;
    wood.default_texture = "wood_log";
    registerBlock(wood);
    
    // Leaves (8)
    BlockDefinition leaves("azurevoxel:leaves", 8, "Leaves");
    leaves.solid = false;
    leaves.transparent = true;
    leaves.hardness = 0.3f;
    leaves.flammable = true;
    leaves.default_texture = "leaves";
    registerBlock(leaves);
    
    // Gravel (9)
    BlockDefinition gravel("azurevoxel:gravel", 9, "Gravel");
    gravel.hardness = 1.8f;
    gravel.default_texture = "gravel";
    registerBlock(gravel);
    
    // Gold Ore (10)
    BlockDefinition gold_ore("azurevoxel:gold_ore", 10, "Gold Ore");
    gold_ore.hardness = 4.0f;
    gold_ore.blast_resistance = 35.0f;
    gold_ore.default_texture = "gold_ore";
    registerBlock(gold_ore);
    
    // Add more block types for diverse biomes
    
    // Clay (11) - for swamp areas
    BlockDefinition clay("azurevoxel:clay", 11, "Clay");
    clay.hardness = 1.2f;
    clay.default_texture = "clay";
    registerBlock(clay);
    
    // Mud (12) - for swamp areas
    BlockDefinition mud("azurevoxel:mud", 12, "Mud");
    mud.hardness = 0.8f;
    mud.default_texture = "mud";
    registerBlock(mud);
    
    // Obsidian (13) - for volcanic areas
    BlockDefinition obsidian("azurevoxel:obsidian", 13, "Obsidian");
    obsidian.hardness = 5.0f;
    obsidian.blast_resistance = 50.0f;
    obsidian.default_texture = "obsidian";
    registerBlock(obsidian);
    
    // Lava (14) - for volcanic areas
    BlockDefinition lava("azurevoxel:lava", 14, "Lava");
    lava.solid = false;
    lava.transparent = true;
    lava.light_emission = 12;
    lava.hardness = 0.0f;
    lava.default_texture = "lava";
    registerBlock(lava);
    
    // Ice (15) - for arctic/cold areas
    BlockDefinition ice("azurevoxel:ice", 15, "Ice");
    ice.hardness = 1.5f;
    ice.transparent = true;
    ice.default_texture = "ice";
    registerBlock(ice);
    
    // Sandstone (16) - for desert areas
    BlockDefinition sandstone("azurevoxel:sandstone", 16, "Sandstone");
    sandstone.hardness = 2.5f;
    sandstone.blast_resistance = 20.0f;
    sandstone.default_texture = "sandstone";
    registerBlock(sandstone);
    
    // Cactus (17) - for desert areas
    BlockDefinition cactus("azurevoxel:cactus", 17, "Cactus");
    cactus.hardness = 1.0f;
    cactus.default_texture = "cactus";
    registerBlock(cactus);
    
    // Moss Stone (18) - for forest/tropical areas
    BlockDefinition moss_stone("azurevoxel:moss_stone", 18, "Moss Stone");
    moss_stone.hardness = 2.2f;
    moss_stone.default_texture = "moss_stone";
    registerBlock(moss_stone);
    
    // Granite (19) - for mountain areas
    BlockDefinition granite("azurevoxel:granite", 19, "Granite");
    granite.hardness = 3.5f;
    granite.blast_resistance = 30.0f;
    granite.default_texture = "granite";
    registerBlock(granite);
    
    // Basalt (20) - for volcanic/mountain areas
    BlockDefinition basalt("azurevoxel:basalt", 20, "Basalt");
    basalt.hardness = 3.0f;
    basalt.blast_resistance = 25.0f;
    basalt.default_texture = "basalt";
    registerBlock(basalt);
}

/**
 * Register a new block definition
 */
bool BlockRegistry::registerBlock(const BlockDefinition& definition) {
    if (definition.numeric_id >= MAX_BLOCK_TYPES) {
        std::cerr << "Error: Block ID " << definition.numeric_id << " exceeds maximum (" << MAX_BLOCK_TYPES << ")" << std::endl;
        return false;
    }
    
    // Check for ID conflicts
    if (definition.numeric_id < block_definitions_.size() && 
        !block_definitions_[definition.numeric_id].id.empty()) {
        std::cerr << "Error: Block ID " << definition.numeric_id << " already in use by " 
                  << block_definitions_[definition.numeric_id].id << std::endl;
        return false;
    }
    
    // Check for name conflicts
    if (name_to_id_.find(definition.id) != name_to_id_.end()) {
        std::cerr << "Error: Block name " << definition.id << " already registered" << std::endl;
        return false;
    }
    
    // Resize vectors if needed
    if (definition.numeric_id >= block_definitions_.size()) {
        block_definitions_.resize(definition.numeric_id + 1);
    }
    
    // Register the block
    block_definitions_[definition.numeric_id] = definition;
    name_to_id_[definition.id] = definition.numeric_id;
    id_to_name_[definition.numeric_id] = definition.id;
    
    // Set up render data
    BlockRenderData& render_data = render_data_[definition.numeric_id];
    render_data.setSolid(definition.solid);
    render_data.setTransparent(definition.transparent);
    render_data.light_level = definition.light_emission;
    render_data.texture_atlas_index = getTextureIndex(definition.default_texture);
    
    // Set up cull mask for face culling optimization
    if (definition.solid) {
        render_data.cull_mask = 0xFF; // Solid blocks cull against everything
    } else {
        render_data.cull_mask = BlockRenderData::FLAG_SOLID; // Non-solid blocks only cull against solid blocks
    }
    
    std::cout << "Registered block: " << definition.id << " (ID: " << definition.numeric_id << ")" << std::endl;
    
    // Update next_block_id_ for auto-incrementing
    if (definition.numeric_id >= next_block_id_) {
        next_block_id_ = definition.numeric_id + 1;
    }
    
    return true;
}

/**
 * Register a biome context
 */
uint8_t BlockRegistry::registerBiome(const BiomeContext& biome) {
    if (next_biome_id_ == 255) {
        std::cerr << "Error: Maximum number of biomes reached" << std::endl;
        return 0;
    }
    
    uint8_t biome_id = next_biome_id_++;
    
    if (biomes_.size() <= biome_id) {
        biomes_.resize(biome_id + 1);
    }
    
    biomes_[biome_id] = biome;
    if (!biome.biome_id.empty()) {
        biome_name_to_id_[biome.biome_id] = biome_id;
    }
    
    std::cout << "Registered biome: " << biome.biome_id << " (ID: " << (int)biome_id << ")" << std::endl;
    return biome_id;
}

/**
 * Register a planet context
 */
uint8_t BlockRegistry::registerPlanet(const PlanetContext& planet) {
    if (next_planet_id_ == 255) {
        std::cerr << "Error: Maximum number of planets reached" << std::endl;
        return 0;
    }
    
    uint8_t planet_id = next_planet_id_++;
    
    if (planets_.size() <= planet_id) {
        planets_.resize(planet_id + 1);
    }
    
    planets_[planet_id] = planet;
    if (!planet.planet_id.empty()) {
        planet_name_to_id_[planet.planet_id] = planet_id;
    }
    
    std::cout << "Registered planet: " << planet.planet_id << " (ID: " << (int)planet_id << ")" << std::endl;
    return planet_id;
}

/**
 * Get render data for hot-path operations
 */
const BlockRenderData& BlockRegistry::getRenderData(uint16_t block_id) const {
    if (block_id >= MAX_BLOCK_TYPES) {
        static BlockRenderData invalid_data;
        return invalid_data;
    }
    return render_data_[block_id];
}

/**
 * Check if a block is solid (optimized for hot path)
 */
bool BlockRegistry::isBlockSolid(uint16_t block_id) const {
    if (block_id >= MAX_BLOCK_TYPES) return false;
    return render_data_[block_id].isSolid();
}

/**
 * Check if a block is transparent (optimized for hot path)
 */
bool BlockRegistry::isBlockTransparent(uint16_t block_id) const {
    if (block_id >= MAX_BLOCK_TYPES) return false;
    return render_data_[block_id].isTransparent();
}

/**
 * Get block light level (optimized for hot path)
 */
uint8_t BlockRegistry::getBlockLightLevel(uint16_t block_id) const {
    if (block_id >= MAX_BLOCK_TYPES) return 0;
    return render_data_[block_id].light_level;
}

/**
 * Context-aware block selection
 */
uint16_t BlockRegistry::selectBlock(const std::string& base_block_name, 
                                   const BiomeContext& biome,
                                   const PlanetContext& planet) const {
    uint16_t base_id = getBlockId(base_block_name);
    if (base_id == INVALID_BLOCK_ID) return 0; // Default to air
    
    uint8_t biome_id = 0;
    uint8_t planet_id = 0;
    
    // Find biome ID
    auto biome_it = biome_name_to_id_.find(biome.biome_id);
    if (biome_it != biome_name_to_id_.end()) {
        biome_id = biome_it->second;
    }
    
    // Find planet ID
    auto planet_it = planet_name_to_id_.find(planet.planet_id);
    if (planet_it != planet_name_to_id_.end()) {
        planet_id = planet_it->second;
    }
    
    return selectBlock(base_id, biome_id, planet_id);
}

/**
 * Optimized block selection with numeric IDs
 */
uint16_t BlockRegistry::selectBlock(uint16_t base_block_id, uint8_t biome_id, uint8_t planet_id) const {
    if (base_block_id >= MAX_BLOCK_TYPES) return 0;
    
    ContextKey key(biome_id, planet_id);
    uint16_t context_index = static_cast<uint16_t>(key.biome_id) * 16 + static_cast<uint16_t>(key.planet_id);
    
    if (context_index < MAX_CONTEXTS) {
        uint16_t result = context_map_[base_block_id][context_index];
        if (result != INVALID_BLOCK_ID) {
            return result;
        }
    }
    
    // Fall back to base block if no context variant exists
    return base_block_id;
}

/**
 * Get block definition by ID
 */
const BlockDefinition* BlockRegistry::getBlockDefinition(uint16_t block_id) const {
    if (block_id >= block_definitions_.size()) return nullptr;
    if (block_definitions_[block_id].id.empty()) return nullptr;
    return &block_definitions_[block_id];
}

/**
 * Get block ID by name
 */
uint16_t BlockRegistry::getBlockId(const std::string& block_name) const {
    auto it = name_to_id_.find(block_name);
    if (it != name_to_id_.end()) {
        return it->second;
    }
    return INVALID_BLOCK_ID;
}

/**
 * Get block name by ID
 */
std::string BlockRegistry::getBlockName(uint16_t block_id) const {
    auto it = id_to_name_.find(block_id);
    if (it != id_to_name_.end()) {
        return it->second;
    }
    return "unknown";
}

/**
 * Get texture index for atlas mapping
 */
uint16_t BlockRegistry::getTextureIndex(const std::string& texture_name) const {
    auto it = texture_name_to_index_.find(texture_name);
    if (it != texture_name_to_index_.end()) {
        return it->second;
    }
    
    // Return a default index based on texture name hash for now
    // This will be replaced with proper texture atlas loading
    std::hash<std::string> hasher;
    return static_cast<uint16_t>(hasher(texture_name) % 256);
}

/**
 * Build optimization tables for fast lookups
 */
void BlockRegistry::buildOptimizationTables() {
    std::cout << "Building optimization tables..." << std::endl;
    
    // Initialize context map with base block IDs
    for (uint16_t block_id = 0; block_id < MAX_BLOCK_TYPES; ++block_id) {
        for (uint16_t context_id = 0; context_id < MAX_CONTEXTS; ++context_id) {
            context_map_[block_id][context_id] = block_id; // Default to base block
        }
    }
    
    // TODO: Build variants based on biome and planet contexts
    // For now, we'll create some simple variants for demonstration
    
    // Example: Cold biome variants
    uint8_t cold_biome_id = 2; // Assuming cold biome is registered as ID 2
    if (cold_biome_id < biomes_.size()) {
        // Grass becomes snow in cold biomes
        uint16_t grass_id = getBlockId("azurevoxel:grass");
        uint16_t snow_id = getBlockId("azurevoxel:snow");
        if (grass_id != INVALID_BLOCK_ID && snow_id != INVALID_BLOCK_ID) {
            uint16_t context_index = cold_biome_id * 16; // Planet ID 0
            context_map_[grass_id][context_index] = snow_id;
        }
    }
    
    std::cout << "Optimization tables built." << std::endl;
}

/**
 * Load block definition from file (supports both JSON and simple text format)
 */
bool BlockRegistry::loadBlockDefinitionFile(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open block definition file: " << file_path << std::endl;
        return false;
    }
    
    std::cout << "Loading block definitions from: " << file_path << std::endl;
    
    // Determine file type by extension
    std::filesystem::path path(file_path);
    std::string extension = path.extension().string();
    
    if (extension == ".json") {
        return loadBlockDefinitionFromJSON(file);
    } else {
        return loadBlockDefinitionFromText(file);
    }
}

/**
 * Load block definitions from JSON format
 */
bool BlockRegistry::loadBlockDefinitionFromJSON(std::ifstream& file) {
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    try {
        // Simple JSON parser for block definitions
        
        // Find "blocks" array
        size_t blocks_start = content.find("\"blocks\"");
        if (blocks_start == std::string::npos) {
            std::cerr << "Error: No 'blocks' array found in JSON file" << std::endl;
            return false;
        }
        
        // Find opening bracket of blocks array
        size_t array_start = content.find('[', blocks_start);
        if (array_start == std::string::npos) {
            std::cerr << "Error: Invalid JSON format - no opening bracket for blocks array" << std::endl;
            return false;
        }
        
        // Find closing bracket of blocks array
        size_t array_end = content.find_last_of(']');
        if (array_end == std::string::npos || array_end <= array_start) {
            std::cerr << "Error: Invalid JSON format - no closing bracket for blocks array" << std::endl;
            return false;
        }
        
        // Extract blocks array content
        std::string blocks_content = content.substr(array_start + 1, array_end - array_start - 1);
        
        // Parse each block object
        size_t current_pos = 0;
        while (current_pos < blocks_content.length()) {
            // Find next block object
            size_t obj_start = blocks_content.find('{', current_pos);
            if (obj_start == std::string::npos) break;
            
            size_t obj_end = blocks_content.find('}', obj_start);
            if (obj_end == std::string::npos) {
                std::cerr << "Error: Unclosed block object in JSON" << std::endl;
                break;
            }
            
            std::string block_obj = blocks_content.substr(obj_start + 1, obj_end - obj_start - 1);
            
            // Parse block object properties
            BlockDefinition def;
            
            // Parse required fields
            std::string id = parseJSONString(block_obj, "id");
            uint16_t numeric_id = static_cast<uint16_t>(parseJSONNumber(block_obj, "numeric_id"));
            std::string display_name = parseJSONString(block_obj, "display_name");
            
            if (id.empty() || numeric_id == 0 || display_name.empty()) {
                std::cerr << "Error: Missing required fields in block definition" << std::endl;
                current_pos = obj_end + 1;
                continue;
            }
            
            def.id = id;
            def.numeric_id = numeric_id;
            def.display_name = display_name;
            
            // Parse optional fields
            def.default_texture = parseJSONString(block_obj, "texture", "stone");
            def.solid = parseJSONBool(block_obj, "solid", true);
            def.transparent = parseJSONBool(block_obj, "transparent", false);
            def.hardness = static_cast<float>(parseJSONNumber(block_obj, "hardness", 1.0));
            def.blast_resistance = static_cast<float>(parseJSONNumber(block_obj, "blast_resistance", 1.0));
            def.flammable = parseJSONBool(block_obj, "flammable", false);
            def.light_emission = static_cast<uint8_t>(parseJSONNumber(block_obj, "light_emission", 0));
            
            // Register the block
            registerBlock(def);
            
            current_pos = obj_end + 1;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return false;
    }
}

/**
 * Load block definitions from simple text format
 */
bool BlockRegistry::loadBlockDefinitionFromText(std::ifstream& file) {
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue; // Skip empty lines and comments
        
        std::istringstream iss(line);
        std::string id, display_name, texture, solid_str;
        uint16_t numeric_id;
        
        if (iss >> id >> numeric_id >> display_name >> texture >> solid_str) {
            BlockDefinition def(id, numeric_id, display_name);
            def.default_texture = texture;
            def.solid = (solid_str == "true" || solid_str == "1");
            
            registerBlock(def);
        }
    }
    
    return true;
}

/**
 * Helper function to parse JSON string values
 */
std::string BlockRegistry::parseJSONString(const std::string& json, const std::string& key, const std::string& default_value) {
    std::string search_key = "\"" + key + "\"";
    size_t key_pos = json.find(search_key);
    if (key_pos == std::string::npos) return default_value;
    
    size_t colon_pos = json.find(':', key_pos);
    if (colon_pos == std::string::npos) return default_value;
    
    size_t quote_start = json.find('"', colon_pos);
    if (quote_start == std::string::npos) return default_value;
    
    size_t quote_end = json.find('"', quote_start + 1);
    if (quote_end == std::string::npos) return default_value;
    
    return json.substr(quote_start + 1, quote_end - quote_start - 1);
}

/**
 * Helper function to parse JSON number values
 */
double BlockRegistry::parseJSONNumber(const std::string& json, const std::string& key, double default_value) {
    std::string search_key = "\"" + key + "\"";
    size_t key_pos = json.find(search_key);
    if (key_pos == std::string::npos) return default_value;
    
    size_t colon_pos = json.find(':', key_pos);
    if (colon_pos == std::string::npos) return default_value;
    
    // Skip whitespace after colon
    size_t num_start = colon_pos + 1;
    while (num_start < json.length() && std::isspace(json[num_start])) {
        num_start++;
    }
    
    if (num_start >= json.length()) return default_value;
    
    // Find end of number (comma, brace, or whitespace)
    size_t num_end = num_start;
    while (num_end < json.length() && 
           json[num_end] != ',' && 
           json[num_end] != '}' && 
           !std::isspace(json[num_end])) {
        num_end++;
    }
    
    if (num_end <= num_start) return default_value;
    
    std::string num_str = json.substr(num_start, num_end - num_start);
    try {
        return std::stod(num_str);
    } catch (const std::exception&) {
        return default_value;
    }
}

/**
 * Helper function to parse JSON boolean values
 */
bool BlockRegistry::parseJSONBool(const std::string& json, const std::string& key, bool default_value) {
    std::string search_key = "\"" + key + "\"";
    size_t key_pos = json.find(search_key);
    if (key_pos == std::string::npos) return default_value;
    
    size_t colon_pos = json.find(':', key_pos);
    if (colon_pos == std::string::npos) return default_value;
    
    // Find "true" or "false"
    size_t true_pos = json.find("true", colon_pos);
    size_t false_pos = json.find("false", colon_pos);
    
    // Check which comes first after the colon
    if (true_pos != std::string::npos && (false_pos == std::string::npos || true_pos < false_pos)) {
        // Make sure "true" is the next non-whitespace content
        size_t check_pos = colon_pos + 1;
        while (check_pos < true_pos && std::isspace(json[check_pos])) {
            check_pos++;
        }
        if (check_pos == true_pos) {
            return true;
        }
    }
    
    if (false_pos != std::string::npos) {
        // Make sure "false" is the next non-whitespace content
        size_t check_pos = colon_pos + 1;
        while (check_pos < false_pos && std::isspace(json[check_pos])) {
            check_pos++;
        }
        if (check_pos == false_pos) {
            return false;
        }
    }
    
    return default_value;
}

/**
 * Print registry statistics
 */
void BlockRegistry::printRegistryStats() const {
    std::cout << "\n=== Block Registry Statistics ===" << std::endl;
    std::cout << "Blocks registered: " << block_definitions_.size() << std::endl;
    std::cout << "Biomes registered: " << biomes_.size() << std::endl;
    std::cout << "Planets registered: " << planets_.size() << std::endl;
    std::cout << "Next block ID: " << next_block_id_ << std::endl;
    
    std::cout << "\nRegistered blocks:" << std::endl;
    for (size_t i = 0; i < block_definitions_.size(); ++i) {
        if (!block_definitions_[i].id.empty()) {
            const auto& def = block_definitions_[i];
            std::cout << "  " << i << ": " << def.id << " (" << def.display_name << ")" 
                      << " solid=" << def.solid << " texture=" << def.default_texture << std::endl;
        }
    }
    
    std::cout << "\nRegistered biomes:" << std::endl;
    for (size_t i = 0; i < biomes_.size(); ++i) {
        const auto& biome = biomes_[i];
        std::cout << "  " << i << ": " << biome.biome_id 
                  << " temp=" << biome.temperature << " moisture=" << biome.moisture << std::endl;
    }
    
    std::cout << "\nRegistered planets:" << std::endl;
    for (size_t i = 0; i < planets_.size(); ++i) {
        const auto& planet = planets_[i];
        std::cout << "  " << i << ": " << planet.planet_id 
                  << " atmosphere=" << planet.atmosphere_type << std::endl;
    }
    std::cout << "================================\n" << std::endl;
} 