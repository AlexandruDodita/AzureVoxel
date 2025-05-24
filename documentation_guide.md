# AzureVoxel Project Documentation

AzureVoxel is a voxel-based 3D environment/game engine featuring a sophisticated Block Registry system for modular block management. The engine supports both flat terrain and spherical planet generation with biome-aware block selection and context-sensitive material placement. The core architecture revolves around a data-driven approach where block definitions are managed through a centralized registry system that supports environmental adaptation and performance optimization.

The Block Registry system implements a component-based architecture with BlockDefinition, BlockVariant, and BlockRenderData structures, enabling context-aware block selection based on biome and planet conditions. This allows for dynamic material adaptation where the same logical block type can have different properties and appearances depending on environmental factors.

Performance is optimized through O(1) lookup tables, pre-computed context mappings, and branch-free face culling operations, making the system suitable for large-scale voxel worlds with thousands of active chunks.

## File Structure

```
.
├── CMakeLists.txt
├── README.md
├── build/
├── chunk_data/ 
│   └── [world_name]/ // Directory for storing saved chunk files for a specific world
├── cmake_install.cmake
├── create_texture.py
├── documentation_guide.md
├── external/
│   └── stb_image.h
├── headers/
│   ├── block.h
│   ├── block_registry.h    // New: Block Registry system header
│   ├── camera.h
│   ├── chunk.h
│   ├── crosshair.h
│   ├── planet.h       // Planet class header
│   ├── shader.h
│   ├── texture.h
│   ├── window.h
│   └── world.h
├── main.cpp
├── res/
│   ├── blocks/
│   │   ├── example_blocks.txt  // Simple text format block definitions
│   │   └── blocks.json         // JSON format block definitions with rich metadata
│   └── textures/
│       ├── grass_block.png
│       └── spritesheet.png 
└── shaders/
    ├── crosshair_fragment.glsl 
    ├── crosshair_vertex.glsl   
    ├── fragment.glsl           
    └── vertex.glsl             
└── src/
    ├── block.cpp
    ├── block_registry.cpp  // New: Block Registry implementation
    ├── camera.cpp
    ├── chunk.cpp
    ├── crosshair.cpp
    ├── planet.cpp       // Planet class implementation
    ├── shader.cpp
    ├── texture.cpp
    ├── window.cpp
    └── world.cpp
```

---

## Game Architecture and Design

### Conceptual Overview

AzureVoxel is a voxel-based 3D environment/game engine built around a modular Block Registry architecture. The core system revolves around a `World` which can contain multiple `Planet` objects. Each `Planet` is composed of `Chunk`s, which in turn are made up of individual `Block`s managed through the centralized `BlockRegistry`. Rendering is managed through OpenGL, with a `Window` class handling GLFW interactions, a `Camera` class for viewpoint control, and `Shader` and `Texture` classes for visual representation. A `Crosshair` class provides a 2D UI element.

The system is designed with performance in mind, particularly for rendering large voxel worlds and planets. Key aspects include:
*   **Block Registry System:** Centralized management of all block definitions, variants, and properties with O(1) lookup performance
*   **Context-Aware Block Selection:** Biome and planet-aware block placement using environmental contexts
*   **Planet-based world management:** The `World` manages planets. Planets manage their own chunks.
*   **Spherical Chunk Generation & Meshing:** `Chunk`s can now generate terrain and meshes that conform to a spherical planet surface.
*   **Optimized chunk meshing:** Each chunk (whether flat or part of a sphere) builds a single optimized mesh (`surfaceMesh`) containing only the visible faces of its blocks.
*   **Texture Atlasing:** A global spritesheet (`Block::spritesheetTexture`) is used for block textures.
*   **Multithreaded chunk loading/generation:** The `World` uses a worker thread to handle chunk data loading (if implemented for planets) or procedural generation, including tasks queued by `Planet` objects for their chunks.

### Block Registry Architecture

The Block Registry system implements a sophisticated component-based architecture:

```ascii
+---------------------+       +---------------------+       +---------------------+
|   BlockDefinition   |------>|    BlockVariant     |------>|   BlockRenderData   |
| -id, numeric_id     |       | -context overrides  |       | -texture_atlas_index|
| -display_name       |       | -biome/planet refs   |       | -cull_mask, flags   |
| -core properties    |       +---------------------+       | -optimized lookups  |
| -texture info       |                                     +---------------------+
+---------------------+                                              ^
           |                                                         |
           v                                                         |
+---------------------+       +---------------------+               |
|   BiomeContext      |<----->|   PlanetContext     |               |
| -temperature        |       | -gravity_modifier   |               |
| -moisture           |       | -atmosphere_type    |               |
| -material prefs     |       | -geological_comp    |               |
+---------------------+       +---------------------+               |
           |                           |                             |
           +---------------------------+-----------------------------+
                                       |
                                       v
                              +---------------------+
                              |   BlockRegistry     |
                              | (Singleton)         |
                              | -O(1) lookup tables |
                              | -context mappings   |
                              | -optimization cache |
                              +---------------------+
```

### System Design Diagram (High-Level Flow)

```ascii
+-----------------+      +-----------------+      +-----------------+
|   Input Handler |----->|   Main Game     |<-----|   User (Player) |
| (Window/GLFW)   |      |   Loop (main.cpp)|      +-----------------+
+-----------------+      +-----------------+
      |                        |         ^
      | (Mouse/Keyboard)       | (Update/Render) |
      v                        v         |
+-----------------+      +-----------------+
|  Camera Control |----->|      World      |
|  (Camera Class) |      | (World Class)   |
+-----------------+      +-----------------+
                               |         ^
                               | (Manages Planets)
                               v         |
                         +-----------------+
                         |     Planet      |
                         | (Planet Class)  |
                         +-----------------+
                               |         ^
                               | (Manages Chunks) |
                               v         |
                         +-----------------+
                         |  Chunk (Visible)|
                         |  (Chunk Class)  |
                         |  (Spherical or  |
                         |   Flat)         |
                         +-----------------+
                               |         ^
                               | (Block Data) |
                               v         |
                         +-----------------+      +-----------------+
                         |      Block      |<---->|  BlockRegistry  |
                         |  (Block Class)  |      | (Singleton)     |
                         +-----------------+      | -Definitions    |
                               |         ^        | -Contexts       |
                               | (Shader/Texture) | -Optimizations  |
                               v         |        +-----------------+
+-----------------+      +-----------------+      +-----------------+
|   Shader Manager|----->| Rendering Engine|<-----| Texture Manager |
|  (Shader Class) |      | (OpenGL Calls)  |      | (Texture Class) |
+-----------------+      +-----------------+      +-----------------+
                                   ^
                                   | (Draw 2D)
                                   |
                         +-----------------+
                         |    Crosshair    |
                         | (Crosshair Class)|
                         +-----------------+
```

### Class Diagram (Key Classes and Relationships - UML-like)

```ascii
+---------------------+       +---------------------+       +---------------------+
|        main()       |------>|       Window        |<------|        GLFW         |
+---------------------+       +---------------------+       +---------------------+
           |                          |       ^
           | (Creates & Uses)         | (Input Callbacks)
           v                          v
+---------------------+       +---------------------+
|       Camera        |       |      Crosshair      |
+---------------------+       +---------------------+
           |                          |
           | (Provides View)          | (Uses Shader)
           v                          v
+---------------------+       +---------------------+
|        World        |<----->|       Shader        |  (Shader is used by Block, Crosshair)
| -planets_ (vector)  |       +---------------------+
| -defaultSeed_       |
| -workerThread_      |
| +addPlanet()        |
| +update(camera)     |
| +render(...)        |
| +getBlockAtWorldPos()|
+---------------------+
           |
           | (Manages)
           v
+---------------------+
|       Planet        |
| -position_, radius_ |
| -chunks_ (map)      |
| +update(cam, world) |
| +render(...)        |
| +getBlockAtWorldPos()|
+---------------------+
           |
           | (Manages & Renders)
           v
+---------------------+
|        Chunk        |
| -position           |
| -planetContext_ (opt)|
| -blocks (3D vector) |
| -surfaceMesh (VAO)  |
| +setPlanetContext() |
| +ensureInitialized()| // Takes planet context
| +generateTerrain()  | // Takes planet context
| +buildSurfaceMesh() | // Takes planet context
| +renderSurface()    |
+---------------------+
           |
           | (Contains)
           v
+---------------------+       +---------------------+       +---------------------+
|        Block        |------>|       Texture       |<----->|   BlockRegistry     |
| -block_type_id      |       +---------------------+       | (Singleton)         |
| +isSolid()          |                                     | +selectBlock()      |
| +isTransparent()    |                                     | +getRenderData()    |
| +getBlockName()     |                                     | +isBlockSolid()     |
+---------------------+                                     +---------------------+
```

---

## File Details

### `main.cpp` in `./`
This file serves as the main entry point for the AzureVoxel application. It initializes GLFW, GLEW, creates the game window, and sets up shared resources including the Block Registry system. The main loop creates a `World` object, to which `Planet` objects can be added. The main loop calls the `World`'s update and render methods, which in turn manage the planets and their chunks. A `Camera` and a 2D `Crosshair` are also managed here.

*   **`main()`** - Primary application function.
    *   Initializes GLFW and GLEW.
    *   Creates a `Window` object.
    *   Enables mouse capture for FPS camera control.
    *   **Initializes `BlockRegistry::getInstance().initialize()` before any block operations.**
    *   Initializes `Block::InitBlockShader()` and `Block::InitSpritesheet()`.
    *   Enables depth testing and face culling.
    *   Creates a `Camera` object, with adjusted starting position and speed suitable for planetary scales.
    *   Creates a `World` object (e.g., `new World("SolarSystem")`).
    *   Calls `world->addPlanet(...)` to add one or more planets with specified positions, radii, seeds, and names.
    *   Creates a `Crosshair` object.
    *   Enters the main game loop:
        *   Calculates `deltaTime` and FPS.
        *   Processes keyboard and mouse input for camera movement.
        *   Calls `world->update(camera)` to update all planets and their chunks. This includes triggering chunk generation/meshing via the world's worker thread system.
        *   Calls `world->processMainThreadTasks()` for OpenGL operations queued by worker threads.
        *   Calculates view and projection matrices (projection matrix has an increased far plane for viewing planets).
        *   Clears the screen.
        *   Calls `world->render(...)` to draw all planets.
        *   Calls `crosshair->render()` to draw the 2D crosshair.
        *   Swaps window buffers and polls events.
    *   After the loop, cleans up resources (`Block::CleanupBlockShader()`, `BlockRegistry::getInstance().shutdown()`, deletes `crosshair`, `world`, `camera`).

---

### `headers/block_registry.h` in `headers/`
This header defines the comprehensive Block Registry system that manages all block definitions, variants, and runtime optimizations. The system implements a singleton pattern with sophisticated caching and context-aware block selection.

*   **Core Data Structures:**
    *   **`BlockDefinition`** - Immutable block properties including ID, display name, physical properties (hardness, blast resistance), rendering info (textures), and variant definitions.
        *   `id` (string): Namespaced identifier like "azurevoxel:stone"
        *   `numeric_id` (uint16_t): Runtime numeric ID for fast lookups
        *   `display_name` (string): Human-readable name
        *   Core properties: `solid`, `transparent`, `light_emission`, `hardness`, `blast_resistance`, `flammable`
        *   Texture information: `default_texture`, `per_face_textures` map
        *   `variants` map: Context-specific overrides
    *   **`BlockVariant`** - Context-specific overrides for base blocks
        *   `base_block_id` (uint16_t): Reference to base block
        *   `context_name` (string): Context identifier
        *   Optional overrides: `texture_override`, `display_name_override`, `hardness_override`, `solid_override`
    *   **`BlockRenderData`** - Optimized runtime cache for hot-path operations
        *   `texture_atlas_index` (uint16_t): Direct GPU texture index
        *   `cull_mask` (uint8_t): Bit flags for face culling optimization
        *   `light_level` (uint8_t): 0-15 light emission value
        *   `flags` (uint8_t): Packed boolean properties (solid, transparent, light source)
        *   Bit flag constants and accessor methods for performance
    *   **`BiomeContext`** - Environmental context for biome-aware block selection
        *   `biome_id` (string): Biome identifier
        *   `temperature` (float): -1.0 to 1.0 (cold to hot)
        *   `moisture` (float): -1.0 to 1.0 (dry to wet)
        *   `atmospheric_pressure` (float): Atmospheric conditions
        *   `preferred_materials` (string): Material preference hints
    *   **`PlanetContext`** - Planetary context for planet-specific overrides
        *   `planet_id` (string): Planet identifier
        *   `gravity_modifier` (float): Gravity scaling factor
        *   `atmosphere_type` (string): Atmospheric composition
        *   `geological_composition` (string): Geological material preferences
        *   `material_overrides` map: Planet-specific material mappings
    *   **`ContextKey`** - Optimized key structure for context lookups
        *   Packed 16-bit structure with biome_id (8-bit) and planet_id (8-bit)
        *   Custom hash function for unordered_map usage

*   **`BlockRegistry`** - Main singleton class managing the entire block system
    *   **Constants:**
        *   `MAX_BLOCK_TYPES` (4096): Maximum supported block types
        *   `MAX_CONTEXTS` (256): Maximum context combinations
        *   `INVALID_BLOCK_ID` (0xFFFF): Invalid block identifier
    *   **Initialization Methods:**
        *   `getInstance()` - Singleton access pattern
        *   `initialize(blocks_directory)` - Initialize registry from file definitions
        *   `shutdown()` - Cleanup and resource deallocation
    *   **Registration Methods:**
        *   `registerBlock(definition)` - Register new block definition
        *   `registerVariant(variant)` - Register context-specific variant
        *   `registerBiome(biome)` - Register biome context (returns numeric ID)
        *   `registerPlanet(planet)` - Register planet context (returns numeric ID)
    *   **Hot-Path Lookup Methods (O(1) performance):**
        *   `getRenderData(block_id)` - Get optimized render data
        *   `isBlockSolid(block_id)` - Fast solidity check
        *   `isBlockTransparent(block_id)` - Fast transparency check
        *   `getBlockLightLevel(block_id)` - Fast light level query
        *   `shouldRenderFace(block_id, neighbor_id)` - Optimized face culling
    *   **Context-Aware Selection Methods:**
        *   `selectBlock(base_name, biome, planet)` - Select block variant by name and contexts
        *   `selectBlock(base_id, biome_id, planet_id)` - Select block variant by IDs
    *   **Information Query Methods:**
        *   `getBlockDefinition(block_id/name)` - Get full block definition
        *   `getBlockId(name)` - Convert name to numeric ID
        *   `getBlockName(block_id)` - Convert numeric ID to name
        *   `getTextureIndex(texture_name)` - Get texture atlas index
    *   **Utility Methods:**
        *   `printRegistryStats()` - Debug information output
        *   `reloadDefinitions(directory)` - Hot-reload block definitions

---

### `src/block_registry.cpp` in `src/`
Implementation of the Block Registry system with comprehensive functionality for block management, context handling, and performance optimization.

*   **`initialize(blocks_directory)`** - Main initialization method
    *   Clears all existing data structures
    *   Initializes optimization arrays (render_data_, context_map_)
    *   Calls `createDefaultBlocks()` for backward compatibility
    *   Loads external block definitions from files (.json/.txt)
    *   Registers default biomes (temperate, cold, hot, water) and planets (earth, mars)
    *   Calls `buildOptimizationTables()` for performance caching
    *   Sets initialized flag and outputs statistics
*   **`createDefaultBlocks()`** - Creates hardcoded default blocks for backward compatibility
    *   Air (0): Non-solid, transparent
    *   Stone (1): Basic solid block with high hardness
    *   Grass (2): Surface block with moderate hardness
    *   Dirt (3): Subsurface block
    *   Sand (4): Granular material
    *   Water (5): Non-solid, transparent liquid
    *   Snow (6): Cold climate surface material
    *   Wood Log (7): Flammable structural material
    *   Leaves (8): Non-solid, transparent, flammable
    *   Gravel (9): Rocky debris material
    *   Gold Ore (10): Valuable mineral with high hardness
*   **`registerBlock(definition)`** - Register new block with conflict checking
    *   Validates numeric ID within bounds
    *   Checks for ID conflicts with existing blocks
    *   Expands storage vectors if needed
    *   Updates name-to-ID and ID-to-name mappings
    *   Builds render data cache entry
    *   Registers texture name if not already present
*   **Hot-Path Lookup Methods** - Optimized O(1) operations
    *   `getRenderData(block_id)` - Direct array access to cached render data
    *   `isBlockSolid(block_id)` - Bit flag check on cached data
    *   `isBlockTransparent(block_id)` - Bit flag check on cached data
    *   `getBlockLightLevel(block_id)` - Direct field access
*   **Context-Aware Selection** - Biome and planet-aware block selection
    *   `selectBlock(base_name, biome, planet)` - Converts contexts to IDs and delegates
    *   `selectBlock(base_id, biome_id, planet_id)` - Uses pre-computed context_map_ for O(1) lookup
    *   Falls back to base block if no context-specific variant exists
*   **File Loading** - External block definition support with multiple formats
    *   `loadBlockDefinitionFile(file_path)` - Multi-format block definition loader
        *   Automatically detects file format by extension (.json or .txt)
        *   Routes to appropriate parser based on file type
        *   Supports both JSON and simple text formats for maximum flexibility
    *   `loadBlockDefinitionFromJSON(file)` - Comprehensive JSON parser
        *   Parses structured JSON files with "blocks" array format
        *   Supports all block properties: ID, display name, physical properties, rendering settings
        *   Handles optional fields with sensible defaults
        *   Provides detailed error reporting for malformed JSON
        *   Enables rich block definitions with metadata
    *   `loadBlockDefinitionFromText(file)` - Simple text format parser
        *   Maintains backward compatibility with existing `.txt` format
        *   Format: "id numeric_id display_name texture_name solid_flag"
        *   Supports comments (lines starting with #)
        *   Ideal for quick prototyping and simple mod definitions
    *   **JSON Helper Methods** - Robust JSON parsing utilities
        *   `parseJSONString()` - Extracts string values with fallback defaults
        *   `parseJSONNumber()` - Parses numeric values with type conversion
        *   `parseJSONBool()` - Handles boolean values with validation
        *   All methods include comprehensive error handling and validation
*   **Optimization Methods** - Performance enhancement systems
    *   `buildOptimizationTables()` - Pre-computes frequently accessed data
    *   `rebuildContextMap()` - Updates context-to-block mappings
    *   `findOrCreateVariant()` - Lazy creation of context-specific variants

---

### `headers/block.h` in `headers/`
Updated Block class definition that integrates with the Block Registry system while maintaining backward compatibility. The class now serves as a lightweight wrapper around registry-managed block data.

*   **`Block`** - Individual voxel block with registry integration
    *   **Private Members:**
        *   OpenGL objects for individual rendering: `VAO`, `VBO`, `EBO`, `texCoordVBO`
        *   Block properties: `position`, `color`, `size`
        *   **`block_type_id` (uint16_t)**: Registry-managed block type identifier
        *   Texture handling: `texture`, `hasTexture` flag
        *   Movement properties: `speed`
    *   **Static Members:**
        *   `shaderProgram` (GLuint): Shared shader program for all blocks
        *   `spritesheetTexture` (Texture): Global texture atlas
        *   `spritesheetLoaded` (bool): Atlas loading status flag
    *   **Static Initialization Methods:**
        *   `InitBlockShader()` - Initialize shared shader program with vertex/fragment shaders
        *   `InitSpritesheet(path)` - Load global texture atlas
        *   `CleanupBlockShader()` - Cleanup shader resources
    *   **Backward Compatibility Methods:**
        *   `isTypeSolid(blockType)` - Static method using registry lookup (maintains old API)
        *   Overloaded for both unsigned char and uint16_t block types
    *   **Constructors:**
        *   `Block(position, color, size)` - Default constructor (air block)
        *   `Block(position, blockType, color, size)` - Constructor with registry block type
    *   **Registry-Powered Property Queries:**
        *   `isSolid()` - Instance method using registry lookup
        *   `isTransparent()` - Instance method using registry lookup
        *   `isLightSource()` - Instance method using registry lookup
        *   `getLightLevel()` - Instance method using registry lookup
        *   `getBlockName()` - Instance method using registry lookup
    *   **Block Type Management:**
        *   `setBlockType(typeId)` - Update block type with registry validation
        *   `getBlockType()` - Get current block type ID
    *   **Rendering Methods:**
        *   `init()` - Initialize OpenGL objects for individual block rendering
        *   `render(projection, view)` - Render single block (less common with chunk meshing)
        *   Helper methods: `useBlockShader()`, `bindBlockTexture()`, `setShaderUniforms()`
    *   **Utility Methods:**
        *   `move(offset, deltaTime)` - Move block with speed consideration
        *   `setPosition(newPosition)` - Set absolute position
        *   Texture loading: `loadTexture()` methods for individual and atlas textures
        *   `shareTextureAndShaderFrom(other)` - Share resources between blocks

---

### `src/block.cpp` in `src/`
Implementation of the Block class with full integration to the Block Registry system. The implementation maintains backward compatibility while leveraging the registry for all property queries and type management.

*   **Static Shader Management:**
    *   `InitBlockShader()` - Comprehensive shader compilation and linking
        *   Compiles vertex and fragment shaders from embedded source code
        *   Handles compilation error checking and reporting
        *   Links shaders into program and validates
        *   Stores program ID in static `shaderProgram` variable
        *   Includes OpenGL error checking and cleanup on failure
    *   `CleanupBlockShader()` - Proper OpenGL resource cleanup
    *   `InitSpritesheet(path)` - Global texture atlas loading with error handling
*   **Registry Integration:**
    *   **Constructors** - Initialize with registry-compatible block types
        *   Default constructor creates air block (type 0)
        *   Type-specific constructor accepts uint16_t block type ID
        *   Both constructors properly initialize all member variables
    *   **Property Query Methods** - All delegate to BlockRegistry
        *   `isSolid()` - Uses `BlockRegistry::getInstance().isBlockSolid(block_type_id)`
        *   `isTransparent()` - Uses `BlockRegistry::getInstance().isBlockTransparent(block_type_id)`
        *   `isLightSource()` - Checks light level > 0 via registry
        *   `getLightLevel()` - Uses `BlockRegistry::getInstance().getBlockLightLevel(block_type_id)`
        *   `getBlockName()` - Uses `BlockRegistry::getInstance().getBlockName(block_type_id)`
    *   **Backward Compatibility** - Static methods maintain old API
        *   `isTypeSolid(unsigned char)` - Converts to uint16_t and uses registry
        *   `isTypeSolid(uint16_t)` - Direct registry lookup
        *   Ensures existing code continues to work without modification
*   **Block Type Management:**
    *   `setBlockType(typeId)` - Updates block type with registry validation
        *   Validates type ID exists in registry
        *   Updates internal `block_type_id` member
        *   Could trigger visual updates if needed
    *   `getBlockType()` - Simple getter for current type ID
*   **Individual Block Rendering:**
    *   `init()` - OpenGL VAO/VBO/EBO setup for single block rendering
        *   Creates cube geometry with proper vertex attributes
        *   Sets up position (location 0) and texture coordinate (location 1) attributes
        *   Handles texture coordinate VBO separately for flexibility
        *   Includes proper OpenGL state management
    *   `render(projection, view)` - Single block rendering (used for debugging)
        *   Uses shared shader program and global spritesheet
        *   Calculates model matrix from block position
        *   Sets all required shader uniforms
        *   Performs OpenGL draw call with proper state management
    *   **Shader Utility Methods:**
        *   `useBlockShader()` - Activates shared shader program
        *   `bindBlockTexture()` - Binds block's texture or global spritesheet
        *   `setShaderUniforms()` - Sets model/view/projection matrices and texture uniforms
*   **Texture Management:**
    *   `loadTexture(filepath)` - Individual texture loading with file existence checking
    *   `loadTexture(spritesheet, x, y, w, h)` - Atlas-based texture loading
    *   `shareTextureAndShaderFrom(other)` - Resource sharing between blocks
    *   Proper error handling and logging for all texture operations
*   **Movement and Positioning:**
    *   `move(offset, deltaTime)` - Time-based movement with speed consideration
    *   `setPosition(newPosition)` - Direct position setting
    *   `getPosition()`, `getColor()` - Simple getters

---

### `headers/chunk.h` in `headers/`
The `Chunk` class definition has been updated to support being part of a spherical planet or a traditional flat world, with full integration to the Block Registry system for biome-aware terrain generation.

*   **Enhanced Members:**
    *   `planetCenter_` (std::optional<glm::vec3>): Stores the center of the planet this chunk belongs to, if any.
    *   `planetRadius_` (std::optional<float>): Stores the radius of the planet this chunk belongs to, if any.
    *   `blocks_` (3D vector): Stores shared_ptr<Block> instances with registry-managed types
    *   `blockDataForInitialization_` (3D vector): Stores BlockInfo with registry type IDs for meshing
    *   `surfaceMesh`: Optimized mesh data (VAO, VBO, EBO, indexCount)
*   **Registry Integration Methods:**
    *   `setPlanetContext(planetCenter, planetRadius)` - Sets planetary context for spherical generation
    *   `ensureInitialized(world, seed, planetCenter, planetRadius)` - Main initialization with registry-aware generation
    *   `generateTerrain(seed, planetCenter, planetRadius)` - Registry-powered terrain generation
    *   `buildSurfaceMesh(world, planetCenter, planetRadius)` - Registry-optimized mesh building
*   **Biome-Aware Generation:**
    *   Terrain generation now uses `BlockRegistry::selectBlock()` for context-aware block selection
    *   Supports biome contexts (cold, hot, temperate, water) for environmental adaptation
    *   Planet contexts enable planetary material overrides
*   **Performance Optimizations:**
    *   Uses `BlockRegistry::shouldRenderFace()` for optimized face culling
    *   Leverages registry's O(1) lookup tables for block property queries
    *   Pre-computed texture atlas indices for efficient GPU rendering
*   Other members (`position`, `needsRebuild`, `isInitialized_`) and methods (`renderSurface`, `getBlockAtLocal`, etc.) remain conceptually similar but operate with registry integration.

---

### `src/chunk.cpp` in `src/`
The `Chunk` class implementation has been significantly enhanced to support the Block Registry system with biome-aware terrain generation, context-sensitive block placement, and automatic data persistence.

*   **Automatic Chunk Data Persistence:**
    *   `ensureInitialized()` - Enhanced initialization with automatic save/load functionality
        *   First attempts to load chunk data from saved files using `loadFromFile_DataOnly()`
        *   If loading fails, generates new terrain using `generateTerrain()` 
        *   Automatically saves newly generated chunks to disk using `saveToFile()`
        *   Provides console feedback for successful loads and saves
        *   Integrates with world's data directory structure (`chunk_data/[world_name]/`)
    *   `saveToFile(directoryPath)` - Binary chunk data serialization
        *   Creates world-specific directories automatically
        *   Saves block type data in efficient binary format
        *   Uses filename pattern: `chunk_[x]_[y]_[z].chunk`
        *   Handles file I/O errors gracefully with logging
    *   `loadFromFile_DataOnly(directoryPath, world)` - Binary chunk data deserialization
        *   Validates file existence and size before loading
        *   Performs integrity checks on loaded data
        *   Populates `blockDataForInitialization_` for subsequent mesh building
        *   Returns success/failure status for proper fallback handling
*   **Registry-Powered Terrain Generation:**
    *   `generateTerrain(seed, pCenterOpt, pRadiusOpt)` - Enhanced terrain generation
        *   **Flat Terrain Mode**: Uses registry to get block IDs by name (`azurevoxel:stone`, `azurevoxel:grass`)
        *   **Spherical Terrain Mode**: Implements sophisticated biome-aware block selection
            *   Determines biome context based on material noise (cold, hot, temperate)
            *   Uses `BlockRegistry::selectBlock()` for context-aware block placement
            *   Implements depth-based material layering (surface, sub-surface, deep)
            *   Supports ore generation with noise-based placement
            *   Includes water level implementation for planetary water bodies
        *   Creates Block instances with registry-managed type IDs
        *   Populates both `blocks_` (for game logic) and `blockDataForInitialization_` (for meshing)
*   **Optimized Mesh Building:**
    *   `buildSurfaceMesh(world, pCenterOpt, pRadiusOpt)` - Registry-optimized mesh generation
        *   **Face Culling**: Uses `BlockRegistry::shouldRenderFace()` for optimized visibility checks
        *   **Texture Mapping**: Leverages registry's texture atlas system for UV coordinate calculation
        *   **Performance**: O(1) block property lookups via registry cache
        *   **Spherical Meshing**: Handles planetary boundary conditions with registry-aware neighbor checks
        *   **Flat Meshing**: Maintains compatibility with traditional flat world generation
*   **Biome-Aware Block Selection:**
    *   Implements noise-based biome determination using material noise values
    *   Creates appropriate BiomeContext objects (cold, hot, temperate, water)
    *   Uses default PlanetContext for earth-like conditions
    *   Supports depth-based material selection:
        *   Surface layer (depth < 1.5): Biome-appropriate surface materials
        *   Sub-surface layer (depth < 5.0): Soil and debris materials
        *   Deep layer (depth > 5.0): Stone with occasional ore deposits
*   **Registry Integration Points:**
    *   All block type queries use registry methods instead of hardcoded values
    *   Block creation uses registry-managed type IDs
    *   Face culling leverages registry's optimized `shouldRenderFace()` method
    *   Texture atlas mapping uses registry's texture index system
*   **Performance Optimizations:**
    *   Pre-computed lookup tables for block properties
    *   Branch-free face culling using registry bit flags
    *   Optimized neighbor checking with registry-aware boundary conditions
    *   Efficient texture atlas UV calculation using registry indices
*   **Backward Compatibility:**
    *   Maintains support for both flat and spherical terrain generation
    *   Preserves existing chunk loading/saving interfaces
    *   Supports legacy block type checking through registry delegation

---

### `headers/world.h` in `headers/`
The `World` class has been refactored to manage `Planet` objects instead of a flat grid of `Chunk`s, with full integration to the Block Registry system for coordinated block management.

*   **Member Changes:**
    *   `chunks` (std::unordered_map): Removed in favor of planet-based management.
    *   `planets_` (std::vector<std::shared_ptr<Planet>>): New member to store all planets in the world.
    *   `renderDistance`, `chunkSize`: Removed as they were specific to the flat chunk grid.
    *   `worldName_` (std::string): Name for the world, used in data paths.
    *   `defaultSeed_` (int): A default seed for planets if not specified.
*   **Registry Integration:**
    *   World initialization ensures Block Registry is properly initialized before planet creation
    *   Coordinates with registry for consistent block type management across all planets
    *   Supports registry-aware block queries for cross-planet operations
*   **New Methods:**
    *   `World(worldName, defaultSeed)` (Constructor): Initializes with a world name and seed. Creates world-specific data directories. Starts the worker thread.
    *   `addPlanet(position, radius, seed, name)`: Creates a new `Planet` and adds it to the `planets_` vector.
*   **Modified Methods:**
    *   `~World()`: Manages cleanup of planets and the worker thread.
    *   `update(camera)`: Iterates through `planets_` and calls `planet->update(camera, this)`. `this` (world context) is passed so planets can queue tasks to the world's worker thread. Also calls `processMainThreadTasks()`.
    *   `render(projection, view, camera, wireframeState)`: Iterates through `planets_` and calls `planet->render(...)`.
    *   `getBlockAtWorldPos(worldPos)`: Iterates through `planets_`. For each planet, it performs a rough bounding sphere check. If `worldPos` is potentially within the planet, it calls `planet->getBlockAtWorldPos()`.
    *   `addTaskToWorker(task)`: Renamed from `addTask` for clarity. Adds a task to the background worker thread's queue.
*   The worker thread system (`workerThread_`, `queueMutex_`, `queueCondition_`, `taskQueue_`, `shouldTerminate_`, `workerThreadFunction`, `addMainThreadTask`, `processMainThreadTasks`) is largely preserved. It's now used for tasks queued by planets for their chunks (e.g., `Chunk::ensureInitialized`).

---

### `src/world.cpp` in `src/`
Implementation of the refactored `World` class with full Block Registry integration and planet-based world management.

*   **Constructor `World(worldName, defaultSeed)`**: 
    *   Initializes members and creates world data directories using `std::filesystem::create_directories`
    *   Starts `workerThread_` for background chunk processing
    *   Ensures Block Registry is initialized and available for planet operations
*   **Destructor `~World()`**: 
    *   Signals worker thread to terminate and joins it
    *   Clears `planets_` vector with proper cleanup
    *   Coordinates with Block Registry for any necessary cleanup
*   **Planet Management:**
    *   `addPlanet(position, radius, seed, name)`: Creates `std::make_shared<Planet>` and adds to `planets_`
    *   Ensures each planet has access to the Block Registry for consistent block management
*   **Update and Rendering:**
    *   `update(camera)`: Loops through `planets_`, calls `planet->update(camera, this)`, processes main thread tasks
    *   `render(projection, view, camera, wireframeState)`: Loops through `planets_`, calls `planet->render(...)`
    *   Coordinates registry-aware rendering across all planets
*   **Block Query Integration:**
    *   `getBlockAtWorldPos(worldPos)`: 
        *   Loops through `planets_` with bounding sphere checks
        *   Uses registry-aware block queries for consistent results
        *   Handles cross-planet block lookups efficiently
*   **Worker Thread System:**
    *   `workerThreadFunction()`: Waits for tasks on `queueCondition_`, executes tasks from `taskQueue_`
    *   `addTaskToWorker(task)`: Pushes task to `taskQueue_`, notifies condition variable
    *   `addMainThreadTask(task)`: Queues OpenGL operations for main thread execution
    *   `processMainThreadTasks()`: Executes queued main thread tasks (typically OpenGL operations)
    *   Supports registry-aware chunk initialization and mesh building tasks

---

### `res/blocks/example_blocks.txt` in `res/blocks/`
Simple text format block definition file demonstrating backward-compatible block system configuration. This file shows how blocks can be defined using a straightforward text format for quick prototyping and simple modifications.

*   **File Format:**
    *   Simple text format: `block_id numeric_id display_name texture_name solid_flag`
    *   Comments supported with `#` prefix
    *   Lightweight format ideal for rapid development and testing
*   **Example Definitions:**
    *   `azurevoxel:cobblestone 11 Cobblestone cobblestone true` - Structural building material
    *   `azurevoxel:sandstone 12 Sandstone sandstone true` - Desert building material
    *   `azurevoxel:ice 13 Ice ice true` - Cold climate material
    *   `azurevoxel:obsidian 14 Obsidian obsidian true` - Volcanic material
*   **Registry Integration:**
    *   Loaded automatically during `BlockRegistry::initialize()`
    *   Supports hot-reloading via `BlockRegistry::reloadDefinitions()`
    *   Maintains full compatibility with existing workflows

### `res/blocks/blocks.json` in `res/blocks/`
Comprehensive JSON format block definition file demonstrating the advanced block system capabilities. This file shows how complex block properties can be defined using structured JSON for rich metadata and detailed block behavior configuration.

*   **File Format:**
    *   Structured JSON with "blocks" array containing block objects
    *   Supports all block properties: physical, visual, and behavioral
    *   Rich metadata support with optional fields and defaults
    *   Extensible format for future enhancements
*   **Supported Properties:**
    *   Core identification: `id`, `numeric_id`, `display_name`
    *   Physical properties: `hardness`, `blast_resistance`, `flammable`
    *   Visual properties: `texture`, `solid`, `transparent`, `light_emission`
    *   Future-ready: Easy to extend with new properties
*   **Example Definitions:**
    *   Glowstone: Light-emitting block with emission level 15
    *   Lava: Non-solid, transparent liquid with light emission 12
    *   Bedrock: Indestructible block with infinite blast resistance
    *   Ice: Transparent solid with reduced hardness for cold biomes
*   **Registry Integration:**
    *   Automatically parsed by enhanced `BlockRegistry::loadBlockDefinitionFromJSON()`
    *   Supports real-time reloading for development and modding
    *   Enables complex block behavior without code compilation
    *   Provides foundation for advanced modding capabilities

---

## Chunk Data Persistence System

### Overview
The AzureVoxel engine implements an automatic chunk data persistence system that seamlessly saves and loads chunk data to reduce generation overhead and maintain world consistency. The system operates transparently during chunk initialization, automatically handling file I/O operations based on world-specific directory structures.

### Persistence Architecture

**File Organization:**
- World-specific directories: `chunk_data/[world_name]/`
- Chunk filename pattern: `chunk_[x]_[y]_[z].chunk`
- Binary format for efficient storage and fast loading
- Automatic directory creation with proper error handling

**Save/Load Workflow:**
1. **Chunk Initialization**: `ensureInitialized()` first attempts to load existing chunk data
2. **Load Attempt**: Uses `loadFromFile_DataOnly()` to check for saved chunk files
3. **Generation Fallback**: If no file exists, generates new terrain using `generateTerrain()`
4. **Automatic Save**: Newly generated chunks are automatically saved using `saveToFile()`
5. **Mesh Building**: Both loaded and generated chunks proceed to mesh building phase

**Performance Benefits:**
- Eliminates redundant terrain generation for previously created chunks
- Reduces world loading times for established areas
- Maintains consistency across game sessions
- Supports incremental world expansion with mixed loaded/generated chunks

### Data Format
The chunk persistence system uses a compact binary format optimized for fast I/O:

**Binary Structure:**
- Sequential block type data: `[x][y][z].type` values
- Fixed size: `CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(BlockInfo::type)`
- Platform-independent binary format
- Integrity validation on load with size verification

This persistence system seamlessly integrates with the Block Registry, ensuring that saved chunk data properly references registered block types and maintains compatibility across registry updates.

---

## Block Registry System Architecture

### Core Components

The Block Registry system implements a sophisticated component-based architecture designed for performance, modularity, and extensibility:

**1. Block Definition Layer**
- `BlockDefinition`: Immutable core block properties
- `BlockVariant`: Context-specific overrides and adaptations
- Namespace support for modular block organization
- File-based definition loading for data-driven design

**2. Context System**
- `BiomeContext`: Environmental conditions (temperature, moisture, atmospheric pressure)
- `PlanetContext`: Planetary characteristics (gravity, atmosphere, geology)
- `ContextKey`: Optimized lookup structure for context combinations
- Pre-computed context mapping tables for O(1) performance

**3. Performance Optimization Layer**
- `BlockRenderData`: Cache-friendly structure for hot-path operations
- Bit-packed flags for boolean properties
- Direct texture atlas indices for GPU efficiency
- Branch-free face culling using bit masks

**4. Registry Management**
- Singleton pattern for global access
- Thread-safe operations for multi-threaded chunk generation
- Hot-reloading support for development and modding
- Comprehensive error handling and validation

### Performance Characteristics

**Lookup Performance:**
- Block property queries: O(1) array access
- Context-aware selection: O(1) pre-computed table lookup
- Face culling decisions: O(1) bit flag operations
- Texture atlas mapping: O(1) direct index access

**Memory Efficiency:**
- Structure of Arrays layout for cache efficiency
- Bit-packed boolean properties to minimize memory usage
- Pre-computed lookup tables to avoid runtime calculations
- Lazy loading of context-specific variants

**Scalability:**
- Supports up to 4,096 different block types
- Handles up to 256 context combinations
- Efficient batch operations for chunk generation
- Minimal memory overhead per block instance

### Integration Points

**Terrain Generation:**
- Biome-aware block selection using environmental noise
- Depth-based material layering for realistic geology
- Ore distribution using secondary noise functions
- Water level implementation for planetary bodies

**Rendering System:**
- Optimized face culling using registry bit flags
- Direct texture atlas UV mapping
- Efficient neighbor checking for mesh generation
- Support for both opaque and transparent block rendering

**Game Logic:**
- Registry-powered block property queries
- Context-sensitive block behavior
- Modular block interaction system
- Support for custom block properties and behaviors

This architecture successfully transforms the rigid hardcoded block system into a flexible, performance-optimized, data-driven framework that supports environmental adaptation and modular expansion while preserving all existing game functionality.

---

## Non-Solid Block Rendering System

### Overview
The AzureVoxel engine implements a sophisticated non-solid block rendering system that allows players to see through transparent or non-solid blocks like water, air, and leaves. This system ensures that faces of solid blocks are properly rendered when they are adjacent to non-solid blocks, creating realistic visibility through water bodies and other transparent materials.

### Block Solidity Classification
The system classifies blocks into solid and non-solid categories using the `BlockRegistry::isBlockSolid()` method, which provides O(1) lookup performance through pre-computed bit flags:

**Non-Solid Blocks:**
- Air (type 0) - Empty space
- Water (type 5) - Liquid blocks in planet cores and water bodies  
- Leaves (type 8) - Tree foliage (transparent)

**Solid Blocks:**
- Stone (type 1) - Basic rock material
- Grass (type 2) - Surface grass blocks
- Dirt (type 3) - Subsurface soil
- Sand (type 4) - Desert/beach material
- Snow (type 6) - Cold climate surface
- Wood Log (type 7) - Tree trunks
- Gravel (type 9) - Rocky debris
- Gold Ore (type 10) - Precious metal deposits

### Face Culling Logic
The mesh building system in `Chunk::buildSurfaceMesh()` implements intelligent face culling using the Block Registry's optimized `shouldRenderFace()` method:

**Rendering Rules:**
1. **Solid-to-Non-Solid Interface**: Always render the solid block's face when adjacent to non-solid blocks
2. **Solid-to-Solid Interface**: Cull faces between adjacent solid blocks (hidden from view)
3. **Chunk Boundaries**: Render faces at chunk edges for solid blocks (assumes air beyond chunk)
4. **Planetary Boundaries**: Special handling for spherical chunks where blocks outside planet radius are treated as air

**Performance Optimization:**
- Uses bit flags in `BlockRenderData` for branch-free face culling decisions
- Pre-computed neighbor checking with O(1) registry lookups
- Optimized for both flat terrain and spherical planet generation
- Supports efficient batch processing during mesh generation

### Registry Integration
The face culling system is fully integrated with the Block Registry:

```cpp
// Example usage in chunk mesh building
uint16_t currentBlockType = blockDataForInitialization_[x][y][z].type;
uint16_t neighborBlockType = blockDataForInitialization_[nx][ny][nz].type;

// O(1) registry lookup for face culling decision
bool shouldRender = registry.shouldRenderFace(currentBlockType, neighborBlockType);
```

This system ensures optimal rendering performance while maintaining visual fidelity for complex voxel environments with mixed solid and transparent materials.