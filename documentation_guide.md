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
    *   Creates a `World` object, defining its render distance (e.g., 3 chunks, adjustable for performance/viewing needs). The world dynamically loads/generates chunks based on this render distance and player position via its `update()` method.
    *   Enters the main game loop which continues until the window is closed.
        *   Calculates `deltaTime` for frame-rate independent movement and physics.
        *   Calculates and updates Frames Per Second (FPS).
        *   Processes keyboard input via the `camera.processKeyboard()` method.
        *   Processes mouse input for camera orientation using `window.getMouseOffset()` and `camera.processMouseMovement()`.
        *   Creates the view matrix using `camera.getViewMatrix()`.
        *   Creates the projection matrix using `glm::perspective`, defining the camera's field of view, aspect ratio, and near/far clipping planes.
        *   Clears the screen with a sky blue color and clears the depth buffer.
        *   Calls `world.update(camera)` to handle dynamic loading/unloading of chunks around the camera. This method now ensures chunks are initialized (loaded from file or generated using a seed, saved, and meshed) as they come into render distance.
        *   Renders the currently loaded and active chunks using `world.render()`, passing the projection, view matrices, and camera object.
        *   Prints the current camera position and FPS to the console.
        *   Swaps the front and back buffers of the window (`window.swapBuffers()`) to display the rendered frame.
    *   After the loop ends (window is closed), it prints a newline and returns 0, indicating successful execution.

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
This file contains instructions for CMake, a cross-platform build system generator. It defines how the AzureVoxel project should be built, including specifying the C++ standard, finding necessary libraries (OpenGL, GLEW, GLFW), listing source and header files, creating the executable, setting include directories, and linking libraries. It also handles copying shader and resource files (including `Spritesheet.PNG`) to the build directory.

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
        *   `renderSurface(const glm::mat4& projection, const glm::mat4& view)`: Renders the chunk using its `surfaceMesh`.
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
This header file defines the `World` class, which manages the overall game environment, including all chunks. It handles dynamic loading and unloading of chunks based on camera position and render distance, and provides access to blocks at world coordinates.

*   **`IVec2Hash` (struct)**: A custom hash function for `glm::ivec2` keys, enabling their use in `std::unordered_map`. It combines the hashes of the x and y components.
*   **`IVec2Compare` (struct)**: A custom comparison function for `glm::ivec2` keys, enabling their use in `std::set`. It compares x components first, then y components.
*   **`World` (class)**:
    *   **Private Members:**
        *   `chunks` (std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>, IVec2Hash>): An unordered map storing shared pointers to `Chunk` objects, keyed by their 2D chunk coordinates (x, z).
        *   `renderDistance` (int): The number of chunks to load and render in each direction (x and z) from the player's current chunk. E.g., a value of 3 means a (2*3+1)x(2*3+1) = 7x7 grid.
        *   `chunkSize` (const int): The size of each chunk in one dimension (e.g., 16 blocks). This must match the `CHUNK_SIZE_X/Z` constants in `chunk.h`.
        *   `worldSeed_` (int): The seed used for procedural world generation, ensuring reproducible terrain if other parameters are the same.
        *   `worldDataPath_` (std::string): The directory path where chunk files are saved and from where they are loaded.
        *   `saveAllChunks() const`: A private helper method to iterate through all currently loaded chunks and call their `saveToFile()` method.
    *   **Public Members:**
        *   `World(int renderDistance = 3)` (Constructor): Initializes the world with a given `renderDistance`. It also creates the `chunk_data` directory if it doesn't exist, initializes `worldSeed_` to a default value, and `worldDataPath_` to point to the `chunk_data` directory.
        *   `~World()` (Destructor): Calls `saveAllChunks()` to ensure all modified chunk data is written to disk before the world is destroyed.
        *   `update(const Camera& camera)`: This method is called every frame to manage which chunks are active.
            *   It determines the player's current chunk coordinates.
            *   It identifies a square region of chunks around the player based on `renderDistance`.
            *   For each chunk coordinate in this active region: if the chunk is not already in the `chunks` map, a new `Chunk` object is created, and its `ensureInitialized(this, worldSeed_, worldDataPath_)` method is called. This method handles loading the chunk from file if it exists, or generating new terrain (using `worldSeed_`), saving it, and building its mesh if it does not.
            *   It then iterates through the currently loaded chunks in the `chunks` map. If a chunk is found to be outside the new active region, it is saved to a file (if modified) and then removed from the `chunks` map to free resources.
        *   `render(const glm::mat4& projection, const glm::mat4& view, const Camera& camera)`: Renders all currently loaded (and initialized) chunks.
            *   It iterates through the `chunks` map.
            *   Mesh building is no longer explicitly called here as `ensureInitialized` (called in `update`) guarantees that loaded/generated chunks have their meshes built if needed.
            *   It distinguishes between the chunk the camera is currently in and other chunks, potentially using `renderAllBlocks` for the current chunk (for higher detail or effects) and `renderSurface` for others.
        *   `getChunkAt(int chunkX, int chunkZ)` (std::shared_ptr<Chunk>): Retrieves a chunk by its chunk coordinates. Returns `nullptr` if the chunk is not loaded.
        *   `addChunk(int chunkX, int chunkZ)`: (Note: This method might be deprecated or its primary logic incorporated into `update`. The current `update` logic directly adds chunks if they are missing from the `activeChunkCoords` set and not found in `chunks` map.)
        *   `worldToChunkCoords(const glm::vec3& worldPos) const` (glm::ivec2): Converts a 3D world position into 2D chunk coordinates (x, z).
        *   `getBlockAtWorldPos(int worldX, int worldY, int worldZ) const` (std::shared_ptr<Block>): Retrieves a block at specific absolute world coordinates. It determines the correct chunk, then the local coordinates within that chunk, and returns the block.
        *   `getBlockAtWorldPos(const glm::vec3& worldPos) const` (std::shared_ptr<Block>): Overload that takes a `glm::vec3` world position.

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
This file implements the `Chunk` class, which represents a segment of the game world composed of many blocks. It handles terrain generation within the chunk, mesh building for efficient rendering of visible block faces, and saving/loading chunk data to/from files.

*   **`faceVertices` (const glm::vec3[][4])**: A constant array defining the vertex positions for each of the 6 faces of a cube, relative to the block's center.
*   **`texCoords` (const glm::vec2[])**: A constant array defining the standard texture coordinates for a square face.
*   **`Chunk(const glm::vec3& position)` (Constructor)**: Initializes a chunk at a given world `position`. Sets `needsRebuild` to true and `isInitialized_` to false. It also resizes the `blocks` 3D vector to the chunk dimensions, initially filled with `nullptr`.
*   **`~Chunk()` (Destructor)**: Calls `cleanupMesh()` to release OpenGL resources associated with the chunk's surface mesh.
*   **`init(const World* world)`**: This method's role has been significantly reduced. The primary initialization, including terrain generation/loading and mesh building, is now handled by `ensureInitialized()`. This method can be used for minimal, non-conditional setup if needed in the future, but is currently kept simple as `ensureInitialized()` is called by the `World` object.
*   **`simpleNoise(int x, int y, int z, int seed)` (float)**: A placeholder noise function that generates a pseudo-random float value based on 3D coordinates and a `seed`. The `seed` parameter allows for reproducible noise generation. This is intended to be replaced with a more sophisticated noise algorithm like Perlin or Simplex noise.
*   **`generateTerrain(int seed)`**: Populates the chunk with blocks based on a noise-generated height map.
    *   It uses the `simpleNoise` function (with the provided `seed`) to determine the terrain height at each (x, z) column within the chunk.
    *   Blocks are created up to the calculated `terrainHeight`. The top layer of blocks is currently assigned a "grass" texture, and blocks below it are assigned a "stone" texture by directly calling `loadTexture`. This is a simplification and will be replaced by a more robust block type and texture management system.
    *   A `representativeBlock` is used to initialize and share shader and texture resources among new blocks to improve efficiency.
*   **`buildSurfaceMesh(const World* world)`**: Constructs a single mesh for the chunk containing only the faces of blocks that are visible (i.e., adjacent to an air block or the boundary of the loaded world).
    *   It iterates through each block in the chunk.
    *   For each block, it checks its 6 neighbors (using `world->getBlockAtWorldPos()` for blocks in adjacent chunks or `hasBlockAtLocal` for blocks within the same chunk).
    *   If a neighbor is an air block or outside the loaded world, the shared face is considered visible and its vertices (positions and texture coordinates) and indices are added to `meshVertices` and `meshIndices` vectors.
    *   After processing all blocks, if `meshVertices` is not empty, it creates and populates OpenGL buffers (VAO, VBO, EBO) for the surface mesh.
    *   Vertex attributes for position (location 0) and texture coordinates (location 1) are defined.
    *   `surfaceMesh.indexCount` is updated, and `needsRebuild` is set to `false`.
*   **`renderSurface(const glm::mat4& projection, const glm::mat4& view)`**: Renders the pre-built surface mesh of the chunk.
    *   It first finds a `representativeBlock` within the chunk to access the shared shader program and texture.
    *   It sets shader uniforms (model, view, projection matrices, texture usage, and fallback color). The model matrix is a translation to the chunk's world `position`.
    *   If the representative block has a texture, it binds the texture. Otherwise, it uses the block's color.
    *   It binds the `surfaceMesh.VAO` and draws the mesh using `glDrawElements`.
*   **`renderAllBlocks(const glm::mat4& projection, const glm::mat4& view)`**: A slower rendering method that iterates through all blocks in the chunk and calls their individual `render()` methods. This is typically used for debugging or for the currently active/modifiable chunk where individual block rendering might be needed.
*   **`hasBlockAtLocal(int x, int y, int z) const` (bool)**: Checks if a block exists at the given local coordinates within the chunk. Returns `false` if coordinates are out of bounds or if the block pointer is `nullptr`.
*   **`getBlockAtLocal(int x, int y, int z) const` (std::shared_ptr<Block>)**: Returns a shared pointer to the block at the given local coordinates. Returns `nullptr` if coordinates are out of bounds or no block exists there.
*   **`setBlockAtLocal(int x, int y, int z, std::shared_ptr<Block> block)`**: Sets or replaces the block at the given local coordinates. If the block state changes (air to block or block to air), it sets `needsRebuild` to `true`.
*   **`removeBlockAtLocal(int x, int y, int z)`**: Removes the block at the given local coordinates by setting its pointer to `nullptr`. If a block was present, it sets `needsRebuild` to `true`.
*   **`cleanupMesh()`**: Deletes the OpenGL vertex array (VAO), vertex buffer (VBO), and element buffer (EBO) associated with the `surfaceMesh` and resets `surfaceMesh.indexCount` to 0.
*   **`getPosition() const` (glm::vec3)**: Returns the world position of the chunk (typically the corner with the lowest x, y, z coordinates).
*   **`getChunkFileName() const` (std::string)**: Generates a unique filename for the chunk based on its position (e.g., "chunk_X_Y_Z.chunk").
*   **`saveToFile(const std::string& directoryPath) const` (bool)**: Saves the chunk's block data to a binary file in the specified directory.
    *   Ensures the directory exists (creates it on Unix-like systems if not).
    *   Writes a simplified representation of block data (1 for block, 0 for air) to the file. This will be expanded later to save block types/IDs.
    *   Returns `true` on successful save, `false` otherwise.
*   **`loadFromFile(const std::string& directoryPath, const World* world)` (bool)**: Loads the chunk's block data from a binary file.
    *   If the file doesn't exist, it returns `false` (indicating the chunk needs to be generated).
    *   Reads block data (currently 0 or 1) and recreates `Block` objects. Loaded blocks share texture and shader from a `representativeBlock`.
    *   Sets `needsRebuild` to `true` if blocks are loaded.
    *   Returns `true` on successful load, `false` otherwise (e.g., file not found, read error).
*   **`ensureInitialized(const World* world, int seed, const std::string& worldDataPath)`**: This is the primary method for preparing a chunk.
    *   If the chunk `isInitialized_` flag is true, it returns immediately.
    *   It first attempts to `loadFromFile()` using `worldDataPath`.
    *   If loading fails (e.g., file not found), it calls `generateTerrain(seed)` to populate the chunk with new terrain based on the provided `seed`.
    *   After successful generation, it attempts to `saveToFile()` in `worldDataPath`.
    *   If `needsRebuild` is true (either from loading an existing chunk or after generating a new one), it calls `buildSurfaceMesh(world)`.
    *   Finally, it sets the `isInitialized_` flag to `true`.
*   **`isInitialized() const` (bool)**: Returns the state of the `isInitialized_` flag, indicating whether `ensureInitialized()` has been successfully run for this chunk.
*   **Private Members (Conceptual, based on .cpp changes):**
    *   `isInitialized_` (bool): Flag to track if `ensureInitialized` has been called and completed for this chunk. Initialized to `false` in the constructor.

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
This file implements the `World` class methods. It manages chunk loading, unloading, rendering, and provides access to blocks within the world. It uses a hardcoded directory `chunk_data` for saving and loading chunk files.

*   **`CHUNK_DATA_DIR` (const std::string)**: Defines the directory name ("chunk_data") for storing chunk files.
*   **`World(int renderDistance)` (Constructor)**:
    *   Initializes `renderDistance`.
    *   Creates the `CHUNK_DATA_DIR` directory if it doesn't already exist (using `mkdir` for Unix-like systems).
    *   Initializes `worldSeed_` to a hardcoded integer value (e.g., 12345).
    *   Initializes `worldDataPath_` to the value of `CHUNK_DATA_DIR`.
*   **`~World()` (Destructor)**:
    *   Calls `saveAllChunks()` to ensure all data is persisted.
*   **`update(const Camera& camera)`**:
    *   Calculates the camera's current chunk coordinates (`playerChunkPos`).
    *   Defines a `std::set` (`activeChunkCoords`) to keep track of chunks that should be loaded based on `renderDistance` around `playerChunkPos`.
    *   Iterates from `playerChunkPos.x - renderDistance` to `playerChunkPos.x + renderDistance` (and similarly for z/y component of `playerChunkPos` which represents the chunk's Z coordinate):
        *   For each `currentChunkKey` in this range, adds it to `activeChunkCoords`.
        *   If `chunks.find(currentChunkKey)` is not found (chunk is not loaded):
            *   A new `Chunk` is created at the corresponding world position.
            *   `newChunk->ensureInitialized(this, worldSeed_, worldDataPath_)` is called. This method encapsulates the logic: it attempts to load the chunk from `worldDataPath_`; if that fails, it generates new terrain using `worldSeed_`, saves the newly generated chunk to `worldDataPath_`, and builds its surface mesh.
            *   The `newChunk` is then added to the `chunks` map.
    *   Iterates through the `chunks` map to find chunks to unload:
        *   If a chunk's key is not found in `activeChunkCoords`, it means it's outside the render distance.
        *   The chunk is saved using `it->second->saveToFile(worldDataPath_)`.
        *   The chunk is then removed from the `chunks` map using `chunks.erase(it)`.
*   **`render(const glm::mat4& projection, const glm::mat4& view, const Camera& camera)`**:
    *   Determines the camera's current chunk coordinates (`camChunkPos`).
    *   Iterates through all chunks in the `chunks` map:
        *   The check `chunk->needsMeshRebuild()` and the call to `chunk->buildSurfaceMesh(this)` have been removed from this method. Mesh building is now handled within `chunk->ensureInitialized()`, which is called during the `World::update()` phase.
        *   It checks if the current iterating chunk (`pos`) is the same as `camChunkPos`.
        *   If it is the current chunk, `chunk->renderAllBlocks()` might be called (potentially for different rendering effects or higher detail on the player's current chunk).
        *   Otherwise, `chunk->renderSurface()` is called for efficient rendering of other chunks.
*   **`getChunkAt(int chunkX, int chunkZ)` (std::shared_ptr<Chunk>)**: (Implementation details as per header description).
*   **`worldToChunkCoords(const glm::vec3& worldPos) const` (glm::ivec2)**: (Implementation details as per header description).
*   **`getBlockAtWorldPos(int worldX, int worldY, int worldZ) const` (std::shared_ptr<Block>)**: (Implementation details as per header description).
*   **`saveAllChunks() const`**:
    *   Iterates through all chunks in the `chunks` map and calls `chunk->saveToFile(worldDataPath_)` for each valid chunk.

---

### `shaders/vertex.glsl` in `shaders/`
This file likely contains the GLSL code for the vertex shader used by the application. Vertex shaders are responsible for processing individual vertices, performing transformations (like model-view-projection), and passing data (like texture coordinates) to the next stage in the graphics pipeline.
*(Note: Content not read, this is a general description)*

---

### `shaders/fragment.glsl` in `shaders/`
This file likely contains the GLSL code for the fragment shader (also known as a pixel shader). Fragment shaders are responsible for determining the final color of each pixel on the screen. This typically involves texture sampling, lighting calculations, and other effects.
*(Note: Content not read, this is a general description)*


---
### `external/stb_image.h` in `external/`
This is a single-file public domain image loading library for C/C++. It's used in this project (specifically in `src/texture.cpp`) to load image files (like PNGs for textures) from disk into memory so they can be processed and uploaded to the GPU as OpenGL textures.
*(Note: Third-party library header, content not detailed here but widely available)*

### `res/textures/Spritesheet.PNG` in `res/textures/`
This is the primary spritesheet image file intended for block textures. It is loaded by default in `Chunk::generateTerrain`. If this file is a texture atlas, all block faces will currently map the entire atlas. Specific sub-texture selection would require UV coordinate adjustments. If blocks appear white or untextured, ensure this file exists at `res/textures/Spritesheet.PNG` (accessible from the build directory as `build/res/textures/Spritesheet.PNG`), that it is a valid image file, and check the console output for any texture loading error messages.
*(Note: This is an image file, content not displayed as text)*

 