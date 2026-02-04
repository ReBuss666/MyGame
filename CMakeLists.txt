cmake_minimum_required(VERSION 3.10)
project(MyGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set output directory to root
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Find SFML
find_package(SFML 3.0 COMPONENTS Graphics Window Audio System Network REQUIRED)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.cpp
    src/Rendering/FireEffect.cpp
    src/Rendering/SectorRenderer.cpp
    src/States/MenuState.cpp
    src/weapon/pistol.cpp
)

# Executable
add_executable(MyGame ${SOURCES})

# Link libraries
target_link_libraries(MyGame SFML::Graphics SFML::Window SFML::Audio SFML::System SFML::Network)

# Copy assets and settings to build directory
# Note: When building in root, assets are already there, but this ensures they exist for out-of-source builds
add_custom_command(TARGET MyGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/settings.cfg
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/settings.cfg
)
