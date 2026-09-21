cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Disable online update step for FetchContent" FORCE)

project(BetterVisualandAudio)

# nontype_functional (pulled in by the Geode SDK) uses type traits that are
# not transitively available with the Android NDK's libc++. Ensure its headers
# see them when compiling Android dependencies.
if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_compile_options(-include type_traits)
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (MSVC)
    # Avoid an MSVC 18.x whole-program optimization crash during the final
    # "Generating Code" pass. Geode does not require LTCG for a mod DLL.
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION FALSE
    )
    target_compile_options(${PROJECT_NAME} PRIVATE "/GL-" "/Od")
endif()

# MSVC can crash while optimizing the very large embedded SMAA GLSL source.
# Keep that translation unit unoptimized on Windows; the shader itself is
# compiled by OpenGL at runtime, so this does not reduce shader quality.
if (MSVC)
    set_source_files_properties(src/shaders/aa/SmaaShader.cpp PROPERTIES
        COMPILE_OPTIONS "/Od;/bigobj"
    )
endif()

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})
