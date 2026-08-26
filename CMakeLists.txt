cmake_minimum_required(VERSION 3.22)

# Project definition with C++23 requirement
project(Ibex 
    VERSION 0.1.0
    DESCRIPTION "A native, non-garbage-collected language with integrated IDE"
    LANGUAGES CXX C
)

# Set C++ standard to 23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Determine compiler-specific flags
if(MSVC)
    # MSVC compiler flags for Windows
    add_compile_options(
        /W4              # Warning level 4
        /permissive-     # Strict C++ conformance
        /fp:fast         # Fast floating point
    )
    # Enable parallel builds on Windows
    add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)
else()
    # GCC/Clang compiler flags for Linux
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wunused
    )
    # Optimize by default
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release)
    endif()
    
    # Fast math
    add_compile_options(-ffast-math)
endif()

# Build options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_IDE "Build IDE component" ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Dependencies
include(FetchContent)

# Add yyjson library
FetchContent_Declare(
    yyjson
    URL https://github.com/ibireme/yyjson/archive/refs/tags/0.12.0.tar.gz
    FIND_PACKAGE_ARGS NAMES yyjson
)
FetchContent_MakeAvailable(yyjson)

# Include directories
include_directories(BEFORE ${PROJECT_SOURCE_DIR}/include)

# Compiler library
add_subdirectory(src)

# Compiler executable
add_subdirectory(compiler)

# Runtime library
add_subdirectory(runtime)

# IDE (if enabled)
if(BUILD_IDE)
    add_subdirectory(ide)
endif()

# Tests (if enabled)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples (if enabled)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Print configuration summary
message(STATUS "===============================================")
message(STATUS "Ibex Language Configuration:")
message(STATUS "  Generator: ${CMAKE_GENERATOR}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build IDE: ${BUILD_IDE}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
message(STATUS "  Build Examples: ${BUILD_EXAMPLES}")
message(STATUS "===============================================")
