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
│   ├── shader.h
│   ├── texture.h
│   ├── window.h
│   └── world.h
├── main.cpp
├── res/
│   └── textures/
│       ├── grass_block.png
│       ├── grass_block.pngZone.Identifier
│       └── spritesheet.png // Master spritesheet for block textures
└── shaders/
    ├── fragment.glsl
    ├── vertex.glsl
    └── vortex.glsl
└── src/
    ├── block.cpp
    ├── camera.cpp
    ├── chunk.cpp
    ├── shader.cpp
    ├── texture.cpp
    ├── window.cpp
    └── world.cpp
```

---

## File Details

### `main.cpp` in `./`
This file serves as the main entry point and orchestrator for the AzureVoxel application. It initializes the windowing system (GLFW), graphics rendering (GLEW), creates the game world, and runs the main game loop. It handles user input for camera movement, updates game state, and renders the scene.

*   **`main()`** - This is the primary function where the application starts.
    *   Initializes a `Window` object with specified dimensions and title.
    *   Initializes GLEW for OpenGL extension loading. If initialization fails, it prints an error and exits.
    *   Prints the current OpenGL version.
    *   Enables depth testing for correct 3D rendering.
    *   Creates a `Camera` object, positioning it for a good view of the terrain and setting mouse sensitivity.
    *   (Note: The call to an external Python script `create_texture.py` to generate textures has been commented out, as textures are now expected to be pre-existing, e.g., `spritesheet.png`.)
    *   Creates a `World` object, defining its render distance (currently 12 chunks) and initializing a grid of chunks.
    *   Enters the main game loop which continues until the window is closed.
        *   Calculates `deltaTime` for frame-rate independent movement and physics.
        *   Calculates and updates Frames Per Second (FPS).
        *   Processes keyboard input via the `camera.processKeyboard()` method.
        *   Processes mouse input for camera orientation using `window.getMouseOffset()` and `camera.processMouseMovement()`.
        *   Creates the view matrix using `camera.getViewMatrix()`.
        *   Creates the projection matrix using `glm::perspective`, defining the camera\'s field of view, aspect ratio, and near/far clipping planes.
        *   Clears the screen with a sky blue color and clears the depth buffer.
        *   Renders the world using `world.render()`, passing the projection, view matrices, and camera object.
        *   Prints the current camera position and FPS to the console.
        *   Swaps the front and back buffers of the window (`window.swapBuffers()`) to display the rendered frame.
        *   Polls for window events like keyboard presses or mouse movements (`window.pollEvents()`).
    *   After the loop ends (window is closed), it prints a newline and returns 0, indicating successful execution.

---

### `create_texture.py` in `./`
This Python script is responsible for programmatically generating a texture for a grass block. It uses the Pillow (PIL) library to create and manipulate image data. (Note: This script is currently not actively used by `main.cpp` for block texture generation as of the latest changes; the system expects a `spritesheet.png` in `res/textures/`.) The generated texture has distinct top (grass), sides (dirt), and bottom (darker dirt) sections, with some added noise for visual variation and an outline. The script saves the generated texture to `res/textures/grass_block.png` and also copies it to `build/res/textures/grass_block.png` if the `build` directory exists.

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
        *   Calls `representativeBlock.init()` to compile shaders and `representativeBlock.loadTexture("res/textures/spritesheet.png")` to load the default block texture. This ensures these resources are prepared once per block type.
        *   Iterates through all `x, y, z` coordinates within the chunk dimensions:
            *   Calculates the `blockWorldPos` for the current local coordinates.
            *   Creates a new `std::make_shared<Block>` at this world position.
            *   Calls `block->shareTextureAndShaderFrom(representativeBlock)` to make the new block use the already prepared shader and texture, avoiding redundant work.
            *   Calls `setBlockAtLocal(x, y, z, block)` to add the block to the chunk\'s internal `blocks` array.
        *   Prints a message indicating the chunk at its position has been generated. (Currently, this fills the entire chunk with blocks).

---

### `CMakeLists.txt` in `./`
This file contains instructions for CMake, a cross-platform build system generator. It defines how the AzureVoxel project should be built, including specifying the C++ standard, finding necessary libraries (OpenGL, GLEW, GLFW), listing source and header files, creating the executable, setting include directories, and linking libraries. It also handles copying shader and resource files to the build directory.

*   **`cmake_minimum_required(VERSION 3.10)`** - Specifies the minimum required version of CMake.
*   **`project(AzureVoxel)`** - Sets the name of the project to "AzureVoxel".
*   **`set(CMAKE_CXX_STANDARD 17)`** - Sets the C++ standard to C++17.
*   **`set(CMAKE_CXX_STANDARD_REQUIRED ON)`** - Enforces the C++17 standard.
*   **`find_package(OpenGL REQUIRED)`** - Finds the OpenGL library; it\'s a required dependency.
*   **`find_package(GLEW REQUIRED)`** - Finds the GLEW (OpenGL Extension Wrangler Library); it\'s a required dependency.
*   **`find_package(glfw3 3.3 REQUIRED)`** - Finds the GLFW library (version 3.3 or higher); it\'s a required dependency for windowing and input handling.
*   **`set(SOURCES ...)`** - Defines a variable `SOURCES` containing a list of all .cpp source files used in the project.
    *   `main.cpp`
    *   `src/window.cpp`
    *   `src/block.cpp`
    *   `src/texture.cpp`
    *   `src/camera.cpp`
    *   `src/shader.cpp`
    *   `src/chunk.cpp`
    *   `src/world.cpp`
*   **`set(HEADERS ...)`** - Defines a variable `HEADERS` containing a list of all .h header files used in the project.
    *   `headers/window.h`
    *   `headers/block.h`
    *   `headers/texture.h`
    *   `headers/camera.h`
    *   `headers/shader.h`
    *   `headers/chunk.h`
    *   `headers/world.h`
*   **`add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})`** - Creates an executable named "AzureVoxel" from the specified source and header files.
*   **`target_include_directories(${PROJECT_NAME} PRIVATE ...)`** - Specifies the include directories for the "AzureVoxel" target.
    *   `${CMAKE_SOURCE_DIR}` (the root directory of the project)
    *   `${OPENGL_INCLUDE_DIR}` (include directory for OpenGL)
    *   `${GLEW_INCLUDE_DIRS}` (include directories for GLEW)
*   **`target_link_libraries(${PROJECT_NAME} PRIVATE ...)`** - Links the "AzureVoxel" target with necessary libraries.
    *   `${OPENGL_LIBRARIES}` (OpenGL libraries)
    *   `GLEW::GLEW` (GLEW library, using its imported target)
    *   `glfw` (GLFW library for windowing and input)
*   **`file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})`** - Copies the entire `shaders` directory from the source directory to the build directory.
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
This header file defines the `Block` class, which represents a single voxel or cube in the 3D world. It manages the block\'s properties such as position, color, size, and texture, as well as its rendering through OpenGL.

*   **`Block`** - Represents a single cube in the game world. It handles its own rendering setup (VAO, VBO, EBO, shaders) and can be textured.
    *   **Private Members:**
        *   `VAO, VBO, EBO, texCoordVBO` (GLuint): OpenGL identifiers for Vertex Array Object, Vertex Buffer Object, Element Buffer Object, and texture coordinate VBO respectively. Used for rendering.
        *   `shaderProgram` (GLuint): Identifier for the compiled and linked GLSL shader program used to render the block.
        *   `position` (glm::vec3): The 3D coordinates of the block\'s center.
        *   `color` (glm::vec3): The base color of the block (used if no texture is applied).
        *   `size` (float): The edge length of the cube.
        *   `texture` (Texture): A `Texture` object associated with this block.
        *   `hasTexture` (bool): Flag indicating whether the block has a texture loaded.
        *   `speed` (float): A variable presumably intended for movement, though not actively used for block movement in typical voxel engines.
    *   **Private Methods:**
        *   `compileShader(const char* vertexShaderSource, const char* fragmentShaderSource)` (unsigned int): A utility function to compile vertex and fragment shaders and link them into a shader program. Returns the shader program ID.
    *   **Public Methods:**
        *   `Block(const glm::vec3& position, const glm::vec3& color, float size)` - Constructor that initializes the block with a given position, color, and size.
        *   `~Block()` - Destructor, responsible for cleaning up OpenGL resources (VAO, VBO, EBO, shaderProgram if owned).
        *   `init()` - Initializes the OpenGL buffers (VAO, VBO, EBO) for the block\'s geometry and texture coordinates. It also compiles and links the shaders if `shaderProgram` is 0.
        *   `render(const glm::mat4& projection, const glm::mat4& view)` - Renders the individual block. It sets up the model matrix based on the block\'s position and size, uses the block\'s shader, binds its texture (if any), sets shader uniforms (projection, view, model matrices), binds the VAO, and draws the cube using `glDrawElements`.
        *   `move(float dx, float dy, float dz)` - Modifies the block\'s position by the given deltas.
        *   `setPosition(const glm::vec3& newPosition)` - Sets the block\'s position to a new specified position.
        *   `loadTexture(const std::string& texturePath)` (bool) - Loads a texture from the given file path using the `Texture` object. Sets `hasTexture` to true on success. Returns true if texture loading was successful, false otherwise.
        *   `shareTextureAndShaderFrom(const Block& other)` - Allows this block to share the texture and shader program from another `Block` object. This is an optimization to avoid redundant texture loading and shader compilation for identical blocks.
        *   `getPosition() const` (glm::vec3) - Returns the current 3D position of the block.
        *   `getColor() const` (glm::vec3) - Returns the base color of the block.
        *   `getShaderProgram() const` (GLuint) - Returns the ID of the shader program used by this block.
        *   `hasTextureState() const` (bool) - Returns true if the block has a texture, false otherwise. (Renamed to avoid conflict with `Texture` class methods).
        *   `getTextureID() const` (GLuint) - Returns the OpenGL ID of the block\'s texture.
        *   `useBlockShader() const` - Activates the block\'s shader program using `glUseProgram`.
        *   `bindBlockTexture() const` - Binds the block\'s texture using `glBindTexture` if `hasTexture` is true.
        *   `setShaderUniforms(const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model) const` - Sets the necessary transformation matrix uniforms (projection, view, model) in the block\'s shader.

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
This header file defines the `Chunk` class and a helper struct `ChunkMesh`. A chunk is a fixed-size 3D segment of the game world, containing multiple blocks. This class is crucial for managing and rendering large voxel worlds efficiently by organizing blocks into manageable units and implementing mesh generation for visible block faces.

*   **Constants:**
    *   `CHUNK_SIZE_X` (int): Defines the width of a chunk (number of blocks along the X-axis), set to 16.
    *   `CHUNK_SIZE_Y` (int): Defines the height of a chunk (number of blocks along the Y-axis), set to 16.
    *   `CHUNK_SIZE_Z` (int): Defines the depth of a chunk (number of blocks along the Z-axis), set to 16.
*   **`ChunkMesh`** - A structure to hold OpenGL buffer objects (VAO, VBO, EBO) and the index count for a chunk\'s renderable mesh. This mesh typically consists only of the visible faces of the blocks within the chunk.
    *   **Members:**
        *   `VAO` (GLuint): Vertex Array Object for the chunk\'s mesh.
        *   `VBO` (GLuint): Vertex Buffer Object for the chunk\'s mesh (stores vertex positions, normals, texture coordinates).
        *   `EBO` (GLuint): Element Buffer Object for the chunk\'s mesh (stores indices for drawing).
        *   `indexCount` (GLsizei): The number of indices to draw for this chunk\'s mesh.
*   **`Chunk`** - Represents a 3D segment of the world containing a grid of blocks. It manages the blocks within its boundaries and is responsible for generating an optimized mesh of their visible faces for rendering.
    *   **Private Members:**
        *   `position` (glm::vec3): The world-space coordinates of the origin (e.g., corner with lowest x,y,z) of this chunk.
        *   `blocks` (std::vector<std::vector<std::vector<std::shared_ptr<Block>>>>): A 3D vector (effectively a 3D array) storing shared pointers to `Block` objects. This structure holds all the blocks within the chunk.
        *   `needsRebuild` (bool): A flag that indicates whether the chunk\'s `surfaceMesh` needs to be regenerated (e.g., after a block is added or removed).
        *   `surfaceMesh` (ChunkMesh): The mesh data for rendering only the visible faces of the chunk.
        *   `hasBlockAtLocal(int x, int y, int z) const` (bool): Checks if a non-air block exists at local chunk coordinates.
        *   `buildSurfaceMesh(const World* world)` - Generates the `surfaceMesh` by iterating through blocks and adding faces that are exposed to air or adjacent to blocks in unloaded/different chunks. Requires a `World` pointer to check neighboring chunks.
        *   `addFace(...)` - Helper method for `buildSurfaceMesh` to add vertex and index data for a single block face to the mesh buffers.
        *   `getChunkFileName() const` (std::string) - Generates a unique filename for the chunk based on its position (e.g., \"chunk_0_0_0.chunk\"). Used internally for saving and loading.
    *   **Private Methods:**
        *   `hasBlockAtLocal(int x, int y, int z) const` (bool): Checks if a block exists (is not null) at the given local coordinates (0 to CHUNK_SIZE-1) within the chunk.
        *   `buildSurfaceMesh(const World* world)` - Generates or regenerates the `surfaceMesh`. This involves iterating through all blocks in the chunk and, for each block, determining which of its faces are exposed (i.e., not adjacent to another solid block within this chunk or in a neighboring chunk). Only these exposed faces are added to the mesh data (vertices, indices, texture coordinates). It uses the `world` pointer to check adjacent blocks in neighboring chunks.
        *   `addFace(const glm::vec3& corner, const glm::vec3& side1, const glm::vec3& side2, std::vector<float>& vertices, std::vector<unsigned int>& indices, const std::vector<float>& faceTexCoords)` - A helper method used by `buildSurfaceMesh` to add the vertex data (positions and texture coordinates) and indices for a single block face to the provided vectors.
    *   **Public Methods:**
        *   `Chunk(const glm::vec3& position)` - Constructor that initializes the chunk with a given world position. It also resizes the `blocks` 3D vector to the chunk dimensions.
        *   `~Chunk()` - Destructor, responsible for calling `cleanupMesh()` to release OpenGL resources.
        *   `init(const World* world)` - Initializes the chunk. This typically involves calling `generateTerrain()` to populate the chunk with blocks and then `buildSurfaceMesh(world)` to create the initial renderable mesh.
        *   `generateTerrain()` - Populates the chunk with blocks. The current implementation in `chunk.cpp` seems to fill the entire chunk with default blocks. In a more complex system, this would involve procedural generation algorithms.
        *   `renderSurface(const glm::mat4& projection, const glm::mat4& view)` - Renders the chunk using its pre-built `surfaceMesh`. It binds the mesh\'s VAO, sets shader uniforms (model, view, projection matrices), binds the appropriate texture (likely a texture atlas for all block types), and draws the mesh using `glDrawElements`. This is the primary method for rendering chunks efficiently.
        *   `renderAllBlocks(const glm::mat4& projection, const glm::mat4& view)` - Renders every block within the chunk individually by calling each block\'s `render()` method. This is less efficient than `renderSurface` and is likely used for debugging or for chunks very close to the camera where individual block interactions might be more detailed.
        *   `getBlockAtLocal(int x, int y, int z) const` (std::shared_ptr<Block>) - Returns a shared pointer to the block at the specified local coordinates within the chunk. Returns `nullptr` if no block exists or coordinates are out of bounds.
        *   `setBlockAtLocal(int x, int y, int z, std::shared_ptr<Block> block)` - Places or replaces a block at the specified local coordinates. It sets `needsRebuild` to true because the chunk\'s mesh will need to be updated.
        *   `removeBlockAtLocal(int x, int y, int z)` - Removes a block (sets the pointer to `nullptr`) at the specified local coordinates. It sets `needsRebuild` to true.
        *   `getPosition() const` (glm::vec3) - Returns the world-space position of the chunk.
        *   `needsMeshRebuild() const` (bool) - Returns the value of the `needsRebuild` flag.
        *   `markMeshRebuilt()` - Sets the `needsRebuild` flag to false, typically called after `buildSurfaceMesh` has completed.
        *   `cleanupMesh()` - Deletes the OpenGL buffer objects (VAO, VBO, EBO) associated with `surfaceMesh` to free up GPU memory.

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
        *   `enableMouseCapture(bool enable)` - Enables or disables mouse cursor capture. If enabled, the cursor is hidden and locked to the window (`GLFW_CURSOR_DISABLED`). If disabled, the cursor is made visible and behaves normally (`GLFW_CURSOR_NORMAL`).
        *   `getMouseOffset(double& x, double& y)` - Retrieves the calculated mouse movement offsets (`xOffset`, `yOffset`) since the last call.
        *   `resetMouseOffset()` - Resets `xOffset` and `yOffset` to 0.0, typically called after processing mouse input for a frame.
        *   `isKeyPressed(int key) const` (bool) - Checks if a specific keyboard key is currently pressed. It uses `glfwGetKey(window, key) == GLFW_PRESS`.
        *   `isWireframeMode() const` (bool) - Returns the current state of `wireframeMode`.
        *   `toggleWireframeMode()` - Toggles the `wireframeMode` flag. If wireframe mode is enabled, it sets `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)`. If disabled, it sets `glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)`.
        *   `Window(const Window&) = delete;` - Deleted copy constructor to prevent copying of `Window` objects.
        *   `Window& operator=(const Window&) = delete;` - Deleted copy assignment operator to prevent assignment of `Window` objects.

---

### `headers/world.h` in `headers/`
This header file defines the `World` class, which manages the overall game world, composed of multiple `Chunk` objects. It handles the storage, retrieval, and rendering of chunks based on the camera\'s position and a specified render distance. It also provides utility functions for converting between world coordinates and chunk coordinates.

*   **`IVec2Hash`** - A custom hash function struct for `glm::ivec2`. This is necessary to use `glm::ivec2` (which represents 2D integer chunk coordinates) as keys in `std::unordered_map`.
    *   `operator()(const glm::ivec2& key) const` (size_t) - Computes a hash value for a `glm::ivec2` key by combining the hash of its x and y components.
*   **`World`** - Manages the collection of all chunks in the game world, handling their creation, loading, and rendering.
    *   **Private Members:**
        *   `chunks` (std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>, IVec2Hash>): Stores shared pointers to `Chunk` objects, mapped by their 2D chunk coordinates (X, Z).
        *   `renderDistance` (int): The radius of chunks (in chunk units) around the player that should be loaded and rendered.
        *   `chunkSize` (const int): The size of each chunk in one dimension (e.g., 16 blocks). Must match `CHUNK_SIZE_X/Z` in `chunk.h`.
    *   **Private Methods:**
        *   `saveAllChunks() const` - Iterates through all loaded chunks and calls their `saveToFile` method to persist their data. This is typically called when the world is being destroyed (e.g., game exit).
    *   **Public Methods:**
        *   `World(int renderDistance = 3)` - Constructor. Initializes `renderDistance` and creates the `chunk_data` directory if it doesn't exist.
        *   `~World()` - Destructor. Clears the `chunks` map, which will lead to the destruction of the managed `Chunk` objects if their shared pointers are no longer referenced elsewhere.
        *   `init(int gridSize = 5)` - Initializes the world by creating an initial grid of chunks. It creates a `gridSize` x `gridSize` square of chunks centered around (0,0). For each chunk position in this grid, it calls `addChunk()`.
        *   `render(const glm::mat4& projection, const glm::mat4& view, const Camera& camera)` - Renders all chunks that are within the `renderDistance` of the camera.
            *   It first determines the chunk coordinates (chunkX, chunkZ) where the camera is currently located using `worldToChunkCoords(camera.getPosition())`.
            *   Then, it iterates in a square area around this central chunk, from `(cameraChunkX - renderDistance)` to `(cameraChunkX + renderDistance)` and `(cameraChunkZ - renderDistance)` to `(cameraChunkZ + renderDistance)`.
            *   For each chunk coordinate pair in this range, it retrieves the corresponding chunk using `getChunkAt()`.
            *   If the chunk exists, it calls the chunk\'s `renderSurface()` method (or potentially `renderAllBlocks()` for the chunk the player is in, as seen in `main.cpp`\'s logic though not explicitly in `world.cpp` shown so far). It ensures chunks have their meshes rebuilt if `needsMeshRebuild()` is true.
        *   `getChunkAt(int chunkX, int chunkZ)` (std::shared_ptr<Chunk>) - Retrieves a shared pointer to the chunk at the specified chunk coordinates (chunkX, chunkZ). Returns `nullptr` if no chunk exists at those coordinates.
        *   `addChunk(int chunkX, int chunkZ)` - Creates a new `Chunk` at the given chunk coordinates (chunkX, chunkZ). It calculates the world position for the new chunk, creates a `std::make_shared<Chunk>` instance, calls `init(this)` on the new chunk (passing `this` world object for context like neighbor lookups during mesh building), and then stores it in the `chunks` map.
        *   `worldToChunkCoords(const glm::vec3& worldPos) const` (glm::ivec2) - Converts a 3D world position (`worldPos`) into 2D chunk coordinates (X, Z). It divides the world X and Z coordinates by `chunkSize` and floors the result to get the integer chunk indices.
        *   `getBlockAtWorldPos(int worldX, int worldY, int worldZ) const` (std::shared_ptr<Block>) - Retrieves a block from its absolute world coordinates. It first converts the world coordinates to chunk coordinates using `worldToChunkCoords`. Then, it gets the corresponding chunk using `getChunkAt()`. If the chunk exists, it calculates the local coordinates of the block within that chunk and calls the chunk\'s `getBlockAtLocal()` method. Returns `nullptr` if the chunk or block doesn\'t exist.
        *   `getBlockAtWorldPos(const glm::vec3& worldPos) const` (std::shared_ptr<Block>) - An overloaded version of `getBlockAtWorldPos` that takes a `glm::vec3` for the world position. It converts the float coordinates to integers by flooring them and then calls the integer-based version.

---

### `src/block.cpp` in `src/`
This file provides the implementation for the `Block` class declared in `headers/block.h`. It includes the definitions for the constructor, destructor, methods for initialization (OpenGL buffers, shaders), rendering, texture loading, movement, and shader management. It also contains the GLSL source code for the vertex and fragment shaders used to render blocks.

*   **Global Constants (Shader Source Code):**
    *   `vertexShaderSource` (const char*): A C-style string containing the GLSL source code for the vertex shader. This shader takes vertex position and texture coordinates as input, transforms the position using model, view, and projection matrices, and passes the texture coordinates to the fragment shader.
    *   `fragmentShaderSource` (const char*): A C-style string containing the GLSL source code for the fragment shader. This shader receives texture coordinates from the vertex shader. If `useTexture` uniform is true, it samples color from `blockTexture`. Otherwise, it uses the `blockColor` uniform.
*   **`Block`** - (Implementation of methods)
    *   **Constructor `Block(const glm::vec3& position, const glm::vec3& color, float size)`:**
        *   Initializes `position`, `color`, and `size` members with the provided arguments. Other members like OpenGL handles and flags are initialized in the header or by default.
    *   **Destructor `~Block()`:**
        *   Cleans up OpenGL resources (VAO, VBO, EBO, texCoordVBO) by calling `glDeleteVertexArrays` and `glDeleteBuffers` if these objects were generated (i.e., their IDs are not 0). It notes that shader programs are potentially shared and are not deleted here to avoid issues; shader lifetime should be managed carefully (e.g., by a central shader manager or through the `Shader` class if blocks use individual `Shader` objects).
    *   **`init()`:**
        *   Compiles and links the shader program using `compileShader()` if `shaderProgram` is 0 (meaning it hasn\'t been compiled yet for this block type or a representative block).
        *   If `VAO` is 0 (not yet created):
            *   Defines vertex data (positions) for a cube (currently only front and back faces are fully defined in the example `vertices` array, this VAO seems intended for individual block rendering rather than chunk meshing).
            *   Defines texture coordinates (also only for front and back faces in the example `texCoords` array).
            *   Defines indices for drawing the cube faces (again, only front and back).
            *   Generates VAO, VBO, EBO using `glGenVertexArrays`, `glGenBuffers`.
            *   Binds the VAO.
            *   Configures the VBO for vertex positions: binds it, transfers data with `glBufferData`, sets vertex attribute pointer for `layout (location = 0)`.
            *   (Note: Texture coordinate VBO (`texCoordVBO`) setup is commented out or simplified; in a full setup, texture coordinates would also be uploaded and configured with a vertex attribute pointer for `layout (location = 1)`.)
            *   Configures the EBO: binds it, transfers index data with `glBufferData`.
            *   Unbinds VBO and VAO.
    *   **`render(const glm::mat4& projection, const glm::mat4& view)`:**
        *   Calls `init()` if the block\'s VAO is 0 to ensure OpenGL resources are set up.
        *   Returns if `VAO` or `shaderProgram` is 0 (indicating initialization might have failed).
        *   Activates the block\'s shader program using `useBlockShader()`.
        *   Binds the block\'s texture (if any) using `bindBlockTexture()`.
        *   Calculates the `model` matrix by translating an identity matrix to the block\'s `position`.
        *   Sets shader uniforms (projection, view, model matrices, and texture/color info) using `setShaderUniforms()`.
        *   Binds the block\'s `VAO`.
        *   Draws the block using `glDrawElements`. The current call `glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0)` suggests it draws 12 indices (4 triangles, likely 2 faces), which matches the simplified vertex data in `init()`.
        *   Unbinds the `VAO` and then calls `Texture::unbind()` to unbind the texture from texture unit 0.
    *   **`loadTexture(const std::string& texturePath)` (bool):**
        *   Calls `texture.loadFromFile(texturePath)` on its member `Texture` object.
        *   If loading is successful, sets `hasTexture` to true and returns true.
        *   Otherwise, sets `hasTexture` to false and returns false.
    *   **`shareTextureAndShaderFrom(const Block& other)`:**
        *   Copies the `Texture` object from `other` (the `Texture` class\'s assignment operator should handle the sharing logic, e.g., by incrementing a reference count or setting `isShared` flags).
        *   Copies the `hasTexture` state from `other.hasTextureState()`.
        *   Copies the `shaderProgram` ID from `other.getShaderProgram()`. This allows multiple blocks of the same type to share a single compiled shader program and loaded texture.
    *   **`move(float dx, float dy, float dz)`:**
        *   Updates the block\'s `position` by adding `dx * speed`, `dy * speed`, and `dz * speed` to its components.
    *   **`setPosition(const glm::vec3& newPosition)`:**
        *   Sets the block\'s `position` to `newPosition`.
    *   **`getPosition() const` (glm::vec3):**
        *   Returns the block\'s current `position`.
    *   **`getColor() const` (glm::vec3):**
        *   Returns the block\'s base `color`.
    *   **`useBlockShader() const`:**
        *   If `shaderProgram` is not 0, it calls `glUseProgram(shaderProgram)` to activate it.
    *   **`bindBlockTexture() const`:**
        *   If `hasTexture` is true, it calls `texture.bind(0)` to bind the block\'s texture to texture unit 0.
    *   **`setShaderUniforms(const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model) const`:**
        *   Returns if `shaderProgram` is 0.
        *   Sets the "model", "view", and "projection" matrix uniforms in the shader program using `glUniformMatrix4fv` and `glGetUniformLocation`.
        *   Sets the "useTexture" boolean uniform.
        *   If `hasTexture` is true, sets the "blockTexture" sampler uniform to 0 (indicating texture unit 0).
        *   If `hasTexture` is false, sets the "blockColor" vec3 uniform to the block\'s `color` member.
    *   **`compileShader(const char* vShaderSource, const char* fShaderSource)` (unsigned int):**
        *   Creates a vertex shader object (`glCreateShader(GL_VERTEX_SHADER)`).
        *   Attaches the `vShaderSource` to it (`glShaderSource`) and compiles it (`glCompileShader`).
        *   Checks for compilation errors using `glGetShaderiv` and `glGetShaderInfoLog`. If errors occur, prints them to `std::cerr`, deletes the shader, and returns 0.
        *   Creates a fragment shader object (`glCreateShader(GL_FRAGMENT_SHADER)`).
        *   Attaches the `fShaderSource` to it and compiles it.
        *   Checks for compilation errors for the fragment shader. If errors occur, prints them, deletes both shaders, and returns 0.
        *   Creates a shader program object (`glCreateProgram()`).
        *   Attaches both compiled vertex and fragment shaders to the program (`glAttachShader`).
        *   Links the program (`glLinkProgram()`).
        *   Checks for linking errors using `glGetProgramiv` and `glGetProgramInfoLog`. If errors occur, prints them, deletes the program and shaders, and returns 0.
        *   Detaches the shaders from the program (`glDetachShader`) and deletes the individual shader objects (`glDeleteShader`) as they are now linked into the program.
        *   Returns the ID of the linked shader program.

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
This file implements the `Chunk` class methods, responsible for managing a `CHUNK_SIZE_X` x `CHUNK_SIZE_Y` x `CHUNK_SIZE_Z` segment of the game world. It includes logic for terrain generation within the chunk, building an optimized mesh of only the visible block faces (`buildSurfaceMesh`), rendering this mesh, and managing individual blocks. It requires `<glm/gtc/type_ptr.hpp>` for `glm::value_ptr` when setting shader uniforms.

*   **Global Constants:**
    *   `faceVertices` (const glm::vec3[][4]): A 2D array defining the 4 vertex positions for each of the 6 faces of a unit cube (centered at origin). Used by `buildSurfaceMesh`.
        *   Indices: `[face_index][vertex_index_of_face]`
        *   Face order: Back (-Z), Front (+Z), Left (-X), Right (+X), Bottom (-Y), Top (+Y).
    *   `texCoords` (const glm::vec2[]): An array defining the 4 standard texture coordinates for a quadrilateral face (0,0 to 1,1). Used by `buildSurfaceMesh`.
*   **`Chunk`** - (Implementation of methods)
    *   **Constructor `Chunk(const glm::vec3& position)`:**
        *   Initializes the chunk\'s `position` and sets `needsRebuild` to true.
        *   Resizes the 3D `blocks` vector to the defined `CHUNK_SIZE` dimensions, initializing all block shared_ptrs to `nullptr`.
    *   **Destructor `~Chunk()`:**
        *   Calls `cleanupMesh()` to release any OpenGL resources associated with the chunk\'s mesh.
    *   **`init(const World* world)`:**
        *   Checks if terrain has been generated (e.g., by seeing if `blocks[0][0][0]` is null). If not, calls `generateTerrain()`.
        *   Calls `buildSurfaceMesh(world)` to create the renderable mesh of visible faces.
        *   Sets `needsRebuild` to false.
    *   **`generateTerrain()`:**
        *   Creates a temporary `representativeBlock`.
        *   Calls `representativeBlock.init()` to compile shaders and `representativeBlock.loadTexture("res/textures/spritesheet.png")` to load the default block texture. This ensures these resources are prepared once per block type.
        *   Iterates through all `x, y, z` coordinates within the chunk dimensions:
            *   Calculates the `blockWorldPos` for the current local coordinates.
            *   Creates a new `std::make_shared<Block>` at this world position.
            *   Calls `block->shareTextureAndShaderFrom(representativeBlock)` to make the new block use the already prepared shader and texture, avoiding redundant work.
            *   Calls `setBlockAtLocal(x, y, z, block)` to add the block to the chunk\'s internal `blocks` array.
        *   Prints a message indicating the chunk at its position has been generated. (Currently, this fills the entire chunk with blocks).
    *   **`buildSurfaceMesh(const World* world)`:**
        *   Calls `cleanupMesh()` to clear any existing mesh data if this is a rebuild.
        *   Initializes `meshVertices` (vector of floats for position and tex coords) and `meshIndices` (vector of unsigned ints for triangle indices).
        *   Defines `neighborOffsets` (a 6x3 array) to easily get coordinates of adjacent blocks for face culling.
        *   Iterates `y`, then `x`, then `z` through the chunk\'s dimensions:
            *   If `!hasBlockAtLocal(x, y, z)`, skips to the next iteration (air block).
            *   Calculates the `worldX, worldY, worldZ` of the current block.
            *   Iterates through each of the 6 faces (`face = 0 to 5`):
                *   Calculates the `neighborWorldX, neighborWorldY, neighborWorldZ` using `neighborOffsets`.
                *   Checks if the neighbor block exists using `world->getBlockAtWorldPos(...)`. The `world` pointer is used here to check blocks in adjacent chunks.
                *   If the neighbor is air (or outside the loaded world, meaning `getBlockAtWorldPos` returns null), then the current face is visible:
                    *   Adds the 4 vertices for this face to `meshVertices`:
                        *   Vertex positions are `(x,y,z) + faceVertices[face][i]`.
                        *   Texture coordinates are `texCoords[i]`.
                        *   Data is interleaved: (posX, posY, posZ, texU, texV).
                    *   Adds 6 indices (for 2 triangles forming the quad) to `meshIndices`, using `vertexIndexOffset`.
                    *   Increments `vertexIndexOffset` by 4.
        *   If `meshVertices` is empty after checking all blocks (e.g., an entirely empty or internal chunk), sets `surfaceMesh.indexCount = 0` and returns.
        *   Creates OpenGL buffers for the generated mesh data:
            *   `glGenVertexArrays(1, &surfaceMesh.VAO)`
            *   `glGenBuffers(1, &surfaceMesh.VBO)`
            *   `glGenBuffers(1, &surfaceMesh.EBO)`
        *   Binds `surfaceMesh.VAO`.
        *   Configures `surfaceMesh.VBO`: binds it, uploads `meshVertices.data()` using `glBufferData`.
        *   Configures `surfaceMesh.EBO`: binds it, uploads `meshIndices.data()` using `glBufferData`.
        *   Sets up vertex attribute pointers for the VBO (interleaved data):
            *   Position: `layout (location = 0)`, 3 floats, stride 5*sizeof(float), offset 0.
            *   Texture Coords: `layout (location = 1)`, 2 floats, stride 5*sizeof(float), offset 3*sizeof(float).
        *   Unbinds VBO and VAO.
        *   Sets `surfaceMesh.indexCount` to `meshIndices.size()`.
        *   Sets `needsRebuild = false`.
    *   **`renderSurface(const glm::mat4& projection, const glm::mat4& view)`:**
        *   Returns if `surfaceMesh.indexCount` or `surfaceMesh.VAO` is 0 (nothing to render).
        *   Finds a `representativeBlock` from the chunk to get shared shader and texture information. (Iterates through `blocks` until a non-null block is found).
        *   Returns if no `representativeBlock` is found.
        *   Gets `shaderProg = representativeBlock->getShaderProgram()`. Returns if `shaderProg` is 0.
        *   Activates the `shaderProg` using `glUseProgram()`.
        *   Calculates the `model` matrix for the chunk: `glm::translate(glm::mat4(1.0f), position)`.
        *   Sets "model", "view", and "projection" matrix uniforms in the shader.
        *   If `representativeBlock->hasTextureState()` is true:
            *   Sets "useTexture" uniform to true.
            *   Activates texture unit 0 (`glActiveTexture(GL_TEXTURE0)`).
            *   Binds the texture using `glBindTexture(GL_TEXTURE_2D, representativeBlock->getTextureID())`.
            *   Sets "blockTexture" sampler uniform to 0.
        *   Else (no texture):
            *   Sets "useTexture" uniform to false.
            *   Sets the "blockColor" uniform using the color obtained from `representativeBlock->getColor()`.
        *   Binds `surfaceMesh.VAO`.
        *   Draws the chunk mesh using `glDrawElements(GL_TRIANGLES, surfaceMesh.indexCount, GL_UNSIGNED_INT, 0)`.
        *   Unbinds VAO and texture.
    *   **`renderAllBlocks(const glm::mat4& projection, const glm::mat4& view)`:**
        *   Iterates through all `x, y, z` coordinates within the chunk.
        *   Gets `block = getBlockAtLocal(x, y, z)`.
        *   If `block` exists, calls `block->render(projection, view)` to render it individually. (This is less efficient than `renderSurface`).
    *   **`hasBlockAtLocal(int x, int y, int z) const` (bool):**
        *   Checks if `x, y, z` are within the chunk\'s local bounds (0 to CHUNK_SIZE-1).
        *   Returns `blocks[x][y][z] != nullptr`.
    *   **`getBlockAtLocal(int x, int y, int z) const` (std::shared_ptr<Block>):**
        *   Performs bounds checking for `x, y, z`. If out of bounds, returns `nullptr`.
        *   Otherwise, returns `blocks[x][y][z]`.
    *   **`setBlockAtLocal(int x, int y, int z, std::shared_ptr<Block> block)`:**
        *   Performs bounds checking. If out of bounds, returns.
        *   Sets `blocks[x][y][z] = block`.
        *   Sets `needsRebuild = true` because the chunk\'s mesh has changed.
    *   **`removeBlockAtLocal(int x, int y, int z)`:**
        *   Performs bounds checking. If out of bounds, returns.
        *   If `blocks[x][y][z]` exists:
            *   Sets `blocks[x][y][z] = nullptr`.
            *   Sets `needsRebuild = true`.
    *   **`getPosition() const` (glm::vec3):**
        *   Returns the chunk\'s `position`.
    *   **`cleanupMesh()`:**
        *   If `surfaceMesh.VAO` is not 0, deletes VAO, VBO, and EBO using `glDeleteVertexArrays` and `glDeleteBuffers`.
        *   Resets `surfaceMesh.VAO, VBO, EBO, indexCount` to 0.

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
        *   Activates the shader program by calling `glUseProgram(ID)`.
    *   **`setBool(const std::string& name, bool value) const`:**
        *   Sets a boolean uniform variable. `glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value)`.
    *   **`setInt(const std::string& name, int value) const`:**
        *   Sets an integer uniform variable. `glUniform1i(glGetUniformLocation(ID, name.c_str()), value)`.
    *   **`setFloat(const std::string& name, float value) const`:**
        *   Sets a float uniform variable. `glUniform1f(glGetUniformLocation(ID, name.c_str()), value)`.
    *   **`setVec3(const std::string& name, const glm::vec3& value) const`:**
        *   Sets a 3-component vector uniform. `glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value))`.
    *   **`setMat4(const std::string& name, const glm::mat4& value) const`:**
        *   Sets a 4x4 matrix uniform. `glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value))`.
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
*   **`Texture::loadFromFile(...)`**: Loads an image using `stbi_load`, generates an OpenGL texture, sets parameters (wrapping, filtering - uses `GL_NEAREST` for pixel art), and uploads data using `glTexImage2D`. Frees `stb_image` data.
*   **`Texture::loadFromSpritesheet(...)`**: Loads the entire spritesheet using `stbi_load`. Validates the requested sub-region coordinates (`atlasX`, `atlasY`, `atlasWidth`, `atlasHeight`). Allocates memory for the sub-image data and copies pixels from the full spritesheet data into this new buffer. Then, generates an OpenGL texture using this sub-image data, similar to `loadFromFile`. Frees both the full spritesheet data and the temporary sub-image buffer.
*   **`Texture::bind(...)`**: Activates texture unit and binds the texture.
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
    *   **`enableMouseCapture(bool enable)`:**
        *   Contains commented-out code for enabling/disabling mouse cursor capture using `glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED/NORMAL)`.
        *   A disclaimer notes this functionality is currently disabled due to potential issues with window managers.
    *   **`getMouseOffset(double& x, double& y)`:**
        *   Assigns `xOffset` to `x` and `yOffset` to `y`.
    *   **`resetMouseOffset()`:**
        *   Sets `xOffset` and `yOffset` to 0.0.
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
This file implements the `World` class methods, which manage the collection of chunks that make up the game world. It handles world initialization (creating a grid of chunks), rendering visible chunks based on camera position and render distance, and provides utilities for accessing chunks and blocks by various coordinate systems.

*   **`World`** - (Implementation of methods)
    *   **Constructor `World(int renderDistance)`:**
        *   Initializes the `renderDistance` member.
    *   **Destructor `~World()`:**
        *   The comment indicates that smart pointers (`std::shared_ptr<Chunk>`) will handle the cleanup of `Chunk` objects when the `chunks` map is cleared or the `World` object is destroyed.
    *   **`init(int gridSize)`:**
        *   Calculates `halfGrid = gridSize / 2`.
        *   **Generate Chunks:**
            *   Iterates `x` from `-halfGrid` to `halfGrid`.
            *   Iterates `z` from `-halfGrid` to `halfGrid`.
            *   Calls `addChunk(x, z)` for each pair to create and store chunks in an initial grid.
        *   **Build Meshes:**
            *   Prints "Building chunk meshes..." to `std::cout`.
            *   Iterates through the `chunks` map (using a structured binding `auto const& [pos, chunk]`).
            *   Calls `chunk->init(this)` for each chunk. Passing `this` (the `World` object) allows chunks to query neighboring chunks during their mesh building process (e.g., for face culling). This step is done *after* all chunks are created to ensure neighbor information is available.
        *   **Display Statistics:**
            *   Calculates `totalBlocks` based on the number of chunks and chunk dimensions.
            *   Prints the number of initialized chunks, total blocks in loaded chunks, and the render distance to `std::cout`.
    *   **`render(const glm::mat4& projection, const glm::mat4& view, const Camera& camera)` - Renders all chunks that are within the `renderDistance` of the camera.
        *   It first determines the chunk coordinates (chunkX, chunkZ) where the camera is currently located using `worldToChunkCoords(camera.getPosition())`.
        *   Then, it iterates in a square area around this central chunk, from `(cameraChunkX - renderDistance)` to `(cameraChunkX + renderDistance)` and `(cameraChunkZ - renderDistance)` to `(cameraChunkZ + renderDistance)`.
        *   For each chunk coordinate pair in this range, it retrieves the corresponding chunk using `getChunkAt()`.
        *   If the chunk exists, it calls the chunk\'s `renderSurface()` method (or potentially `renderAllBlocks()` for the chunk the player is in, as seen in `main.cpp`\'s logic though not explicitly in `world.cpp` shown so far). It ensures chunks have their meshes rebuilt if `needsMeshRebuild()` is true.
    *   **`getChunkAt(int chunkX, int chunkZ)` (std::shared_ptr<Chunk>) - Retrieves a shared pointer to the chunk at the specified chunk coordinates (chunkX, chunkZ). Returns `nullptr` if no chunk exists at those coordinates.
    *   **`addChunk(int chunkX, int chunkZ)` - Creates a new `Chunk` at the given chunk coordinates (chunkX, chunkZ). It calculates the world position for the new chunk, creates a `std::make_shared<Chunk>` instance, calls `init(this)` on the new chunk (passing `this` world object for context like neighbor lookups during mesh building), and then stores it in the `chunks` map.
    *   **`worldToChunkCoords(const glm::vec3& worldPos) const` (glm::ivec2) - Converts a 3D world position (`worldPos`) into 2D chunk coordinates (X, Z). It divides the world X and Z coordinates by `chunkSize` and floors the result to get the integer chunk indices.
    *   **`getBlockAtWorldPos(int worldX, int worldY, int worldZ) const` (std::shared_ptr<Block>) - Retrieves a block from its absolute world coordinates. It first converts the world coordinates to chunk coordinates using `worldToChunkCoords`. Then, it gets the corresponding chunk using `getChunkAt()`. If the chunk exists, it calculates the local coordinates of the block within that chunk and calls the chunk\'s `getBlockAtLocal()` method. Returns `nullptr` if the chunk or block doesn\'t exist.

---

### `shaders/vertex.glsl` in `shaders/`
This file likely contains the GLSL code for the vertex shader used by the application. Vertex shaders are responsible for processing individual vertices, performing transformations (like model-view-projection), and passing data (like texture coordinates) to the next stage in the graphics pipeline.
*(Note: Content not read, this is a general description)*

---

### `shaders/fragment.glsl` in `shaders/`
This file likely contains the GLSL code for the fragment shader (also known as a pixel shader). Fragment shaders are responsible for determining the final color of each pixel on the screen. This typically involves texture sampling, lighting calculations, and other effects.
*(Note: Content not read, this is a general description)*

---

### `shaders/vortex.glsl` in `shaders/`
This file is present in the shaders directory but was noted to be empty (0 bytes). It might be a placeholder for a future shader or an unused remnant.
*(Note: File is empty)*

---

### `res/textures/grass_block.png` in `res/textures/`
This is an image file for a grass block texture. It can be programmatically generated by `create_texture.py`. (Note: As of the latest changes, the primary block texture loaded by default is `spritesheet.png`. This file might be unused or used for other purposes if `create_texture.py` is run manually.)
*(Note: This is an image file, content not displayed as text)*

### `res/textures/spritesheet.png` in `res/textures/`
This is the primary spritesheet image file intended for block textures. It is loaded by default in `Chunk::generateTerrain`. If this file is a texture atlas, all block faces will currently map the entire atlas. Specific sub-texture selection would require UV coordinate adjustments. If blocks appear white or untextured, ensure this file exists at `res/textures/spritesheet.png` (accessible from the build directory as `build/res/textures/spritesheet.png`), that it is a valid image file, and check the console output for any texture loading error messages.
*(Note: This is an image file, content not displayed as text)*

---
### `external/stb_image.h` in `external/`
This is a single-file public domain image loading library for C/C++. It's used in this project (specifically in `src/texture.cpp`) to load image files (like PNGs for textures) from disk into memory so they can be processed and uploaded to the GPU as OpenGL textures.
*(Note: Third-party library header, content not detailed here but widely available)*

 