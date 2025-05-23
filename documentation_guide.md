# AzureVoxel Project Documentation

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
│   ├── camera.h
│   ├── chunk.h
│   ├── crosshair.h
│   ├── planet.h       // New: Planet class header
│   ├── shader.h
│   ├── texture.h
│   ├── window.h
│   └── world.h
├── main.cpp
├── res/
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
    ├── camera.cpp
    ├── chunk.cpp
    ├── crosshair.cpp
    ├── planet.cpp       // New: Planet class implementation
    ├── shader.cpp
    ├── texture.cpp
    ├── window.cpp
    └── world.cpp
```

---

## Game Architecture and Design

### Conceptual Overview

AzureVoxel is a voxel-based 3D environment/game engine. The core architecture revolves around a `World` which can contain multiple `Planet` objects. Each `Planet` is composed of `Chunk`s, which in turn are made up of individual `Block`s. Rendering is managed through OpenGL, with a `Window` class handling GLFW interactions, a `Camera` class for viewpoint control, and `Shader` and `Texture` classes for visual representation. A `Crosshair` class provides a 2D UI element.

The system is designed with performance in mind, particularly for rendering large voxel worlds and planets. Key aspects include:
*   **Planet-based world management:** The `World` manages planets. Planets manage their own chunks.
*   **Spherical Chunk Generation & Meshing:** `Chunk`s can now generate terrain and meshes that conform to a spherical planet surface.
*   **Optimized chunk meshing:** Each chunk (whether flat or part of a sphere) builds a single optimized mesh (`surfaceMesh`) containing only the visible faces of its blocks.
*   **Texture Atlasing:** A global spritesheet (`Block::spritesheetTexture`) is used for block textures.
*   **Multithreaded chunk loading/generation:** The `World` uses a worker thread to handle chunk data loading (if implemented for planets) or procedural generation, including tasks queued by `Planet` objects for their chunks.

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
                         +-----------------+
                         |      Block      |
                         |  (Block Class)  |
                         +-----------------+
                               |         ^
                               | (Shader/Texture)|
                               v         |
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
+---------------------+       +---------------------+
|        Block        |------>|       Texture       |
+---------------------+       +---------------------+
```

---

## File Details

### `main.cpp` in `./`
This file serves as the main entry point for the AzureVoxel application. It initializes GLFW, GLEW, creates the game window, and sets up shared resources like the `Block` shader and global spritesheet. It now creates a `World` object, to which `Planet` objects can be added. The main loop calls the `World`'s update and render methods, which in turn manage the planets and their chunks. A `Camera` and a 2D `Crosshair` are also managed here.

*   **`main()`** - Primary application function.
    *   Initializes GLFW and GLEW.
    *   Creates a `Window` object.
    *   Enables mouse capture for FPS camera control.
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
    *   After the loop, cleans up resources (`Block::CleanupBlockShader()`, deletes `crosshair`, `world`, `camera`).

---
### `headers/planet.h` in `headers/`
This new header file defines the `Planet` class. A `Planet` represents a spherical voxel-based planet. It manages its own collection of `Chunk` objects, which are arranged and generated to form the planet's sphere.

*   **`Planet`** - Represents a spherical voxel-based planet.
    *   **Private Members:**
        *   `position_` (glm::vec3): The world-space center of the planet.
        *   `radius_` (float): The radius of the planet.
        *   `seed_` (int): Seed used for procedural generation of this planet's features.
        *   `name_` (std::string): The name of the planet.
        *   `chunks_` (std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>>): Stores chunks belonging to this planet, keyed by their 3D grid position relative to the planet's center (in chunk units).
        *   `chunksInRadius_` (int): Approximate number of chunks from the planet's center to its surface along an axis.
    *   **Private Methods:**
        *   `generatePlanetStructure()`: An internal method called by the constructor to create the initial grid of `Chunk` objects that will form the planet. It determines which chunk positions are within the planet's sphere and instantiates `Chunk` objects for them, setting their planet context.
    *   **Public Methods:**
        *   `Planet(glm::vec3 position, float radius, int seed, const std::string& name = "DefaultPlanet")` (Constructor): Initializes the planet with its properties and calls `generatePlanetStructure()`.
        *   `~Planet()` (Destructor): Clears its `chunks_` map.
        *   `update(const Camera& camera, const World* world_context)`: Manages the planet's state. This is where Level of Detail (LoD), and dynamic loading/unloading/initialization of the planet's chunks would be handled. It can use `world_context` to queue tasks (like chunk initialization) to the `World`'s worker thread.
        *   `render(const glm::mat4& projection, const glm::mat4& view, bool wireframeState) const`: Renders all initialized chunks belonging to this planet.
        *   `getBlockAtWorldPos(const glm::vec3& worldPos) const` (std::shared_ptr<Block>): Retrieves a block at a specific world position if it falls within one of this planet's chunks.
        *   Getters for `position_`, `radius_`, `name_`.

---
### `src/planet.cpp` in `src/`
This new file implements the `Planet` class methods.

*   **`Planet(position, radius, seed, name)` (Constructor)**:
    *   Initializes member variables.
    *   Calculates `chunksInRadius_` based on `radius_` and `CHUNK_SIZE_X`.
    *   Calls `generatePlanetStructure()`.
*   **`~Planet()`**:
    *   Clears `chunks_`.
*   **`generatePlanetStructure()`**:
    *   Iterates in a 3D grid of chunk coordinates around the planet's `position_` up to `chunksInRadius_`.
    *   For each potential chunk position, it checks if the center of that conceptual chunk is within the planet's `radius_` (plus a small margin for corners).
    *   If it is, a new `Chunk` is created. The chunk's world position (its minimum corner) is calculated based on its grid offset from the planet's center.
    *   The new `Chunk` is stored in the `chunks_` map, keyed by its 3D grid coordinates relative to the planet.
    *   **Crucially, `chunk->setPlanetContext(position_, radius_)` should be called here (TODO in current code) to inform the chunk it's part of this sphere.**
*   **`update(camera, world_context)`**:
    *   Currently a placeholder. Iterates through its `chunks_`.
    *   For each chunk, if not initialized, it would ideally queue an initialization task via `world_context->addTaskToWorker(...)`. The task would call `chunk->ensureInitialized(world_context, seed_, position_, radius_)`.
*   **`render(projection, view, wireframeState)`**:
    *   Iterates through its `chunks_`.
    *   If a chunk is valid and `isInitialized()`, calls `chunk->renderSurface(...)`.
*   **`getBlockAtWorldPos(worldPos)`**:
    *   Calculates the `worldPos` relative to the planet's `position_`.
    *   Determines the 3D chunk key (grid coordinates relative to planet center) for this position.
    *   Looks up the chunk in `chunks_`.
    *   If found, converts `worldPos` to the chunk's local coordinates and calls `chunk->getBlockAtLocal(...)`.

---
### `headers/chunk.h` in `headers/`
The `Chunk` class definition has been updated to support being part of a spherical planet or a traditional flat world.

*   **New Members:**
    *   `planetCenter_` (std::optional<glm::vec3>): Stores the center of the planet this chunk belongs to, if any.
    *   `planetRadius_` (std::optional<float>): Stores the radius of the planet this chunk belongs to, if any.
*   **New Methods:**
    *   `setPlanetContext(const glm::vec3& planetCenter, float planetRadius)`: Sets the `planetCenter_` and `planetRadius_` for this chunk, marking it as part of a sphere.
*   **Modified Method Signatures:**
    *   `ensureInitialized(const World* world, int seed, const std::optional<glm::vec3>& planetCenter = std::nullopt, const std::optional<float>& planetRadius = std::nullopt)`: Now accepts optional planet context parameters. If provided, these are used for spherical generation/meshing. Otherwise, the chunk's stored context (if any) is used.
    *   `generateTerrain(int seed, const std::optional<glm::vec3>& planetCenter, const std::optional<float>& planetRadius)`: Internal terrain generation method, now takes planet context.
    *   `buildSurfaceMesh(const World* world, const std::optional<glm::vec3>& planetCenter, const std::optional<float>& planetRadius)`: Internal mesh building method, now takes planet context.
*   Other members (`position`, `blocks_`, `surfaceMesh`, etc.) and methods (`renderSurface`, `getBlockAtLocal`, etc.) remain conceptually similar but will operate differently based on the planet context.

---
### `src/chunk.cpp` in `src/`
The `Chunk` class implementation has significant changes to support dual-mode (flat or spherical) operation.

*   **Constructor `Chunk(position)`**: Initializes as a standard flat-world chunk by default (no planet context). `blockDataForInitialization_` and `blocks` vectors are now consistently initialized/resized here or at the start of generation/meshing methods.
*   **`setPlanetContext(planetCenter, planetRadius)`**: Stores the provided planet center and radius.
*   **`ensureInitialized(world, seed, planetCenterOpt, planetRadiusOpt)`**:
    *   Determines the effective planet context (either from direct parameters or stored members).
    *   Calls `generateTerrain` and then `buildSurfaceMesh`, passing the determined planet context.
    *   File loading/saving for planetary chunks is currently bypassed in favor of procedural generation.
*   **`generateTerrain(seed, pCenterOpt, pRadiusOpt)`**:
    *   **If no planet context (`!pCenterOpt` or `!pRadiusOpt`)**: Uses the original flat terrain generation logic (e.g., heightmap-based using Simplex noise). Populates `blocks_` (shared_ptr to Block instances) and `blockDataForInitialization_` (stores block type for meshing).
    *   **If planet context is present**:
        *   Uses the provided `seed` parameter along with Simplex noise for varied terrain generation.
        *   Calculates an `effectivePlanetRadius` for each block column, slightly varied by elevation noise, to create a non-perfect sphere surface.
        *   Places blocks based on their `distToPlanetCenter` relative to this `effectivePlanetRadius`.
        *   Uses material noise (scaled by `blockWorldPos` and `seed`) to distribute different block types:
            *   Top layer (depth < 1.5 from surface): Snow (cold areas), Grass, or Sand (hot/dry areas).
            *   Sub-surface layer (depth < 5.0): Gravel (under cold areas) or Dirt.
            *   Deeper areas (depth > 5.0): Stone, with a chance of Gold Ore based on ore noise.
        *   A basic water level is implemented: if a block is below `planetRadius * 0.7f` and the terrain surface at that point is above this level, the block becomes Water. This uses the chunk's `position.y` for a crude global Y check in relation to the planet's water sphere.
        *   Populates `blocks_` (shared_ptr to Block instances) and `blockDataForInitialization_` (stores block type for meshing).
        *   Block types used: Air (0), Stone (1), Grass (2), Dirt (3), Sand (4), Water (5), Snow (6), Gravel (9), Gold Ore (10). (Wood Log (7) and Leaves (8) are defined for UV mapping but not currently generated by noise).
        *   Individual `Block` instances created here do not load their own textures from the spritesheet; their type is recorded, and `buildSurfaceMesh` will calculate appropriate UVs for the global `Block::spritesheetTexture`.
*   **`buildSurfaceMesh(world, pCenterOpt, pRadiusOpt)`**:
    *   Clears existing mesh data.
    *   **If no planet context**: Uses the original flat terrain meshing logic. Iterates `x,y,z`. A face is rendered if the adjacent block is outside the chunk or if the adjacent block within the chunk is non-solid (checked using `!Block::isTypeSolid(neighborType)`).
    *   **If planet context is present (Spherical Meshing)**:
        *   Iterates local `x_loc,y_loc,z_loc`.
        *   Visibility/culling: A face of a solid block is rendered if the adjacent block is:
            *   Outside the chunk and also determined to be outside the planet's radius (effectively an air boundary).
            *   Outside the chunk but inside the planet's radius (currently simplified to always render these boundary faces for solid blocks; requires future refinement for true cross-chunk non-solid checks).
            *   Within the same chunk and is non-solid (checked using `!Block::isTypeSolid(neighborType)`).
        *   For visible faces:
            *   Determines block type from `blockDataForInitialization_` and calculates UV offsets for `Block::spritesheetTexture` (assumes 80x80px sprites on `Spritesheet.PNG`):
                *   Stone (1): (0,0)
                *   Grass (2): (1,0)
                *   Dirt (3): (2,0)
                *   Sand (4): (3,0)
                *   Water (5): (4,0)
                *   Snow (6): (0,1)
                *   Wood Log (7): (1,1)
                *   Leaves (8): (2,1)
                *   Gravel (9): (3,1)
                *   Gold Ore (10): (4,1)
            *   For each of the 4 vertices of the face:
                *   Calculate its `localFaceVertexPos` within the chunk.
                *   The vertex position stored in the VBO is `vboVertexPos = localFaceVertexPos` (i.e., the original block vertex position, relative to chunk origin). The previous projection onto a perfect sphere has been removed to ensure a blocky appearance.
                *   Standard UV coordinates are used, mapping to the global `Block::spritesheetTexture` based on block type.
            *   Adds these vertices and UVs to `meshVertices` and corresponding indices to `meshIndices`.
    *   After loops, if `meshVertices` is not empty, standard OpenGL VAO/VBO/EBO setup occurs.
*   The `blockDataForInitialization_` struct is used to store block types determined during `generateTerrain`, which `buildSurfaceMesh` then uses for UV mapping against the global `Block::spritesheetTexture`. The `blocks_` (vector of `shared_ptr<Block>`) is also populated with `Block` instances, with basic colors assigned in `openglInitialize` for debugging, though textures are primary.

---
### `headers/world.h` in `headers/`
The `World` class has been refactored to manage `Planet` objects instead of a flat grid of `Chunk`s.

*   **Member Changes:**
    *   `chunks` (std::unordered_map): Removed.
    *   `planets_` (std::vector<std::shared_ptr<Planet>>): New member to store all planets in the world.
    *   `renderDistance`, `chunkSize`: Removed as they were specific to the flat chunk grid.
    *   `worldName_` (std::string): Name for the world, used in data paths.
    *   `defaultSeed_` (int): A default seed for planets if not specified.
*   **New Methods:**
    *   `World(const std::string& worldName = "MyWorld", int defaultSeed = 12345)` (Constructor): Initializes with a world name and seed. Creates world-specific data directories. Starts the worker thread.
    *   `addPlanet(const glm::vec3& position, float radius, int seed, const std::string& name)`: Creates a new `Planet` and adds it to the `planets_` vector.
*   **Modified Methods:**
    *   `~World()`: Manages cleanup of planets and the worker thread.
    *   `update(const Camera& camera)`: Iterates through `planets_` and calls `planet->update(camera, this)`. `this` (world context) is passed so planets can queue tasks to the world's worker thread. Also calls `processMainThreadTasks()`.
    *   `render(projection, view, camera, wireframeState)`: Iterates through `planets_` and calls `planet->render(...)`.
    *   `getBlockAtWorldPos(const glm::vec3& worldPos) const`: Iterates through `planets_`. For each planet, it performs a rough bounding sphere check. If `worldPos` is potentially within the planet, it calls `planet->getBlockAtWorldPos()`.
    *   `addTaskToWorker(const std::function<void()>& task)`: Renamed from `addTask` for clarity. Adds a task to the background worker thread's queue.
*   The worker thread system (`workerThread_`, `queueMutex_`, `queueCondition_`, `taskQueue_`, `shouldTerminate_`, `workerThreadFunction`, `addMainThreadTask`, `processMainThreadTasks`) is largely preserved. It's now used for tasks queued by planets for their chunks (e.g., `Chunk::ensureInitialized`).
*   `worldToChunkCoords` and `getChunkAt(int, int)` (flat world methods): Removed or would need significant adaptation if a similar concept is needed for planets.

---
### `src/world.cpp` in `src/`
Implementation of the refactored `World` class.

*   **Constructor `World(worldName, defaultSeed)`**: Initializes members, creates world data directories using `std::filesystem::create_directories`, and starts `workerThread_`.
*   **Destructor `~World()`**: Signals worker thread to terminate, joins it, clears `planets_`.
*   **`createWorldDirectories()`**: Uses `std::filesystem::create_directories` to set up `worldDataPath_`.
*   **`addPlanet(position, radius, seed, name)`**: Creates `std::make_shared<Planet>` and adds to `planets_`.
*   **`update(camera)`**: Loops `planets_`, calls `planet->update(camera, this)`. Calls `processMainThreadTasks()`.
*   **`render(projection, view, camera, wireframeState)`**: Loops `planets_`, calls `planet->render(...)`.
*   **`getBlockAtWorldPos(worldPos)`**: Loops `planets_`, performs a bounding sphere check (`glm::length(worldPos - planet->getPosition()) <= planet->getRadius() + safety_margin`), then calls `planet->getBlockAtWorldPos()`.
*   **`workerThreadFunction()`**: Waits for tasks on `queueCondition_`. Executes tasks from `taskQueue_`. Handles `shouldTerminate_`.
*   **`addTaskToWorker(task)`**: Pushes task to `taskQueue_`, notifies condition variable.
*   **`addMainThreadTask(task)`**

### `headers/block.h` in `headers/`
Defines the `Block` class, representing an individual voxel.
*   Key static members: `shaderProgram`, `spritesheetTexture`, `spritesheetLoaded`.
*   Key static methods: `InitBlockShader()`, `InitSpritesheet()`, `CleanupBlockShader()`.
*   **New static method: `isTypeSolid(unsigned char blockType)`**: This method is central to determining block solidity for rendering and potentially other game logic. It returns `false` for types defined as non-solid (e.g., Air, Water, Leaves), and `true` otherwise.
*   Instance members include `position`, `color`, and methods for individual block rendering (less used now with chunk meshing).

---
### `src/block.cpp` in `src/`
Implementation of the `Block` class.
*   `Block::isTypeSolid(unsigned char blockType)`: Implemented with a switch statement. Currently, Air (0), Water (5), and Leaves (8) are defined as non-solid. All other types default to solid.