cmake_minimum_required(VERSION 3.16)
project(ChickenPotatoFPS VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# raylib
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

if(ANDROID)
    set(PLATFORM "Android" CACHE STRING "" FORCE)
    # raylib's Android CMake uses ANDROID_NDK to locate native_app_glue
    if(NOT DEFINED ANDROID_NDK AND DEFINED CMAKE_ANDROID_NDK)
        set(ANDROID_NDK "${CMAKE_ANDROID_NDK}" CACHE PATH "Android NDK path" FORCE)
    endif()
endif()

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

# Android builds as shared library, others as executable
if(ANDROID)
    add_library(chicken_potato_fps SHARED src/main.cpp)
    
    # Android-specific linker flags
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
    
    # Link Android system libraries
    target_link_libraries(chicken_potato_fps PRIVATE 
        raylib 
        log 
        android 
        EGL 
        GLESv2 
        OpenSLES
        m
        c
    )
    
    # Define PLATFORM_ANDROID for conditional compilation
    target_compile_definitions(chicken_potato_fps PRIVATE PLATFORM_ANDROID)
else()
    add_executable(chicken_potato_fps src/main.cpp)
    target_link_libraries(chicken_potato_fps PRIVATE raylib)
    
    if(WIN32)
        target_link_libraries(chicken_potato_fps PRIVATE ws2_32)
    endif()
    
    if(UNIX AND NOT APPLE)
        target_link_libraries(chicken_potato_fps PRIVATE m dl pthread rt X11)
    endif()
endif()
