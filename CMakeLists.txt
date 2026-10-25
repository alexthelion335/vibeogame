cmake_minimum_required(VERSION 3.16)
project(ChickenPotatoFPS VERSION 1.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

# Native desktop builds use the platform's libcurl package. Cross builds must
# not accidentally link the host libcurl, so build a small static curl for the
# target architecture on Android and arm64 Linux.
set(CURL_FOUND FALSE)
if(ANDROID OR (CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND
               CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$"))
    set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(USE_STATIC_MBEDTLS_LIBRARY ON CACHE BOOL "" FORCE)
    set(USE_SHARED_MBEDTLS_LIBRARY OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        mbedtls
        GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls.git
        GIT_TAG mbedtls-3.6.7
        GIT_SHALLOW TRUE
        GIT_SUBMODULES_RECURSE TRUE
    )
    FetchContent_MakeAvailable(mbedtls)

    # Let curl's FindMbedTLS module consume the Mbed TLS targets already
    # created above instead of probing the host with pkg-config.
    set(MBEDTLS_FOUND TRUE)
    set(MBEDTLS_VERSION "3.6.7")
    set(MBEDTLS_INCLUDE_DIR "${mbedtls_SOURCE_DIR}/include;${mbedtls_BINARY_DIR}/include")
    set(MBEDTLS_LIBRARY MbedTLS::mbedtls)
    set(MBEDX509_LIBRARY MbedTLS::mbedx509)
    set(MBEDCRYPTO_LIBRARY MbedTLS::mbedcrypto)
    # curl's optional DES availability check invokes a standalone try_compile
    # project, where these in-tree Mbed TLS targets are not defined.
    set(HAVE_MBEDTLS_DES_CRYPT_ECB 0 CACHE INTERNAL "" FORCE)

    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(CURL_ENABLE_SSL ON CACHE BOOL "" FORCE)
    set(CURL_USE_MBEDTLS ON CACHE BOOL "" FORCE)
    set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
    set(CURL_USE_PKGCONFIG OFF CACHE BOOL "" FORCE)
    set(CURL_USE_CMAKECONFIG OFF CACHE BOOL "" FORCE)
    set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
    set(CURL_USE_LIBSSH2 OFF CACHE BOOL "" FORCE)
    set(CURL_ZLIB OFF CACHE BOOL "" FORCE)
    set(CURL_BROTLI OFF CACHE BOOL "" FORCE)
    set(CURL_ZSTD OFF CACHE BOOL "" FORCE)
    set(USE_LIBIDN2 OFF CACHE BOOL "" FORCE)
    set(USE_NGHTTP2 OFF CACHE BOOL "" FORCE)
    set(CURL_DISABLE_LDAP ON CACHE BOOL "" FORCE)
    if(ANDROID)
        # Android stores system root certificates in this hashed directory.
        set(CURL_CA_PATH "/system/etc/security/cacerts" CACHE STRING "" FORCE)
    endif()

    FetchContent_Declare(
        curl
        GIT_REPOSITORY https://github.com/curl/curl.git
        GIT_TAG curl-8_23_0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(curl)
    set(CURL_FOUND TRUE)
elseif(NOT ANDROID)
    find_package(CURL QUIET)
endif()

# Build metadata shown for each compiled binary
set(BUILD_PLATFORM "unknown")
if(EMSCRIPTEN)
    set(BUILD_PLATFORM "web")
elseif(ANDROID)
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

if(EMSCRIPTEN)
    set(PLATFORM "Web" CACHE STRING "" FORCE)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
elseif(ANDROID)
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
    if(CURL_FOUND)
        target_link_libraries(chicken_potato_fps PRIVATE CURL::libcurl)
        target_compile_definitions(chicken_potato_fps PRIVATE HAS_CURL)
        message(STATUS "ChickenPotatoFPS: target libcurl found, latest-version checks enabled")
    else()
        message(STATUS "ChickenPotatoFPS: target libcurl unavailable, latest-version checks disabled")
    endif()
    
    # Define PLATFORM_ANDROID for conditional compilation
    target_compile_definitions(chicken_potato_fps PRIVATE PLATFORM_ANDROID)
else()
    add_executable(chicken_potato_fps src/main.cpp)
    target_link_libraries(chicken_potato_fps PRIVATE raylib)
    target_compile_definitions(chicken_potato_fps PRIVATE APP_VERSION_STR="${PROJECT_VERSION}")

    if(CURL_FOUND)
        target_link_libraries(chicken_potato_fps PRIVATE CURL::libcurl)
        target_compile_definitions(chicken_potato_fps PRIVATE HAS_CURL)
        message(STATUS "ChickenPotatoFPS: libcurl found, latest-version checks enabled")
    else()
        message(STATUS "ChickenPotatoFPS: libcurl not found, latest-version checks disabled")
    endif()
    
    if(EMSCRIPTEN)
        # Emscripten-specific settings for web builds
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_GLFW=3")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_GLFW=3")
        set_target_properties(chicken_potato_fps PROPERTIES 
            LINK_FLAGS "-s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s ALLOW_MEMORY_GROWTH=1 --preload-file ${CMAKE_SOURCE_DIR}/highscores.txt@highscores.txt"
        )
        # Define PLATFORM_WEB for conditional compilation
        target_compile_definitions(chicken_potato_fps PRIVATE PLATFORM_WEB)
    elseif(WIN32)
        target_link_libraries(chicken_potato_fps PRIVATE ws2_32)
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(chicken_potato_fps PRIVATE m dl pthread rt X11)
    endif()
endif()

if(ANDROID)
    target_compile_definitions(chicken_potato_fps PRIVATE APP_VERSION_STR="${PROJECT_VERSION}")
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
