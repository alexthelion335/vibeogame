cmake_minimum_required(VERSION 3.16)
project(ChickenPotatoFPS VERSION 1.2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Build metadata shown for each compiled binary
set(BUILD_PLATFORM "unknown")
if(ANDROID)
    set(BUILD_PLATFORM "android")
elseif(WIN32)
    set(BUILD_PLATFORM "windows")
elseif(APPLE)
    set(BUILD_PLATFORM "macos")
elseif(UNIX)
    set(BUILD_PLATFORM "linux")
endif()

set(BUILD_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
if(ANDROID)
    if(DEFINED CMAKE_ANDROID_ARCH_ABI AND NOT CMAKE_ANDROID_ARCH_ABI STREQUAL "")
        set(BUILD_ARCH "${CMAKE_ANDROID_ARCH_ABI}")
    elseif(DEFINED ANDROID_ABI AND NOT ANDROID_ABI STREQUAL "")
        set(BUILD_ARCH "${ANDROID_ABI}")
    endif()
endif()

message(STATUS "ChickenPotatoFPS target platform: ${BUILD_PLATFORM}")
message(STATUS "ChickenPotatoFPS target architecture: ${BUILD_ARCH}")
set(TARGET_OUTPUT_BASENAME "chicken_potato_fps_${BUILD_PLATFORM}_${BUILD_ARCH}")

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

    # Ensure NDK native_app_glue headers are available to game code
    set(NATIVE_APP_GLUE_DIR "")
    if(DEFINED ANDROID_NDK)
        set(NATIVE_APP_GLUE_DIR "${ANDROID_NDK}/sources/android/native_app_glue")
    elseif(DEFINED CMAKE_ANDROID_NDK)
        set(NATIVE_APP_GLUE_DIR "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")
    endif()
    if(NATIVE_APP_GLUE_DIR)
        target_include_directories(chicken_potato_fps PRIVATE "${NATIVE_APP_GLUE_DIR}")
    endif()
    
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

if(NOT ANDROID)
    set_target_properties(chicken_potato_fps PROPERTIES
        OUTPUT_NAME "${TARGET_OUTPUT_BASENAME}"
    )
endif()

add_custom_command(TARGET chicken_potato_fps POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "[ChickenPotatoFPS] Built: $<TARGET_FILE:chicken_potato_fps>"
    COMMAND ${CMAKE_COMMAND} -E echo "[ChickenPotatoFPS] Platform: ${BUILD_PLATFORM}, Architecture: ${BUILD_ARCH}"
)
