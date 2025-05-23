# AzureVoxel Project Documentation

## File Structure

```
.
├── CMakeLists.txt
├── README.md
├── build/
├── chunk_data/ // Directory for storing saved chunk files
├── cmake_install.cmake
├── create_texture.py
├── documentation_guide.md
├── external/
│   └── stb_image.h
├── headers/
│   ├── block.h
│   ├── camera.h
│   ├── chunk.h
│   ├── crosshair.h  // New: Crosshair class header
│   ├── shader.h
│   ├── texture.h
│   ├── window.h
│   └── world.h
├── main.cpp
├── res/
│   └── textures/
│       ├── grass_block.png
│       ├── grass_block.png Zone.Identifier # Should be deleted if not used
│       └── spritesheet.png // Master spritesheet for block textures
└── shaders/
    ├── crosshair_fragment.glsl // New: Crosshair fragment shader
    ├── crosshair_vertex.glsl   // New: Crosshair vertex shader
    ├── fragment.glsl
    ├── vertex.glsl
    └── vortex.glsl # Typo? Should likely be vertex.glsl or similar.
└── src/
    ├── block.cpp
    ├── camera.cpp
    ├── chunk.cpp
    ├── crosshair.cpp  // New: Crosshair class implementation
    ├── shader.cpp
    ├── texture.cpp
    ├── window.cpp
    └── world.cpp
```

---

## File Details

### `main.cpp` in `./`
This file serves as the main entry point for the AzureVoxel application. It initializes GLFW, GLEW, creates the game window, sets up the camera, world, and a 2D crosshair, and runs the main game loop. It now also initializes a global spritesheet texture and enables mouse capture.

*   **`main()`** - Primary application function.
    *   Initializes GLFW and GLEW.
    *   Creates a `Window` object using an existing `GLFWwindow*`.
    *   Calls `window.enableMouseCapture(true);` to lock and hide the cursor for FPS-style camera control.
    *   Initializes a `Crosshair` object, passing screen dimensions.
    *   Calls `Block::InitBlockShader()` to set up the static shader for blocks.
    *   Calls `Block::InitSpritesheet("res/textures/Spritesheet.PNG")` to load the global texture atlas.
    *   Enables depth testing and face culling.
    *   Creates `Camera` and `World` objects.
    *   Enters the main game loop:
        *   Calculates `deltaTime` and FPS.
        *   Processes keyboard and mouse input for camera movement.
        *   Calculates view and projection matrices (for 3D world and 2D UI/crosshair).
        *   Clears the screen.
        *   Calls `world.update(camera)` to manage chunk loading/unloading.
        *   Calls `world.processMainThreadTasks()` for OpenGL operations queued by worker threads.
        *   Calls `world.render(...)` to draw the 3D scene.
        *   Calls `crosshair->render()` to draw the 2D crosshair on top.
        *   Swaps window buffers and polls events.
    *   After the loop, cleans up `Block::CleanupBlockShader()`, deletes the `crosshair` object, and calls `glfwTerminate()`.
    *   Console output for FPS and camera position updates once per second using `\r` for same-line output.

---

### `create_texture.py` in `./`
This Python script is responsible for programmatically generating a texture for a grass block. It uses the Pillow (PIL) library to create and manipulate image data. (Note: This script is currently not actively used by `main.cpp` for block texture generation as of the latest changes; the system expects a `Spritesheet.PNG` in `res/textures/`.) The generated texture has distinct top (grass), sides (dirt), and bottom (darker dirt) sections, with some added noise for visual variation and an outline. The script saves the generated texture to `res/textures/grass_block.png` and also copies it to `build/res/textures/grass_block.png` if the `build` directory exists.

*   **(script execution)** - The script performs the following steps when run:
    *   Ensures the `res/textures` directory exists, creating it if necessary.
    *   Defines an `image_size` (default 64x64 pixels).
    *   Creates a new RGBA image with the specified size, initially transparent.
    *   Defines colors for grass, dirt, darker dirt, and an outline.
    *   Fills the entire image with the `dirt_brown` color.
    *   Fills the top third of the image with `grass_green`.
    *   Fills the bottom third of the image with `dark_brown`.
    *   Adds random color variations (darker and lighter pixels) to the grass and dirt sections to create a more textured appearance. It uses `random.seed(42)` for reproducible randomness.
    *   Draws an `outline_color` around the edges of the entire image and at the borders between the grass/dirt and dirt/dark_dirt sections.
    *   Saves the final image to `res/textures/grass_block.png`.
    *   If a `build` directory exists, it also ensures `build/res/textures` exists and saves a copy of the texture there as well.
    *   Prints messages to the console indicating the paths where the texture was saved and a final "Done!" message.
    *   **`generateTerrain()`:**
        *   Creates a temporary `representativeBlock`.
        *   Calls `representativeBlock.init()` to compile shaders and `representativeBlock.loadTexture("res/textures/Spritesheet.PNG")` to load the default block texture. This ensures these resources are prepared once per block type.
        *   Iterates through all `x, y, z` coordinates within the chunk dimensions:
            *   Calculates the `blockWorldPos` for the current local coordinates.
            *   Creates a new `std::make_shared<Block>` at this world position.
            *   Calls `block->shareTextureAndShaderFrom(representativeBlock)` to make the new block use the already prepared shader and texture, avoiding redundant work.
            *   Calls `setBlockAtLocal(x, y, z, block)` to add the block to the chunk\'s internal `blocks` array.
        *   Prints a message indicating the chunk at its position has been generated. (Currently, this fills the entire chunk with blocks).

---

### `CMakeLists.txt` in `./`
This file contains instructions for CMake, a cross-platform build system generator. It defines how the AzureVoxel project should be built, including specifying the C++ standard, finding necessary libraries (OpenGL, GLEW, GLFW), listing source and header files, creating the executable, setting include directories, and linking libraries. It also handles copying shader and resource files (including `Spritesheet.PNG`, and now `crosshair_vertex.glsl`, `crosshair_fragment.glsl`) to the build directory.

*   **`cmake_minimum_required(VERSION 3.10)`** - Specifies the minimum required version of CMake.
*   **`project(AzureVoxel)`** - Sets the name of the project to "AzureVoxel".
*   **`set(CMAKE_CXX_STANDARD 17)`** - Sets the C++ standard to C++17.
*   **`set(CMAKE_CXX_STANDARD_REQUIRED ON)`** - Enforces the C++17 standard.
*   **`find_package(OpenGL REQUIRED)`** - Finds the OpenGL library; it's a required dependency.
*   **`find_package(GLEW REQUIRED)`** - Finds the GLEW (OpenGL Extension Wrangler Library); it's a required dependency.
*   **`find_package(glfw3 3.3 REQUIRED)`** - Finds the GLFW library (version 3.3 or higher); it's a required dependency for windowing and input handling.
*   **`set(SOURCES ...)`** - Defines a variable `SOURCES` containing a list of all .cpp source files used in the project.
    *   `main.cpp`
    *   `src/window.cpp`
    *   `src/block.cpp`
    *   `src/texture.cpp`
    *   `src/camera.cpp`
    *   `src/shader.cpp`
    *   `src/chunk.cpp`
    *   `src/world.cpp`
    *   `src/crosshair.cpp` // New: Crosshair implementation file
*   **`set(HEADERS ...)`** - Defines a variable `HEADERS` containing a list of all .h header files used in the project.
    *   `headers/window.h`
    *   `headers/block.h`
    *   `headers/texture.h`
    *   `headers/camera.h`
    *   `headers/shader.h`
    *   `headers/chunk.h`
    *   `headers/world.h`
    *   `headers/crosshair.h` // New: Crosshair header file
*   **`add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})`** - Creates an executable named "AzureVoxel" from the specified source and header files.
*   **`target_include_directories(${PROJECT_NAME} PRIVATE ...)`** - Specifies the include directories for the "AzureVoxel" target.
    *   `${CMAKE_SOURCE_DIR}` (the root directory of the project)
    *   `${OPENGL_INCLUDE_DIR}` (include directory for OpenGL)
    *   `${GLEW_INCLUDE_DIRS}` (include directories for GLEW)
*   **`target_link_libraries(${PROJECT_NAME} PRIVATE ...)`** - Links the "AzureVoxel" target with necessary libraries.
    *   `${OPENGL_LIBRARIES}` (OpenGL libraries)
    *   `GLEW::GLEW` (GLEW library, using its imported target)
    *   `glfw` (GLFW library for windowing and input)
    *   `pthread` (For std::thread compatibility on some platforms)
*   **`file(COPY ... DESTINATION ${CMAKE_BINARY_DIR}/shaders)`** - Copies all shader files (vertex.glsl, fragment.glsl, crosshair_vertex.glsl, crosshair_fragment.glsl) to the `shaders` subdirectory in the build directory.
*   **`file(COPY ${CMAKE_SOURCE_DIR}/res DESTINATION ${CMAKE_BINARY_DIR})`** - Copies the entire `res` (resources) directory from the source directory to the build directory.

---

### `README.md` in `./`
This file provides an overview of the AzureVoxel project. It includes a brief description, a list of features, instructions for getting started (prerequisites, building on Linux and Windows), controls, information about performance optimizations, a high-level project structure, key components, and sections on contributing, licensing, and acknowledgements.

*   **Sections:**
    *   **Overview:** A short introduction to AzureVoxel as a voxel-based game engine.
    *   **Features:** Lists key functionalities like 3D voxel rendering, first-person camera, chunk-based world, wireframe mode, cross-platform support, basic terrain generation, and performance optimization.
    *   **Getting Started:**
        *   **Prerequisites:** Lists necessary tools and libraries (CMake, C++17 compiler, OpenGL 3.3+, GLEW, GLFW).
        *   **Building on Linux:** Provides shell commands for cloning, creating a build directory, configuring with CMake, and building with make.
        *   **Building on Windows:** Provides shell commands for cloning, creating a build directory, and configuring with CMake for Visual Studio.
    *   **Controls:** Explains how to control the camera (W/A/S/D for movement, Mouse for looking) and application (X for wireframe, ESC to exit).
    *   **Performance:** Details the chunk rendering optimization, mentioning configurable render distance and how chunk distance is calculated.
    *   **Project Structure:** Briefly outlines the main directories and files (`main.cpp`, `headers/`, `src/`, `shaders/`, `res/`).
    *   **Key Components:** Lists and briefly describes the main systems of the engine (Window, Camera, Block, Chunk, World, Shader).
    *   **Contributing:** Invites contributions via Pull Requests.
    *   **License:** States the project is licensed under the MIT License.
    *   **Acknowledgements:** Credits the libraries and technologies used (OpenGL, GLFW, GLEW, GLM, stb_image).

---

### `headers/block.h` in `headers/`
Defines the `Block` class, representing a single voxel. It manages block properties and rendering. Now includes static members for a shared shader program and a global spritesheet texture.

*   **`Block`** - Represents a single cube.
    *   **Private Members:**
        *   `VAO, VBO, EBO, texCoordVBO` (GLuint): For individual block rendering (less common).
        *   `position`, `color`, `size` (glm::vec3, float).
        *   `texture` (Texture): For individual block texture instances (e.g., for a template block).
        *   `hasTexture` (bool): If the individual `texture` member is loaded.
    *   **Public Static Members:**
        *   `shaderProgram` (GLuint): ID for the shared GLSL shader program.
        *   `spritesheetTexture` (Texture): Stores the globally loaded texture atlas (`Spritesheet.PNG`).
        *   `spritesheetLoaded` (bool): Flag indicating if `spritesheetTexture` was successfully loaded.
    *   **Public Static Methods:**
        *   `InitBlockShader()`: Compiles and links the static shader program.
        *   `InitSpritesheet(const std::string& path)`: Loads the global spritesheet texture from the given path into `Block::spritesheetTexture`.
        *   `CleanupBlockShader()`: Deletes the static shader program.
    *   **Public Methods:**
        *   Constructor `Block(glm::vec3, glm::vec3, float)`.
        *   Destructor `~Block()`.
        *   `init()`: Sets up VAO/VBO for individual block rendering.
        *   `render(projection, view)`: Renders a single block instance (primarily for debugging or non-chunk rendering).
        *   `loadTexture(...)`: Methods for loading unique textures or sub-regions for an individual `Block` instance's `texture` member.
        *   `shareTextureAndShaderFrom(const Block& other)`: Copies texture data from another block (shader part is now implicit due to static shader).
        *   Other getters and setters for block properties.
        *   `setShaderUniforms(...)`: Now primarily sets uniforms for individual block rendering; chunk rendering sets its own.

---

### `headers/camera.h` in `headers/`
This header file defines the `Camera` class, which is responsible for managing the viewpoint and perspective in the 3D world. It handles camera position, orientation (using Euler angles: yaw and pitch), and movement in response to keyboard and mouse input. It also provides the view matrix necessary for rendering.

*   **`Camera`** - Manages the virtual camera in the 3D scene, including its position, orientation, and movement.
    *   **Private Members:**
        *   `position` (glm::vec3): The 3D coordinates of the camera in world space.
        *   `front` (glm::vec3): A normalized vector pointing in the direction the camera is looking.
        *   `up` (glm::vec3): A normalized vector pointing upwards relative to the camera\'s orientation.
        *   `right` (glm::vec3): A normalized vector pointing to the right relative to the camera\'s orientation.
        *   `worldUp` (glm::vec3): A fixed vector representing the \'up\' direction in world space (typically (0,1,0)).
        *   `yaw` (float): The rotation around the vertical axis (Y-axis).
        *   `pitch` (float): The rotation around the horizontal axis (X-axis).
        *   `movementSpeed` (float): The speed at which the camera moves in response to keyboard input.
        *   `mouseSensitivity` (float): The sensitivity of camera orientation changes in response to mouse movement.
        *   `fov` (float): The field of view angle of the camera, in degrees.
    *   **Private Methods:**
        *   `updateCameraVectors()` - Recalculates the `front`, `right`, and `up` vectors based on the current `yaw` and `pitch` angles. This method is called internally whenever the camera\'s orientation changes.
    *   **Public Methods:**
        *   `Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f)` - Constructor that initializes the camera with an optional initial position, world up vector, yaw, and pitch. It also sets default values for `movementSpeed`, `mouseSensitivity`, and `fov`.
        *   `getViewMatrix() const` (glm::mat4) - Calculates and returns the view matrix using `glm::lookAt`. The view matrix transforms world coordinates to camera (view) space. It uses the camera\'s `position`, `position + front` (target), and `up` vectors.
        *   `processKeyboard(GLFWwindow* window, float deltaTime)` - Processes keyboard input from the given GLFW window to move the camera. It checks for W, A, S, D, SPACE, and LEFT_SHIFT keys (or equivalents) for forward, backward, left, right, up, and down movements respectively. The movement amount is scaled by `movementSpeed` and `deltaTime` for frame-rate independent movement.
        *   `processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true)` - Processes mouse movement to change the camera\'s orientation. `xOffset` and `yOffset` are the changes in mouse position. These offsets are scaled by `mouseSensitivity` and used to update `yaw` and `pitch`. Pitch is typically constrained to prevent flipping (e.g., between -89 and 89 degrees). After updating yaw and pitch, it calls `updateCameraVectors()`.
        *   `getPosition() const` (glm::vec3) - Returns the current position of the camera.
        *   `getFront() const` (glm::vec3) - Returns the current front vector of the camera.
        *   `getFov() const` (float) - Returns the current field of view of the camera.
        *   `setPosition(const glm::vec3& newPosition)` - Sets the camera\'s position to a new specified position.
        *   `setMovementSpeed(float speed)` - Sets the camera\'s movement speed.
        *   `setMouseSensitivity(float sensitivity)` - Sets the camera\'s mouse sensitivity.
        *   `setFov(float newFov)` - Sets the camera\'s field of view.

---

### `headers/chunk.h` in `headers/`
This header file defines the `Chunk` class, which represents a 3D segment of the game world. It manages a collection of blocks, handles terrain generation, surface mesh creation for rendering, and serialization (saving/loading) of chunk data. It also declares constants for chunk dimensions.

*   **Constants:**
    *   `CHUNK_SIZE_X`, `CHUNK_SIZE_Y`, `CHUNK_SIZE_Z` (constexpr int): Define the dimensions of a chunk in blocks.
*   **`ChunkMesh` (struct)**: A simple structure to hold OpenGL buffer IDs (VAO, VBO, EBO) and the index count for a chunk's renderable mesh.
    *   `VAO, VBO, EBO` (GLuint): OpenGL object IDs.
    *   `indexCount` (GLsizei): Number of indices to draw.
*   **`Chunk` (class)**:
    *   **Private Members:**
        *   `position` (glm::vec3): The world space position of the chunk (usually its minimum corner).
        *   `blocks` (std::vector<std::vector<std::vector<std::shared_ptr<Block>>>>): A 3D vector storing shared pointers to the `Block` objects within the chunk.
        *   `needsRebuild` (bool): Flag indicating if the `surfaceMesh` needs to be reconstructed due to changes in blocks.
        *   `isInitialized_` (bool): Flag indicating whether the `ensureInitialized()` method has been successfully run for this chunk. True if the chunk has been loaded or generated and its mesh built.
        *   `surfaceMesh` (ChunkMesh): Holds the VAO, VBO, EBO, and index count for the chunk's optimized renderable mesh.
        *   `hasBlockAtLocal(int x, int y, int z) const` (bool): Private helper to check for a block at local chunk coordinates.
        *   `buildSurfaceMesh(const World* world)`: Private method to generate the `surfaceMesh` from visible block faces.
        *   `addFace(...)`: (Potentially, if it were still present - this seems to have been removed or inlined in `buildSurfaceMesh` based on previous context, keeping a note here if its direct use was intended for documentation from an older state).
        *   `getChunkFileName() const` (std::string): Private helper to generate a standardized filename for saving/loading this chunk based on its position.
    *   **Public Members:**
        *   `Chunk(const glm::vec3& position)` (Constructor): Initializes the chunk with its world `position` and sets `needsRebuild` to true and `isInitialized_` to false.
        *   `~Chunk()` (Destructor): Cleans up OpenGL resources by calling `cleanupMesh()`.
        *   `init(const World* world)`: Placeholder for initial setup. The primary role of loading/generating terrain and mesh is now handled by `ensureInitialized()`. This method can be used for minimal, non-conditional setup if needed in the future, but is currently kept simple as `ensureInitialized()` is called by the `World` object.
        *   `generateTerrain(int seed)`: Populates the chunk with blocks based on a procedural generation algorithm (currently a simple noise function) using the provided `seed`.
        *   `ensureInitialized(const World* world, int seed, const std::string& worldDataPath)`: The main method to prepare a chunk. It tries to load from file using `worldDataPath`. If not found, it calls `generateTerrain(seed)`, saves the new chunk, and then builds its surface mesh via `buildSurfaceMesh(world)`. Sets `isInitialized_` to true upon completion.
        *   `isInitialized() const` (bool): Returns true if `ensureInitialized()` has completed for this chunk, false otherwise.
        *   `renderSurface(const glm::mat4& projection, const glm::mat4& view, bool wireframeState) const`: Renders the chunk using its `surfaceMesh`.
        *   `renderAllBlocks(const glm::mat4& projection, const glm::mat4& view)`: Renders all blocks in the chunk individually (less efficient, for debugging or specific cases).
        *   `getBlockAtLocal(int x, int y, int z) const` (std::shared_ptr<Block>): Retrieves a block at local chunk coordinates.
        *   `setBlockAtLocal(int x, int y, int z, std::shared_ptr<Block> block)`: Sets a block at local coordinates and marks `needsRebuild` as true.
        *   `removeBlockAtLocal(int x, int y, int z)`: Removes a block at local coordinates and marks `needsRebuild` as true.
        *   `getPosition() const` (glm::vec3): Returns the chunk's world `position`.
        *   `needsMeshRebuild() const` (bool): Returns the `needsRebuild` flag.
        *   `markMeshRebuilt()`: Sets `needsRebuild` to false.
        *   `cleanupMesh()`: Deletes OpenGL buffers (VAO, VBO, EBO) associated with `surfaceMesh`.
        *   `saveToFile(const std::string& directoryPath) const` (bool): Saves the chunk's block data to a file.
        *   `loadFromFile(const std::string& directoryPath, const World* world)` (bool): Loads the chunk's block data from a file.

---

### `headers/shader.h` in `headers/`
This header file defines the `Shader` class, a utility for loading, compiling, linking, and using GLSL (OpenGL Shading Language) shader programs. It encapsulates the process of reading shader source code from files, handling compilation and linking errors, and providing methods to activate the shader program and set uniform variables.

*   **`Shader`** - Manages a GLSL shader program, including its loading, compilation, linking, and usage.
    *   **Private Members:**
        *   `ID` (unsigned int): The OpenGL identifier for the linked shader program.
    *   **Private Methods:**
        *   `checkCompileErrors(unsigned int shader, std::string type)` - A utility function to check for compilation or linking errors. If `type` is "PROGRAM", it checks linking errors for the shader program `ID`. Otherwise, it checks compilation errors for the given `shader` object (vertex or fragment shader). It retrieves and prints error messages if any occur.
    *   **Public Methods:**
        *   `Shader(const std::string& vertexPath, const std::string& fragmentPath)` - Constructor that takes file paths to the vertex and fragment shader source codes. It reads the shader code from these files, compiles them, links them into a shader program, stores the program `ID`, and then deletes the individual shader objects. It uses `checkCompileErrors` to report any issues during compilation or linking.
        *   `~Shader()` - Destructor that deletes the shader program using `glDeleteProgram(ID)`.
        *   `use() const` - Activates this shader program for rendering by calling `glUseProgram(ID)`.
        *   `setBool(const std::string& name, bool value) const` - Sets a boolean uniform variable in the shader program.
        *   `setInt(const std::string& name, int value) const` - Sets an integer uniform variable in the shader program.
        *   `setFloat(const std::string& name, float value) const` - Sets a float uniform variable in the shader program.
        *   `setVec3(const std::string& name, const glm::vec3& value) const` - Sets a 3-component vector (glm::vec3) uniform variable in the shader program.
        *   `setMat4(const std::string& name, const glm::mat4& value) const` - Sets a 4x4 matrix (glm::mat4) uniform variable in the shader program.
        *   `getID() const` (unsigned int) - Returns the OpenGL ID of the shader program.

---

### `headers/texture.h` in `headers/`
This header file defines the `Texture` class, which is responsible for loading image files and managing them as OpenGL textures. It handles texture creation, binding, and unbinding. It also includes support for texture sharing to prevent redundant loading and storage of the same texture.

*   **`Texture`** - Manages an OpenGL texture, including loading from a file and binding/unbinding.
    *   **Private Members:**
        *   `textureID` (unsigned int): The OpenGL identifier for the texture object.
        *   `width` (int): The width of the loaded texture in pixels.
        *   `height` (int): The height of the loaded texture in pixels.
        *   `channels` (int): The number of color channels in the loaded texture (e.g., 3 for RGB, 4 for RGBA).
        *   `isShared` (bool): A flag indicating if this `Texture` object represents a shared texture resource. If true, the destructor will not delete the `textureID` as another object might still be using it.
    *   **Public Methods:**
        *   `Texture()` - Default constructor. Initializes `textureID` to 0, width, height, channels to 0, and `isShared` to false.
        *   `~Texture()` - Destructor. If `textureID` is not 0 and `isShared` is false, it deletes the OpenGL texture object using `glDeleteTextures(1, &textureID)`.
        *   `Texture(const Texture& other)` - Copy constructor. It copies the `textureID`, `width`, `height`, and `channels` from the `other` texture. It sets `isShared` to true for both this new texture and the `other` texture (by marking `other.isShared = true`, though this modification of `other` in a copy constructor is unusual and might be better handled with a reference count or by making `isShared` mutable). This indicates that multiple `Texture` objects now refer to the same underlying OpenGL texture.
        *   `operator=(const Texture& other)` (Texture&) - Copy assignment operator. Similar to the copy constructor, it handles sharing of texture resources. If this texture is different from `other` and this texture is not already shared and has a valid `textureID`, it first deletes its own texture. Then it copies data from `other` and sets `isShared` to true for both.
        *   `loadFromFile(const std::string& filepath)` (bool) - Loads an image from the specified file path and creates an OpenGL texture from it. It uses the `stb_image` library (implicitly, via `texture.cpp`) to load image data. It configures texture parameters like wrapping (GL_REPEAT) and filtering (GL_LINEAR_MIPMAP_LINEAR for minification, GL_LINEAR for magnification). It generates mipmaps for the texture. Returns true on successful loading, false otherwise.
        *   `bind(unsigned int textureUnit = 0) const` - Activates the specified texture unit (`glActiveTexture(GL_TEXTURE0 + textureUnit)`) and binds this texture (`glBindTexture(GL_TEXTURE_2D, textureID)`) to it, making it the active texture for subsequent rendering operations.
        *   `unbind()` (static) - Unbinds any 2D texture from the currently active texture unit by calling `glBindTexture(GL_TEXTURE_2D, 0)`.
        *   `getID() const` (unsigned int) - Returns the OpenGL `textureID` of this texture.
        *   `int getWidth() const` - Returns the texture width.
        *   `int getHeight() const` - Returns the texture height.
        *   `int getChannels() const` - Returns the number of texture channels.

---

### `headers/window.h` in `headers/`
This header file defines the `Window` class, which encapsulates the creation and management of an application window using GLFW. It handles window initialization, event polling (keyboard and mouse input), buffer swapping, and window closing. It also manages mouse cursor behavior and a wireframe rendering mode.

*   **`Window`** - Manages the application window, input events, and basic window operations using GLFW.
    *   **Private Members:**
        *   `window` (GLFWwindow*): A pointer to the GLFW window object.
        *   `width` (int): The width of the window in pixels.
        *   `height` (int): The height of the window in pixels.
        *   `title` (std::string): The title of the window.
        *   `lastX` (double), `lastY` (double): The mouse cursor\'s position in the previous frame. Used to calculate movement offsets.
        *   `xOffset` (double), `yOffset` (double): The difference in mouse cursor position between the current and previous frames.
        *   `firstMouse` (bool): A flag to handle the initial mouse input gracefully, preventing large initial jumps in camera orientation.
        *   `wireframeMode` (bool): A flag indicating whether wireframe rendering mode is active.
        *   `currentWindow` (static Window*): A static pointer to the current `Window` instance. This is used to allow static GLFW callback functions to access the non-static members of the `Window` object associated with a particular GLFW window.
    *   **Private Static Methods (Callbacks):**
        *   `framebufferSizeCallback(GLFWwindow* window, int width, int height)` - A static GLFW callback function that is called when the window\'s framebuffer is resized. It updates the OpenGL viewport using `glViewport`.
        *   `keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)` - A static GLFW callback function for keyboard input. It handles global key events like closing the window (ESC key) or toggling wireframe mode (X key). It accesses the `Window` instance via `currentWindow`.
        *   `mouseCallback(GLFWwindow* window, double xpos, double ypos)` - A static GLFW callback function for mouse movement. It calculates `xOffset` and `yOffset` based on the current and last mouse positions. It updates `lastX` and `lastY`. It accesses the `Window` instance via `currentWindow`.
    *   **Public Methods:**
        *   `Window(int width, int height, const std::string& title)` - Constructor. Initializes GLFW, creates a GLFW window with the specified width, height, and title. Sets OpenGL context hints (e.g., version). Makes the window\'s context current. Initializes GLEW. Sets up GLFW callback functions (`framebufferSizeCallback`, `keyCallback`, `mouseCallback`). Initializes mouse position variables and sets `wireframeMode` to false. It also sets `currentWindow = this;`.
        *   `~Window()` - Destructor. Terminates GLFW using `glfwTerminate()`.
        *   `shouldClose() const` (bool) - Returns true if the window has been signaled to close (e.g., by the user clicking the close button or pressing ESC), false otherwise. It checks `glfwWindowShouldClose(window)`.
        *   `swapBuffers()` - Swaps the front and back buffers of the window using `glfwSwapBuffers(window)`, displaying the rendered frame.
        *   `pollEvents()` - Polls for and processes pending events (like keyboard presses, mouse movements, window events) using `glfwPollEvents()`.
        *   `getWindow() const` (GLFWwindow*) - Returns the raw pointer to the GLFW window object.
        *   `enableMouseCapture(bool enable)` - Enables or disables mouse cursor capture. If enabled, the cursor is hidden and locked to the window (`GLFW_CURSOR_DISABLED`), allowing for continuous mouse movement for camera control. If disabled, the cursor is made visible and behaves normally (`GLFW_CURSOR_NORMAL`). This is now actively used.
        *   `getMouseOffset(double& x, double& y)` - Retrieves the calculated mouse movement offsets (`xOffset`, `yOffset`) since the last call.
        *   `resetMouseOffset()` - Resets `xOffset` and `yOffset` to 0.0, typically called after processing mouse input for a frame.
        *   `isKeyPressed(int key) const` (bool) - Checks if a specific keyboard key is currently pressed. It uses `glfwGetKey(window, key) == GLFW_PRESS`.
        *   `isWireframeMode() const` (bool) - Returns the current state of `wireframeMode`.
        *   `toggleWireframeMode()` - Toggles the `wireframeMode` flag. If wireframe mode is enabled, it sets `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)`. If disabled, it sets `glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)`.
        *   `Window(const Window&) = delete;` - Deleted copy constructor to prevent copying of `Window` objects.
        *   `Window& operator=(const Window&) = delete;` - Deleted copy assignment operator to prevent assignment of `Window` objects.

---

### `headers/world.h` in `headers/`
This header file defines the `World` class, which manages the overall game environment, including all chunks. It handles dynamic loading and unloading of chunks based on camera position and render distance, and provides access to blocks at world coordinates. It uses a background worker thread to handle chunk generation to avoid freezing the game.

*   **`IVec2Hash` (struct)**: A custom hash function for `glm::ivec2` keys, enabling their use in `std::unordered_map`. It combines the hashes of the x and y components.
*   **`IVec2Compare` (struct)**: A custom comparison function for `glm::ivec2` keys, enabling their use in `std::set`. It compares x components first, then y components.
*   **`World` (class)**:
    *   **Private Members:**
        *   `chunks` (std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>, IVec2Hash>): An unordered map storing shared pointers to `Chunk` objects, keyed by their 2D chunk coordinates (x, z).
        *   `renderDistance` (int): The number of chunks to load and render in each direction (x and z) from the player's current chunk. E.g., a value of 3 means a (2*3+1)x(2*3+1) = 7x7 grid.
        *   `chunkSize` (const int): The size of each chunk in one dimension (e.g., 16 blocks). This must match the `CHUNK_SIZE_X/Z` constants in `chunk.h`.
        *   `worldSeed_` (int): The seed used for procedural world generation, ensuring reproducible terrain if other parameters are the same.
        *   `worldDataPath_` (std::string): The directory path where chunk files are saved and from where they are loaded.
        *   `workerThread_` (std::thread): A background thread that processes chunk generation and loading tasks.
        *   `queueMutex_` (std::mutex): A mutex to protect the task queue from concurrent access.
        *   `queueCondition_` (std::condition_variable): A condition variable for worker thread synchronization.
        *   `taskQueue_` (std::queue<std::function<void()>>): A queue of tasks to be executed by the worker thread.
        *   `shouldTerminate_` (std::atomic<bool>): Flag to indicate when the worker thread should terminate.
        *   `workerThreadFunction()`: The function executed by the worker thread to process generation tasks.
        *   `addTask(const std::function<void()>& task)`: Adds a task to the queue for background processing.
        *   `saveAllChunks() const`: A helper method to save all loaded chunks to disk.
    *   **Public Members:**
        *   `World(int renderDistance = 3)` (Constructor): Initializes the world with a given `renderDistance`, creates directories, and starts the worker thread.
        *   `~World()` (Destructor): Signals the worker thread to terminate, joins it, and saves all chunk data before destruction.
        *   `update(const Camera& camera)`: This method is called every frame to manage the active chunks.
            *   It determines the player's current chunk coordinates.
            *   It identifies a square region of chunks around the player based on `renderDistance`.
            *   It sorts chunks by distance to the player for prioritized loading.
            *   For each necessary chunk not already loaded, it creates a placeholder and queues background initialization.
            *   It unloads chunks that are outside the active region by saving them and removing them from the map.
        *   `render(const glm::mat4& projection, const glm::mat4& view, const Camera& camera, bool wireframeState)`: Renders all currently loaded and initialized chunks.
            *   It skips chunks that are still being initialized by the worker thread.
            *   It distinguishes between the chunk the camera is currently in (using `renderAllBlocks`) and others (using `renderSurface`).
        *   `getChunkAt(int chunkX, int chunkZ)` (std::shared_ptr<Chunk>): Retrieves an initialized chunk by its chunk coordinates.
        *   `addChunk(int chunkX, int chunkZ)`: Adds a new chunk at the specified coordinates and queues it for background initialization.
        *   `worldToChunkCoords(const glm::vec3& worldPos) const` (glm::ivec2): Converts a 3D world position into 2D chunk coordinates.
        *   `getBlockAtWorldPos(int worldX, int worldY, int worldZ) const` (std::shared_ptr<Block>): Retrieves a block at specific world coordinates.

---

### `src/block.cpp` in `src/`
This file provides the implementation for the `Block` class. It defines the static `shaderProgram` and implements the static `InitBlockShader` and `CleanupBlockShader` methods. The individual `Block::init` method now only sets up VAOs/VBOs for direct block rendering and relies on the static shader.

*   **Global Constants (Shader Source Code):** `vertexShaderSource`, `fragmentShaderSource` (As previously documented)
*   **Static Member Definition:**
    *   `GLuint Block::shaderProgram = 0;`
*   **Static Helper Function (within .cpp):**
    *   `checkShaderCompileErrors(unsigned int shader_id, std::string type)`: Local static helper to check compile/link status and print errors. Used by `InitBlockShader`.
*   **`Block::InitBlockShader()`:**
    *   If `Block::shaderProgram` is not 0, returns (already initialized).
    *   Creates vertex shader, sets source, compiles. Calls `checkShaderCompileErrors`.
    *   Creates fragment shader, sets source, compiles. Calls `checkShaderCompileErrors`.
    *   Creates shader program, attaches shaders, links program. Calls `checkShaderCompileErrors`.
    *   If linking successful, sets `Block::shaderProgram` to the new program ID. Otherwise, cleans up created shaders/program.
    *   Deletes individual vertex and fragment shaders after successful linking.
    *   Prints a success message with the shader program ID.
*   **`Block::CleanupBlockShader()`:**
    *   If `Block::shaderProgram` is not 0, calls `glDeleteProgram` and sets `shaderProgram` to 0. Prints a cleanup message.
*   **`Block`** - (Implementation of other methods)
    *   **Constructor `Block(...)`**: (As previously documented)
    *   **Destructor `~Block()`**: (As previously documented)
    *   **`init()`**: 
        *   No longer compiles shaders.
        *   Checks if `Block::shaderProgram` is 0 and prints an error if so (indicates `InitBlockShader` was missed or failed).
        *   Proceeds with VAO/VBO/EBO/texCoordVBO setup for individual block rendering as before.
    *   **`render(const glm::mat4& projection, const glm::mat4& view)`**: Uses `Block::shaderProgram`.
    *   **`shareTextureAndShaderFrom(const Block& other)`**: No longer copies `shaderProgram` from `other`.
    *   **`useBlockShader() const`**: Uses `Block::shaderProgram`.
    *   **`setShaderUniforms(...) const`**: Uses `Block::shaderProgram`.
    *   The non-static `compileShader` and `checkCompileErrors` member functions have been removed (their logic is now in `InitBlockShader` and the static helper `checkShaderCompileErrors`).

---

### `src/camera.cpp` in `src/`
This file implements the `Camera` class methods declared in `headers/camera.h`. It provides the logic for camera initialization, view matrix calculation, processing keyboard and mouse input for movement and orientation changes, and updating the camera\'s internal orientation vectors.

*   **`Camera`** - (Implementation of methods)
    *   **Constructor `Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)`:**
        *   Initializes `position`, `worldUp`, `yaw`, and `pitch` with the provided arguments.
        *   Sets default values for `front` (initially looking along -Z), `movementSpeed` (to 2.5f), `mouseSensitivity` (to 0.05f), and `fov` (to 45.0f).
        *   Calls `updateCameraVectors()` to compute the initial `front`, `right`, and `up` vectors based on the starting `yaw` and `pitch`.
    *   **`getViewMatrix() const` (glm::mat4):**
        *   Returns the view matrix calculated using `glm::lookAt(position, position + front, up)`. This matrix transforms coordinates from world space to view (camera) space.
    *   **`processKeyboard(GLFWwindow* window, float deltaTime)`:**
        *   Calculates `velocity = movementSpeed * deltaTime` for frame-rate independent movement.
        *   Checks for key presses using `glfwGetKey()`:
            *   `GLFW_KEY_W`: Moves `position` forward along the `front` vector.
            *   `GLFW_KEY_S`: Moves `position` backward along the `front` vector.
            *   `GLFW_KEY_A`: Moves `position` left along the `right` vector (negative `right`).
            *   `GLFW_KEY_D`: Moves `position` right along the `right` vector.
            *   `GLFW_KEY_SPACE`: Moves `position` up along the `up` vector (camera\'s local up).
            *   `GLFW_KEY_LEFT_CONTROL`: Moves `position` down along the `up` vector.
    *   **`processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true)`:**
        *   Scales `xOffset` and `yOffset` by `mouseSensitivity`.
        *   Updates `yaw` by adding `xOffset`.
        *   Updates `pitch` by adding `yOffset`. (The comment notes that the `Window` class might already handle Y-axis inversion for mouse input, so `yOffset` is added directly).
        *   If `constrainPitch` is true:
            *   Clamps `pitch` to be between -89.0f and 89.0f to prevent the camera from flipping over.
        *   Calls `updateCameraVectors()` to recompute orientation vectors based on the new `yaw` and `pitch`.
    *   **`updateCameraVectors()`:**
        *   Calculates a new `front` vector using spherical coordinates based on `yaw` and `pitch`:
            *   `newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch))`
            *   `newFront.y = sin(glm::radians(pitch))`
            *   `newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch))`
        *   Normalizes `newFront` and assigns it to the `front` member.
        *   Recalculates the `right` vector as the cross product of `front` and `worldUp`, then normalizes it.
        *   Recalculates the `up` vector as the cross product of `right` and `front`, then normalizes it. This ensures the `front`, `right`, and `up` vectors form an orthonormal basis.
    *   **`getPosition() const` (glm::vec3):**
        *   Returns the current `position` of the camera.
    *   **`getFront() const` (glm::vec3):**
        *   Returns the current `front` vector of the camera.
    *   **`getFov() const` (float):**
        *   Returns the current `fov` (field of view) of the camera.
    *   **`setPosition(const glm::vec3& newPosition)`:**
        *   Sets the camera\'s `position` to `newPosition`.
    *   **`setMovementSpeed(float speed)`:**
        *   Sets the camera\'s `movementSpeed` to `speed`.
    *   **`setMouseSensitivity(float sensitivity)`:**
        *   Sets the camera\'s `mouseSensitivity` to `sensitivity`.
    *   **`setFov(float newFov)`:**
        *   Sets the camera\'s `fov` to `newFov`.

---

### `src/chunk.cpp` in `src/`
This file implements the `Chunk` class, which represents a segment of the game world composed of many blocks. It handles terrain generation within the chunk, mesh building for efficient rendering of visible block faces, and saving/loading chunk data to/from files.

*   **Global Constants:** `faceVertices`, `texCoords` (As previously documented)
*   **`Chunk(const glm::vec3& position)` (Constructor)**: (As previously documented)
*   **`~Chunk()` (Destructor)**: (As previously documented)
*   **`init(const World* world)`**: (As previously documented - role reduced)
*   **`simpleNoise(...)` (float)**: (As previously documented)
*   **`generateTerrain(int seed)`**: (As previously documented - uses representative blocks, pre-calculates heightmap)
*   **`buildSurfaceMesh(const World* world)`**: Builds an optimized mesh for visible block faces.
    *   (As previously documented regarding reserving memory, iteration order, neighbor checking)
    *   Vertex data added to `meshVertices` includes 3 floats for position and 2 floats for texture coordinates (total 5 floats per vertex).
    *   **Texture Atlas UV Calculation:** For each visible face, the `blockType` (from `blockDataForInitialization_`) is used to determine `uvPixelOffsetX` and `uvPixelOffsetY`. These offsets represent the top-left pixel coordinates of the desired 80x80 texture within the global `Block::spritesheetTexture` (previously assumed 16x16).
        *   For example, `blockType == 1` (Stone) might use `uvPixelOffsetX = 0.0f, uvPixelOffsetY = 0.0f`.
        *   `blockType == 2` (Grass) might use `uvPixelOffsetX = 80.0f, uvPixelOffsetY = 0.0f` (updated from 16.0f).
        *   Users can define new block types and their corresponding pixel offsets in this section to sample different parts of the `Spritesheet.PNG`.
    *   The final UV coordinates (`actualU`, `actualV`) for each vertex of a face are then calculated by:
        *   `actualU = (uvPixelOffsetX + texCoords[i].x * 80.0f) / Block::spritesheetTexture.getWidth();`
        *   `actualV = (uvPixelOffsetY + texCoords[i].y * 80.0f) / Block::spritesheetTexture.getHeight();`
        *   Where `texCoords[i]` provides the 0.0 to 1.0 UVs for a standard quad, and `80.0f` assumes each sub-texture in the atlas is 80x80 pixels (updated from 16.0f).
    *   If `meshVertices` is empty after checking all blocks, returns early.
    *   Creates OpenGL buffers (`surfaceMesh.VAO`, `VBO`, `EBO`).
    *   Configures vertex attribute pointers for `surfaceMesh.VAO`:
        *   Position: `layout (location = 0)`, 3 floats, stride `5 * sizeof(float)`, offset 0.
        *   Texture Coords: `layout (location =1)`, 2 floats, stride `5 * sizeof(float)`, offset `3 * sizeof(float)`.
    *   Sets `surfaceMesh.indexCount` and `needsRebuild = false`.
*   **`renderSurface(const glm::mat4& projection, const glm::mat4& view, bool wireframeState) const`:**
    *   Accepts `wireframeState`.
    *   If `wireframeState` is true:
        *   Sets `useTexture` uniform to `0` (false).
        *   Sets `blockColor` uniform to a calculated color (e.g., blue-ish, varying by chunk position) for wireframe rendering.
    *   If `wireframeState` is false:
        *   Proceeds with normal texture binding using `Block::spritesheetTexture`.
        *   Uses a fallback solid color if the spritesheet isn't loaded.
*   **`renderAllBlocks(...)`**: (As previously documented)
*   **`hasBlockAtLocal(...)` (bool)**: (As previously documented)
*   **`getBlockAtLocal(...)` (std::shared_ptr<Block>)**: (As previously documented)
*   **`setBlockAtLocal(...)`**: (As previously documented)
*   **`removeBlockAtLocal(...)`**: (As previously documented)
*   **`cleanupMesh()`**: (As previously documented - also clears and swaps `meshVertices` and `meshIndices` vectors)
*   **`getPosition() const` (glm::vec3)**: (As previously documented)
*   **`getChunkFileName() const` (std::string)**: (As previously documented)
*   **`saveToFile(...)` (bool)**: (As previously documented - optimized buffer I/O)
*   **`loadFromFile(...)` (bool)**: (As previously documented - optimized buffer I/O, uses representative blocks for textures)
*   **`ensureInitialized(...)`**: (As previously documented - includes timing, reduced console output)
*   **`isInitialized() const` (bool)**: (As previously documented)

---

### `src/shader.cpp` in `src/`
This file provides the implementation for the `Shader` class, which encapsulates GLSL shader program management. It handles loading shader source code from files, compiling vertex and fragment shaders, linking them into a shader program, and providing utility functions for using the program and setting its uniform variables.

*   **`Shader`** - (Implementation of methods)
    *   **Constructor `Shader(const std::string& vertexPath, const std::string& fragmentPath)`:**
        *   Declares `std::string vertexCode`, `fragmentCode` and `std::ifstream vShaderFile`, `fShaderFile`.
        *   Sets `ifstream` objects to throw exceptions on failure (`failbit | badbit`).
        *   **Try-Catch block for file reading:**
            *   Opens the vertex shader file (`vertexPath`) and fragment shader file (`fragmentPath`).
            *   Creates `std::stringstream vShaderStream`, `fShaderStream`.
            *   Reads the file buffer contents into these stringstreams (`vShaderFile.rdbuf()`, `fShaderFile.rdbuf()`).
            *   Closes the file handlers (`vShaderFile.close()`, `fShaderFile.close()`).
            *   Converts the stringstreams to strings: `vertexCode = vShaderStream.str()`, `fragmentCode = fShaderStream.str()`.
        *   **Catch `std::ifstream::failure`:**
            *   Prints an error message to `std::cerr` indicating the shader file could not be read successfully, including `e.what()`.
        *   Gets C-style string pointers: `const char* vShaderCode = vertexCode.c_str()`, `const char* fShaderCode = fragmentCode.c_str()`.
        *   **Compile Shaders:**
            *   **Vertex Shader:**
                *   `vertex = glCreateShader(GL_VERTEX_SHADER)`
                *   `glShaderSource(vertex, 1, &vShaderCode, NULL)`
                *   `glCompileShader(vertex)`
                *   `checkCompileErrors(vertex, "VERTEX")`
            *   **Fragment Shader:**
                *   `fragment = glCreateShader(GL_FRAGMENT_SHADER)`
                *   `glShaderSource(fragment, 1, &fShaderCode, NULL)`
                *   `glCompileShader(fragment)`
                *   `checkCompileErrors(fragment, "FRAGMENT")`
        *   **Shader Program:**
            *   `ID = glCreateProgram()` (ID is the member variable storing the program ID)
            *   `glAttachShader(ID, vertex)`
            *   `glAttachShader(ID, fragment)`
            *   `glLinkProgram(ID)`
            *   `checkCompileErrors(ID, "PROGRAM")`
        *   Deletes the individual vertex and fragment shaders as they are now linked into the program: `glDeleteShader(vertex)`, `glDeleteShader(fragment)`.
    *   **Destructor `~Shader()`:**
        *   Deletes the shader program using `glDeleteProgram(ID)`.
    *   **`use() const`:**
        *   Activates this shader program for rendering by calling `glUseProgram(ID)`.
    *   **`setBool(const std::string& name, bool value) const`:**
        *   Sets a boolean uniform variable. `glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value)`.
    *   **`setInt(const std::string& name, int value) const`:**
        *   Sets an integer uniform variable. `glUniform1i(glGetUniformLocation(ID, name.c_str()), value)`.
    *   **`setFloat(const std::string& name, float value) const`:**
        *   Sets a float uniform variable. `glUniform1f(glGetUniformLocation(ID, name.c_str()), value)`.
    *   **`setVec3(const std::string& name, const glm::vec3& value) const`:**
        *   Sets a 3-component vector (glm::vec3) uniform variable in the shader program.
    *   **`setMat4(const std::string& name, const glm::mat4& value) const`:**
        *   Sets a 4x4 matrix (glm::mat4) uniform variable in the shader program.
    *   **`checkCompileErrors(unsigned int shader, std::string type)`:**
        *   Declares `int success` and `char infoLog[1024]`.
        *   **If `type` is not "PROGRAM" (i.e., "VERTEX" or "FRAGMENT"):**
            *   `glGetShaderiv(shader, GL_COMPILE_STATUS, &success)`
            *   If `!success`:
                *   `glGetShaderInfoLog(shader, 1024, NULL, infoLog)`
                *   Prints an error message to `std::cerr` including `type` and `infoLog`.
        *   **Else (if `type` is "PROGRAM"):**
            *   `glGetProgramiv(shader, GL_LINK_STATUS, &success)` (using `shader` which is `ID` in this case)
            *   If `!success`:
                *   `glGetProgramInfoLog(shader, 1024, NULL, infoLog)`
                *   Prints an error message to `std::cerr` including `type` (PROGRAM) and `infoLog`.

---

### `src/texture.cpp` in `src/`
Implementation of the `Texture` class. Uses `stb_image.h` to load image data from files and creates OpenGL textures. Supports loading sub-regions from spritesheets.

*   Includes `stb_image.h` (with `STB_IMAGE_IMPLEMENTATION` defined).
*   **`Texture::Texture()`**: Default constructor.
*   **`Texture::~Texture()`**: Destructor, deletes texture if not shared.
*   **`Texture::Texture(const Texture& other)` and `Texture& Texture::operator=(const Texture& other)`**: Copy constructor and assignment operator for managing shared texture resources.
*   **`Texture::loadFromFile(const std::string& filepath)`**: Loads an image from the specified file path and creates an OpenGL texture from it. It uses the `stb_image` library (implicitly, via `texture.cpp`) to load image data. It configures texture parameters like wrapping (GL_REPEAT) and filtering (GL_LINEAR_MIPMAP_LINEAR for minification, GL_LINEAR for magnification). It generates mipmaps for the texture. Returns true on successful loading, false otherwise.
*   **`Texture::bind(unsigned int textureUnit = 0) const`**: Activates the specified texture unit (`glActiveTexture(GL_TEXTURE0 + textureUnit)`) and binds this texture (`glBindTexture(GL_TEXTURE_2D, textureID)`) to it, making it the active texture for subsequent rendering operations.
*   **`Texture::unbind()`**: Binds texture 0.

---

### `src/window.cpp` in `src/`
This file implements the `Window` class methods, providing the functionality for creating and managing an application window using GLFW. It handles GLFW initialization, window creation, setting up OpenGL context hints, registering GLFW callbacks for input and window events, and managing window state like wireframe mode.

*   **Static Member Initialization:**
    *   `Window* Window::currentWindow = nullptr;` - Initializes the static pointer used by GLFW callbacks to access the correct `Window` instance.
*   **`Window`** - (Implementation of methods)
    *   **Constructor `Window(int width, int height, const std::string& title)`:**
        *   Initializes member variables: `width`, `height`, `title`.
        *   Initializes `window` pointer to `nullptr`.
        *   Initializes mouse state variables: `lastX` to `width / 2.0`, `lastY` to `height / 2.0`, `xOffset` and `yOffset` to 0.0, `firstMouse` to `true`.
        *   Initializes `wireframeMode` to `false`.
        *   **Initialize GLFW:**
            *   Calls `glfwInit()`. If it fails, prints an error to `std::cerr` and returns.
        *   **Configure GLFW:**
            *   `glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);`
            *   `glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);`
            *   `glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);` (Specifies OpenGL 3.3 Core Profile)
        *   **Create Window:**
            *   `window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);`
            *   If window creation fails (`!window`), prints an error, calls `glfwTerminate()`, and returns.
        *   `glfwMakeContextCurrent(window)`: Makes the created window\'s OpenGL context the current one on the calling thread.
        *   **Set Callbacks:**
            *   `currentWindow = this;`: Assigns the current instance to the static `currentWindow` pointer so static callbacks can access it.
            *   `glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);`
            *   `glfwSetKeyCallback(window, keyCallback);`
            *   `glfwSetCursorPosCallback(window, mouseCallback);`
        *   `glfwSwapInterval(1);`: Enables VSync (synchronizes buffer swaps with the monitor\'s refresh rate).
    *   **Destructor `~Window()`:**
        *   If `window` is not null, `glfwDestroyWindow(window)`.
        *   `glfwTerminate()`: Cleans up all GLFW resources.
    *   **`shouldClose() const` (bool):**
        *   Returns `glfwWindowShouldClose(window)`.
    *   **`swapBuffers()`:**
        *   Calls `glfwSwapBuffers(window)`.
    *   **`pollEvents()`:**
        *   Calls `glfwPollEvents()`.
    *   **`getWindow() const` (GLFWwindow*):**
        *   Returns the `window` pointer.
    *   **`isKeyPressed(int key) const` (bool):**
        *   Returns `glfwGetKey(window, key) == GLFW_PRESS`.
    *   **`enableMouseCapture(bool enable)`**: Enables or disables mouse cursor capture. If enabled, the cursor is hidden and locked to the window (`GLFW_CURSOR_DISABLED`), allowing for continuous mouse movement for camera control. If disabled, the cursor is made visible and behaves normally (`GLFW_CURSOR_NORMAL`). This is now actively used.
    *   **`getMouseOffset(double& x, double& y)`**: Retrieves the calculated mouse movement offsets (`xOffset`, `yOffset`) since the last call.
    *   **`resetMouseOffset()`**: Resets `xOffset` and `yOffset` to 0.0, typically called after processing mouse input for a frame.
    *   **Static Method `framebufferSizeCallback(GLFWwindow* window, int width, int height)`:**
        *   `glViewport(0, 0, width, height)`: Updates the OpenGL viewport when the window\'s framebuffer size changes.
    *   **Static Method `keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)`:**
        *   If `key == GLFW_KEY_ESCAPE` and `action == GLFW_PRESS`, calls `glfwSetWindowShouldClose(window, true)`.
        *   If `key == GLFW_KEY_X` and `action == GLFW_PRESS` and `currentWindow` is not null, calls `currentWindow->toggleWireframeMode()`.
    *   **Static Method `mouseCallback(GLFWwindow* window, double xpos, double ypos)`:**
        *   Returns if `currentWindow` is null.
        *   If `currentWindow->firstMouse` is true:
            *   Sets `currentWindow->lastX = xpos;`
            *   Sets `currentWindow->lastY = ypos;`
            *   Sets `currentWindow->firstMouse = false;`
        *   Calculates mouse offset:
            *   `currentWindow->xOffset = xpos - currentWindow->lastX;`
            *   `currentWindow->yOffset = currentWindow->lastY - ypos;` (Y-offset is reversed because screen Y-coordinates often increase downwards, while typical 3D camera pitch increases upwards).
        *   Updates last mouse position:
            *   `currentWindow->lastX = xpos;`
            *   `currentWindow->lastY = ypos;`
    *   **`toggleWireframeMode()`:**
        *   Flips the `wireframeMode` boolean.
        *   If `wireframeMode` is now true:
            *   `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);`
            *   Prints "Wireframe mode enabled" to `std::cout`.
        *   Else (wireframeMode is false):
            *   `glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);`
            *   Prints "Wireframe mode disabled" to `std::cout`.

---

### `src/world.cpp` in `src/`
This file implements the `World` class methods. Manages chunks, including loading, unloading, and rendering.
*   **`update(const Camera& camera)`:**
    *   Most `std::cout` messages (player chunk changes, chunk unloads, initialization progress) are commented out to reduce console output.
*   **`render(const glm::mat4& projection, const glm::mat4& view, const Camera& camera, bool wireframeState)`:**
    *   Accepts `wireframeState`.
    *   Passes `wireframeState` to `chunk->renderSurface()`.
    *   Periodic render statistics in console are now commented out.

---

### `src/crosshair.cpp` in `src/`
This file implements the `Crosshair` class.

*   **`Crosshair(int screenWidth, int screenHeight)` (Constructor)**:
    *   Initializes its `Shader` member, loading `shaders/crosshair_vertex.glsl` and `shaders/crosshair_fragment.glsl`.
    *   Stores `screenWidth` and `screenHeight`.
    *   Calculates an orthographic `projection_` matrix: `glm::ortho(0.0f, static_cast<float>(screenWidth_), 0.0f, static_cast<float>(screenHeight_))`.
    *   Calls `setupMesh()`.
*   **`~Crosshair()` (Destructor)**:
    *   Calls `glDeleteVertexArrays(1, &VAO_)` and `glDeleteBuffers(1, &VBO_)`.
*   **`setupMesh()`**: 
    *   Defines `crosshairSize` (e.g., 20.0f) and `thickness` (e.g., 2.0f).
    *   Calculates `centerX` and `centerY` based on `screenWidth_` and `screenHeight_`.
    *   Creates a `std::vector<glm::vec2>` for vertices, defining two quads (a horizontal and a vertical line) centered at `centerX, centerY`.
    *   Generates and binds VAO and VBO.
    *   Buffers the vertex data to the VBO (`GL_STATIC_DRAW`).
    *   Sets up vertex attribute pointer for `layout (location = 0)` (2D position), `sizeof(glm::vec2)` stride.
    *   Unbinds VBO and VAO.
*   **`render()`**: 
    *   Calls `glDisable(GL_DEPTH_TEST)`.
    *   Uses `shader_`.
    *   Sets `projection` uniform on the shader with `projection_`.
    *   Sets `crosshairColor` uniform (e.g., to white `glm::vec3(1.0f, 1.0f, 1.0f)`).
    *   Binds `VAO_`.
    *   Draws the two quads using `glDrawArrays(GL_TRIANGLE_FAN, 0, 4)` and `glDrawArrays(GL_TRIANGLE_FAN, 4, 4)`.
    *   Unbinds `VAO_`.
    *   Calls `glEnable(GL_DEPTH_TEST)`.
*   **`updateScreenSize(int screenWidth, int screenHeight)`**: 
    *   Updates `screenWidth_` and `screenHeight_` members.
    *   Recalculates `projection_` matrix using the new dimensions.
    *   (Note: Current implementation doesn't re-call `setupMesh()`, so crosshair pixel size is fixed. If relative sizing is needed, `setupMesh()` would need to be recalled after deleting old VAO/VBO).

---

### `shaders/crosshair_vertex.glsl` in `shaders/`
A simple vertex shader for 2D rendering.
*   Takes a 2D position `aPos` (layout location 0).
*   Takes a `uniform mat4 projection` (orthographic projection matrix).
*   Outputs `gl_Position = projection * vec4(aPos.x, aPos.y, 0.0, 1.0);`.

---

### `shaders/crosshair_fragment.glsl` in `shaders/`
A simple fragment shader for 2D rendering.
*   Takes a `uniform vec3 crosshairColor`.
*   Outputs `FragColor = vec4(crosshairColor, 1.0);`.

---

### `external/stb_image.h` in `external/`
This is a single-file public domain image loading library for C/C++. It's used in this project (specifically in `src/texture.cpp`) to load image files (like PNGs for textures) from disk into memory so they can be processed and uploaded to the GPU as OpenGL textures.
*(Note: Third-party library header, content not detailed here but widely available)*

### `res/textures/Spritesheet.PNG` in `res/textures/`
This is the primary spritesheet image file intended for block textures. It is loaded by default in `Chunk::generateTerrain`. If this file is a texture atlas, all block faces will currently map the entire atlas. Specific sub-texture selection would require UV coordinate adjustments. If blocks appear white or untextured, ensure this file exists at `res/textures/Spritesheet.PNG` (accessible from the build directory as `build/res/textures/Spritesheet.PNG`), that it is a valid image file, and check the console output for any texture loading error messages.
*(Note: This is an image file, content not displayed as text)*

## Known Issues and Troubleshooting

### OpenGL Context Issues

The project is currently experiencing issues with OpenGL resource creation. Specifically:

1. **Error creating Vertex Array Objects (VAOs)**: 
   - `CRITICAL ERROR: Failed to generate VAO. Error code: 0`
   - This prevents chunk meshes from being created and rendered

2. **Error creating textures**:
   - `ERROR::TEXTURE::CREATION_FAILED: Failed to generate texture (error 0)`
   - This prevents textures from being loaded and applied to blocks

These errors indicate a fundamental problem with the OpenGL context or driver. Here are potential solutions:

#### Potential Solutions:

1. **Check OpenGL Driver Compatibility**:
   - The code is written for OpenGL 3.3 Core Profile
   - Run `glxinfo | grep "OpenGL version"` to check your system's OpenGL version
   - Update your graphics drivers if needed

2. **Use Software Rendering Mode**:
   - Set environment variable: `LIBGL_ALWAYS_SOFTWARE=1 ./AzureVoxel`
   - This forces OpenGL to use software rendering instead of hardware acceleration

3. **Try Compatibility Profile**:
   - The current code attempts to use compatibility profile mode
   - You might need to modify graphics settings on your system

4. **Simplify Rendering Code**:
   - Use simpler rendering techniques (immediate mode instead of VAOs)
   - Reduce the number of concurrent OpenGL operations

5. **Check for Resource Leaks**:
   - Ensure all OpenGL resources are properly cleaned up
   - Check for memory leaks or resource exhaustion

#### Emergency Fallback:

If you need to quickly see blocks rendered despite the OpenGL issues:

1. Modify `Chunk::renderSurface()` to use immediate mode rendering (glBegin/glEnd)
2. Disable texture loading and use solid colors for blocks
3. Reduce render distance to minimize resource usage

## Recent Updates and Optimizations

### Shader System Improvements
- Enhanced error handling in `Block::InitBlockShader()` with proper buffer initialization and comprehensive error checking
- Added explicit OpenGL error state checking with `glGetError()` in shader compilation and initialization
- Properly configured texture coordinate attributes (layout location = 1) in vertex shaders and attribute pointers

### Chunk Rendering System Enhancements
- Increased default render distance from 3 to 5 chunks for better visibility
- Added parallel processing of up to 5 chunks at once in the worker thread system
- Implemented detailed chunk loading progress indicators in the console output
- Added comprehensive OpenGL error checking in the mesh building process
- Optimized chunk initialization to prioritize loading from files over generation
- Added performance logging for slow chunk operations to help identify bottlenecks
- Fixed issues with texture loading in `Chunk::generateTerrain()` by properly declaring the `Block::loadTexture()` method that takes spritesheet parameters

### General Optimizations
- Reduced console output for common operations to improve performance
- Prioritized chunks closest to the player for better visual experience
- Improved error reporting for shader compilation and OpenGL operations
- Enhanced mesh building with better error recovery and resource cleanup

 