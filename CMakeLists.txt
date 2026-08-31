cmake_minimum_required(VERSION 3.20)
project(ApexEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(
    Engine/Source
    Engine/Source/Runtime
)

set(ENGINE_SOURCES
    Engine/Source/Programs/ApexEngineApp/main.cpp
)

add_executable(ApexEngine ${ENGINE_SOURCES})
target_link_libraries(ApexEngine PRIVATE pthread)

set_target_properties(ApexEngine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
