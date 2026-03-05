cmake_minimum_required(VERSION 3.16)
project(ChickenPotatoFPS VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# raylib
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

add_executable(chicken_potato_fps
    src/main.cpp
)

target_link_libraries(chicken_potato_fps PRIVATE raylib)

if(WIN32)
    target_link_libraries(chicken_potato_fps PRIVATE ws2_32)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(chicken_potato_fps PRIVATE m dl pthread rt X11)
endif()
