# AzureVoxel

A voxel-based game engine built with C++ and OpenGL.

## Overview

AzureVoxel is a cross-platform 3D voxel engine inspired by games like Minecraft. The engine supports rendering cubes with textures in a 3D world with first-person camera controls, and features optimized chunk rendering.

## Features

- 3D voxel rendering with textured blocks
- First-person camera with WASD movement and mouse look
- Chunk-based world system with optimized rendering
- Wireframe mode toggle for debugging (X key)
- Cross-platform support (Windows, Linux)
- Basic terrain generation
- Performance optimization (only renders nearby chunks)

## Getting Started

### Prerequisites

- CMake 3.10 or higher
- C++17 compatible compiler
- OpenGL 3.3+ compatible graphics card
- GLEW
- GLFW

### Building on Linux

```bash
# Clone the repository
git clone https://github.com/yourusername/AzureVoxel.git
cd AzureVoxel

# Create a build directory
mkdir -p build
cd build

# Configure and build
cmake ..
make
```

### Building on Windows

```bash
# Clone the repository
git clone https://github.com/yourusername/AzureVoxel.git
cd AzureVoxel

# Create a build directory
mkdir build
cd build

# Configure and build
cmake -G "Visual Studio 16 2019" -A x64 ..
```

Then open the generated solution in Visual Studio and build.

## Controls

- **W/A/S/D** - Move forward/left/backward/right
- **Mouse** - Look around
- **X key** - Toggle wireframe mode
- **ESC** - Exit the application

## Performance

The engine implements optimized chunk rendering:
- Renders only chunks within a configurable distance from the player
- Default configuration: 5x5 grid of chunks (25 total), but only renders the 12 nearest chunks
- Calculates chunk distance based on player position to optimize rendering

## Project Structure

- `main.cpp` - Entry point
- `headers/` - Header files
- `src/` - Implementation files
- `shaders/` - GLSL shader files
- `res/` - Resources (textures, etc.)

### Key Components

- **Window System**: Handles window creation, input, and wireframe mode toggle
- **Camera System**: First-person camera with movement and look controls
- **Block System**: Renders individual blocks with textures
- **Chunk System**: Manages collections of blocks in a 3D grid
- **World System**: Manages multiple chunks and implements rendering optimization
- **Shader System**: Loads and manages GLSL shaders for rendering

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgements

- OpenGL
- GLFW
- GLEW
- GLM
- stb_image for texture loading
