cmake_minimum_required(VERSION 3.12.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_INCLUDE_CURRENT_DIR ON)



# ==========================================
# PROJECT CONFIGURATION
# ==========================================


# Minimum required variables/pathes
set(APP_NAME              "BA67") # only set here. Better not change it, then.
message("===========================================================")
message("=== ${APP_NAME} ===")
message("===========================================================")

IF (WIN32)
    project(${APP_NAME} LANGUAGES CXX C RC)
ELSE()
    project(${APP_NAME} LANGUAGES CXX C)
ENDIF()

set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug>:EditAndContinue>")

add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin")

IF (WIN32)
    #################### ICON ################
    add_library(windows_resource_lib OBJECT "resources/resource.rc")
ENDIF()

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

IF (WIN32)
    add_executable(${APP_NAME} WIN32 "${APP_SOURCES}" "${APP_HEADERS}" $<TARGET_OBJECTS:windows_resource_lib>)
ELSE()
    add_executable(${APP_NAME} WIN32 "${APP_SOURCES}" "${APP_HEADERS}")
ENDIF()
target_link_libraries(${APP_NAME} PUBLIC lib_soloud)


add_test(NAME TestVersion  COMMAND $<TARGET_FILE:${APP_NAME}> --version) # dummy test for QtCreator

include_directories(AFTER ./src)
include_directories(AFTER ./final_platform_layer)
include_directories(AFTER ./soloud/include)


# Console Window: GCC flags: -mconsole
set_target_properties(${APP_NAME} PROPERTIES LINK_FLAGS -mconsole)
target_link_options(${APP_NAME} PRIVATE "/SUBSYSTEM:CONSOLE")


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


# CMake defines NDEBUG for release builds
# add_compile_options("$<$<CONFIG:Debug>:-DDEBUG>")  # #ifdef DEBUG
# add_compile_options("$<$<CONFIG:Debug>:-D_DEBUG>") # #ifdef _DEBUG <-- that's what Fides used so far

# add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG -D_DEBUG>")

# OpenMP (parallel computing)
find_package(OpenMP)

if(OpenMP_CXX_FOUND)
    target_link_libraries(${APP_NAME} PUBLIC OpenMP::OpenMP_CXX)
endif()

