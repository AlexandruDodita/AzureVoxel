cmake_minimum_required(VERSION 3.10)
project(AzureVoxel)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prefer GLVND over legacy GL if available
set(OpenGL_GL_PREFERENCE "GLVND")

# Find OpenGL
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 3.3 REQUIRED)

# Source files
set(SOURCES
    main.cpp
    src/window.cpp
    src/block.cpp
    src/texture.cpp
    src/camera.cpp
    src/shader.cpp
    src/chunk.cpp
    src/world.cpp
)

# Headers
set(HEADERS
    headers/window.h
    headers/block.h
    headers/texture.h
    headers/camera.h
    headers/shader.h
    headers/chunk.h
    headers/world.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${OPENGL_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OPENGL_LIBRARIES}
    GLEW::GLEW
    glfw
)

# Copy shader files to the build directory
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})

# Copy resource directory to the build directory
file(COPY ${CMAKE_SOURCE_DIR}/res DESTINATION ${CMAKE_BINARY_DIR})
