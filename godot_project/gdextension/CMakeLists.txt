cmake_minimum_required(VERSION 3.16)
project(chicken_fps_gdextension LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GODOT_CPP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../external/godot-cpp" CACHE PATH "Path to a godot-cpp checkout or install")
set(GODOT_CPP_API_VERSION "4.6" CACHE STRING "Godot API version used by the godot-cpp bindings")

if(NOT EXISTS "${GODOT_CPP_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "godot-cpp is missing at ${GODOT_CPP_ROOT}. Clone the godot-cpp repo there before building Phase C.")
endif()

set(GODOTCPP_API_VERSION "${GODOT_CPP_API_VERSION}" CACHE STRING "" FORCE)
set(GODOTCPP_GDEXTENSION_DIR "${GODOT_CPP_ROOT}/gdextension" CACHE PATH "" FORCE)
set(GODOTCPP_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(GODOTCPP_DEV_BUILD OFF CACHE BOOL "" FORCE)

add_subdirectory(${GODOT_CPP_ROOT} godot-cpp-build)

add_library(chicken_fps_gdextension SHARED
    src/extension_entry.cpp
    src/game_core_bridge.cpp
    ../../src/game_core.cpp
)

target_include_directories(chicken_fps_gdextension PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${GODOT_CPP_ROOT}/include
    ${CMAKE_CURRENT_BINARY_DIR}/godot-cpp-build/gen/include
)

target_link_libraries(chicken_fps_gdextension PRIVATE godot-cpp)

set_target_properties(chicken_fps_gdextension PROPERTIES
    OUTPUT_NAME "chicken_fps"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/linux"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/linux"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/linux"
)
