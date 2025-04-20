# AzureVoxel Project Guide

## Project Overview

AzureVoxel is a 3D voxel-based rendering engine built with OpenGL. The project is capable of rendering multiple chunks of blocks in a 3D world with first-person camera controls and includes optimization for rendering only nearby chunks.

## Architecture

### Core Components

1. **Window Management (`window.h/cpp`)**
   - Handles the creation and management of the OpenGL window using GLFW
   - Processes basic input events (keyboard and mouse)
   - Manages window properties and rendering context
   - Provides wireframe mode toggle functionality (X key)

2. **Camera System (`camera.h/cpp`)**
   - Implements a first-person camera with standard FPS controls
   - Handles camera movement (WASD + mouse look)
   - Manages view and projection matrices

3. **Block Rendering (`block.h/cpp`)**
   - Basic building block of the voxel world
   - Handles the rendering of a single 3D cube with texture mapping
   - Contains shader code for rendering blocks

4. **Texture Management (`texture.h/cpp`)**
   - Handles loading and binding of texture files
   - Supports basic texture operations

5. **Chunk System (`chunk.h/cpp`)**
   - Manages a collection of blocks in a fixed-size 3D grid
   - Handles terrain generation and rendering optimization
   - Provides methods for block manipulation

6. **World System (`world.h/cpp`)**
   - Manages multiple chunks in a world
   - Handles chunk creation and management
   - Implements rendering optimization to only render chunks near the player
   - Supports dynamic chunk loading based on player position

7. **Shader System (`shader.h/cpp`)**
   - Loads and compiles GLSL shaders from files
   - Manages shader programs and uniforms
   - Provides error handling for shader compilation

## Implementation Details

### Graphics Pipeline

- Uses modern OpenGL with GLEW for extension handling
- Shaders are loaded from external GLSL files
- Vertex and Fragment shaders implement basic texture mapping
- OpenGL 3.3+ Core Profile is utilized for rendering
- Supports wireframe rendering mode for debugging (toggle with X key)

### Rendering Approach

- Uses a chunk-based rendering system for efficiency
- Each chunk contains a 16x16x16 grid of blocks
- World contains multiple chunks (default 5x5 grid = 25 chunks)
- Implements distance-based chunk rendering (only renders chunks near the player)
- Supports both textured and colored blocks

### Performance Optimization

- Only renders chunks within a configurable distance from the player
- Uses smart pointers for efficient memory management
- Calculates Euclidean distance to determine which chunks to render
- Default setting renders only the 12 closest chunks around the player

### Input Handling

- Camera movement via WASD keys
- Camera orientation controlled by mouse movement
- Wireframe mode toggle with X key
- Basic window events (close, resize) are handled

## Critical Issues

### Texture Loading Problems

The project currently has issues with texture loading:

1. **Missing Texture Files**: The texture file format is incorrect (`grass_block.pngZone.Identifier` instead of `grass_block.png`)
2. **Platform Compatibility**: Texture paths may need adjustment for cross-platform support
3. **Error Handling**: The texture loading code falls back to default colors but should be more robust

To fix these issues:

1. Add proper texture files to `res/textures/` directory
2. Ensure path separators are handled correctly for cross-platform compatibility
3. Implement better error messaging for texture loading failures
4. Consider using a texture atlas for multiple block types

### Empty Shader Files

The shader files (`fragment.glsl` and `vortex.glsl`) are currently empty, with the shader code hardcoded in `block.cpp`. This should be fixed by:

1. Moving the shader code from `block.cpp` to the appropriate GLSL files
2. Implementing a proper shader loading system
3. Adding error handling for shader compilation and linking

## Improvements Made

### 1. Build System Fixes

- Fixed CMake configuration to ensure proper cross-platform compatibility
- Cleaned CMake cache to resolve build issues when moving between platforms
- Organized project structure with proper build directory

### 2. Texture Management

- Created a simple Python script to generate a basic grass block texture
- Fixed texture file format issues
- Ensured textures are properly copied to the build directory
- Added better error handling for texture loading

### 3. Shader Management

- Extracted hardcoded shader code from `block.cpp` to dedicated GLSL files
- Created a proper Shader class for loading and managing shaders
- Implemented error handling for shader compilation and linking
- Added utility functions for setting uniform variables

### 4. Chunk System Implementation

- Created a Chunk class to manage multiple blocks efficiently
- Implemented a 3D grid system for block positioning
- Added basic terrain generation functionality
- Provided methods for block manipulation within chunks

### 5. World System Implementation

- Created a World class to manage multiple chunks
- Implemented chunk rendering optimization based on distance from player
- Added support for creating a grid of chunks (5x5 = 25 chunks)
- Implemented efficient chunk lookup using hash maps

### 6. Debug Features

- Added wireframe rendering mode toggle (X key) for easier debugging
- Implemented camera position display in console
- Added chunk count information

### 7. Documentation

- Created a comprehensive README.md with build instructions
- Developed this SUBJECT_GUIDE.md document
- Added detailed comments throughout the codebase
- Created a roadmap for future improvements

## Areas for Further Improvement

### 1. Code Structure and Organization

- **Shader Management**: Continue improving shader loading and management
- **Resource Management**: Implement proper resource management with RAII principles
- **Error Handling**: Add more robust error handling throughout the codebase

### 2. Missing Features

- **Chunk Implementation**: Enhance the chunk system with more features
- **World Generation**: Add procedural terrain generation with hills, caves, etc.
- **Texturing**: Implement texture atlas for different block types
- **Physics**: Add basic collision detection and physics interaction

### 3. Performance Optimizations

- **Frustum Culling**: Only render blocks that are visible to the camera
- **Instanced Rendering**: Use instanced rendering for identical blocks
- **Occlusion Culling**: Skip rendering of hidden blocks
- **Mesh Optimization**: Combine faces that are adjacent and share the same texture

### 4. Cross-Platform Compatibility

- **Build System**: Improve CMake configuration for better cross-platform support
- **Platform-Specific Code**: Abstract platform-specific implementations
- **Dependencies**: Manage external dependencies more effectively

### 5. Documentation

- **Code Documentation**: Add comprehensive comments and documentation
- **Setup Instructions**: Create detailed setup and build instructions
- **Architecture Diagrams**: Add visual representations of the code architecture

## Controls

- **WASD**: Move forward/left/backward/right
- **Mouse**: Look around
- **X key**: Toggle wireframe mode
- **ESC**: Exit the application

## Development Guidelines

- Follow consistent naming conventions and code style
- Abstract OpenGL-specific code behind higher-level interfaces
- Use modern C++ features and idioms
- Write unit tests for core components
- Keep performance in mind, especially for render-critical code paths 