cmake_minimum_required(VERSION 3.12.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_INCLUDE_CURRENT_DIR ON)

# ==========================================
# PROJECT CONFIGURATION
# ==========================================

set(BA67_SOUND_BACKEND "DEFAULT" CACHE STRING "Select feature mode: DEFAULT, NONE")
set_property(CACHE BA67_SOUND_BACKEND PROPERTY STRINGS "DEFAULT" "NONE")
if(${BA67_SOUND_BACKEND} STREQUAL "DEFAULT" )
add_compile_definitions(BA67_SOUND_BACKEND_DEFAULT)
else()
add_compile_definitions(BA67_SOUND_BACKEND_NONE)
endif()

set(BA67_GRAPHICS_CRT_EMULATION "ON" CACHE STRING "CRT emulation for graphics: ON, OFF")
set_property(CACHE BA67_GRAPHICS_CRT_EMULATION PROPERTY STRINGS "ON" "OFF")
if(${BA67_GRAPHICS_CRT_EMULATION} STREQUAL "ON" )
add_compile_definitions(BA67_GRAPHICS_CRT_EMULATION_ON)
else()
add_compile_definitions(BA67_GRAPHICS_CRT_EMULATION_OFF)
endif()

set(BA67_GRAPHICS_ENABLE_OPENGL "ON" CACHE STRING "Enable OpenGL: ON, OFF")
set_property(CACHE BA67_GRAPHICS_ENABLE_OPENGL PROPERTY STRINGS "ON" "OFF")
if(${BA67_GRAPHICS_ENABLE_OPENGL} STREQUAL "ON" )
add_compile_definitions(BA67_GRAPHICS_ENABLE_OPENGL_ON)
else()
add_compile_definitions(BA67_GRAPHICS_ENABLE_OPENGL_OFF)
endif()

set(BA67_GRAPHICS_ENABLE_EMOJI "ON" CACHE STRING "Enable Emoji font: ON, OFF")
set_property(CACHE BA67_GRAPHICS_ENABLE_EMOJI PROPERTY STRINGS "ON" "OFF")
if(${BA67_GRAPHICS_ENABLE_EMOJI} STREQUAL "ON" )
add_compile_definitions(BA67_GRAPHICS_ENABLE_EMOJI)
else()
add_compile_definitions(BA67_GRAPHICS_ENABLE_EMOJI)
endif()

# Minimum required variables/pathes
set(APP_NAME              "BA67") # only set here. Better not change it, then.
message("===========================================================")
message("=== ${APP_NAME} ===")
message("===========================================================")

if (WIN32)
    project(${APP_NAME} LANGUAGES CXX C RC)
else()
    project(${APP_NAME} LANGUAGES CXX C)
endif()

set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug>:EditAndContinue>")

add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1)
add_compile_definitions(_UNICODE=1 UNICODE=1) # Windows needs this

# CMake defines NDEBUG for release builds
add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG -D_DEBUG>")

# Hide all warnings for release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if (MSVC)
        add_compile_options(/W0)
    else()
        add_compile_options(-w)
    endif()
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin")




# === App Source ===
file(GLOB APP_SOURCES # cpp files
    "src/*.cpp"
    )
file(GLOB APP_HEADERS # headers
    "src/*.h"
    )

file(GLOB_RECURSE SOLOUD_CPP 
    ./soloud/src/audiosource/*.cpp
    ./soloud/src/audiosource/*.c
    ./soloud/src/backend/miniaudio/*.cpp
    ./soloud/src/core/*.cpp
    ./soloud/src/filter/*.cpp
    )
file(GLOB SOLOUD_H 
    ./soloud/include/*.h
    )

add_compile_definitions(WITH_MINIAUDIO=1)
add_library(lib_soloud OBJECT "${SOLOUD_CPP}")





# FPL takes care of wWinMain and calls main
add_executable(${APP_NAME} "${APP_SOURCES}" "${APP_HEADERS}")
# target_compile_features(${APP_NAME} PUBLIC cxx_std_20)
target_link_libraries(${APP_NAME} PUBLIC lib_soloud)

if (WIN32)
    #################### ICON ################
    add_library(windows_resource_lib OBJECT "resources/resource.rc")
    target_link_libraries(${APP_NAME} PUBLIC windows_resource_lib)
endif()

# OpenGL
if(${BA67_GRAPHICS_ENABLE_OPENGL} STREQUAL "ON" )
find_package(OpenGL)
TARGET_LINK_LIBRARIES(${APP_NAME} PRIVATE OpenGL::GL)
endif()

# bluetooth on Linux
# if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
#     target_link_libraries(${APP_NAME} PRIVATE bluetooth)
# endif()

add_test(NAME TestVersion  COMMAND $<TARGET_FILE:${APP_NAME}> --version) # dummy test for QtCreator

include_directories(AFTER ./src)
include_directories(AFTER ./final_platform_layer)
include_directories(AFTER ./soloud/include)

# precompiled headers (CMake 3.16.0-RC2 and greater)
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
    target_precompile_headers(${APP_NAME} PUBLIC src/stdafx.h "$<$<COMPILE_LANGUAGE:CXX>:cxx_only.h>")
endif()


# Unicode build (for Windows runtime)
add_compile_definitions(UNICODE=1 _UNICODE=1)
if (MSVC)
    # add_compile_options("/bigobj") add_compile_options -> does not seem to work. Must use CMAKE_CXX_FLAGS
    # add_compile_options("/MP")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj") # Increase Number of Sections in .Obj file
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP") # multi-processor compiling
endif()

# DEBUG and _DEBUG preprocessor defines. NDEBUG is set for release mode by CMake.
if( ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug") OR ("${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG"))
    add_compile_definitions(DEBUG=1 _DEBUG=1)
endif()


# find_package() can use cmake/FindXXXX.cmake
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")


# OpenMP (parallel computing)
find_package(OpenMP)

if(OpenMP_CXX_FOUND)
    target_link_libraries(${APP_NAME} PUBLIC OpenMP::OpenMP_CXX)
endif()

