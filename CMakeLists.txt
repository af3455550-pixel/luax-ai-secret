cmake_minimum_required(VERSION 3.16)
project(FirelineWildfireCommand VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(FIRELINE_USE_RAYLIB "Try to use raylib if available" OFF)
option(FIRELINE_BUILD_RELEASE "Build optimized release" ON)

if(FIRELINE_BUILD_RELEASE)
    set(CMAKE_BUILD_TYPE Release)
    add_compile_options(-O3 -march=native -DNDEBUG)
else()
    set(CMAKE_BUILD_TYPE Debug)
    add_compile_options(-g -O1)
endif()

# Core include
include_directories(${CMAKE_SOURCE_DIR}/src)

# Source files
file(GLOB_RECURSE FIRELINE_SOURCES
    src/*.cpp
    src/*.hpp
)

# Remove main from lib if needed, but we have single executable
add_executable(fireline ${FIRELINE_SOURCES})

# Threads
find_package(Threads REQUIRED)
target_link_libraries(fireline PRIVATE Threads::Threads)

# Math
target_link_libraries(fireline PRIVATE m)

# Raylib optional
if(FIRELINE_USE_RAYLIB)
    find_package(raylib QUIET)
    if(raylib_FOUND)
        target_link_libraries(fireline PRIVATE raylib)
        target_compile_definitions(fireline PRIVATE FIRELINE_HAS_RAYLIB=1)
        message(STATUS "Raylib found - enabling graphical backend")
    else()
        message(STATUS "Raylib not found - using software/console backend")
        target_compile_definitions(fireline PRIVATE FIRELINE_HAS_RAYLIB=0)
    endif()
else()
    target_compile_definitions(fireline PRIVATE FIRELINE_HAS_RAYLIB=0)
endif()

# Platform specific
if(UNIX AND NOT APPLE)
    target_link_libraries(fireline PRIVATE dl)
endif()

# Install
install(TARGETS fireline DESTINATION bin)

# Custom target for credits demo
add_custom_target(run_credits
    COMMAND fireline --credits
    DEPENDS fireline
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

message(STATUS "Fireline Wildfire Command v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
