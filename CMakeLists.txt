cmake_minimum_required(VERSION 3.24)

project(
  biopic
  VERSION 0.1.0
  DESCRIPTION "BioPic image moderation engine"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BIOPIC_BUILD_TESTS "Build unit and integration tests" ON)
option(BIOPIC_BUILD_BENCHMARKS "Build performance benchmarks" ON)
option(BIOPIC_BUILD_CLI "Build biopic CLI application" ON)
option(BIOPIC_BUILD_C_ABI "Build stable C ABI shared library" ON)
option(BIOPIC_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)

find_package(OpenCV REQUIRED COMPONENTS core imgcodecs imgproc)

add_library(biopic_core STATIC)
add_library(biopic::core ALIAS biopic_core)

target_sources(
  biopic_core
  PRIVATE src/core/error.cpp
          src/image/image_decoder.cpp
          src/hash/hash_parameters.cpp
          src/hash/biopic_hasher_v1.cpp
          src/hash/fingerprint_codec.cpp
          src/hash/fingerprint.cpp
          src/hash/distance.cpp
          src/ai/classifier.cpp
          src/ai/model.cpp
          src/ai/preprocessing.cpp)

target_include_directories(
  biopic_core
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>)

target_compile_definitions(
  biopic_core
  PUBLIC BIOPIC_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
         BIOPIC_VERSION_MINOR=${PROJECT_VERSION_MINOR}
         BIOPIC_VERSION_PATCH=${PROJECT_VERSION_PATCH})

target_link_libraries(biopic_core PUBLIC ${OpenCV_LIBS})

biopic_set_target_warnings(biopic_core)

if(MSVC)
  target_compile_options(biopic_core PRIVATE /fp:precise)
else()
  target_compile_options(biopic_core PRIVATE -fno-fast-math -ffp-contract=off)
endif()

if(BIOPIC_BUILD_C_ABI)
  # Static archive objects must be PIC when linked into libbiopic_c.so on Linux.
  set_target_properties(biopic_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

  add_library(biopic_c SHARED)
  add_library(biopic::c ALIAS biopic_c)

  target_sources(biopic_c PRIVATE bindings/c/src/biopic_c.cpp)
  target_include_directories(
    biopic_c
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bindings/c/include>
           $<INSTALL_INTERFACE:include>)
  target_link_libraries(biopic_c PRIVATE biopic_core)
  target_compile_definitions(biopic_c PRIVATE BIOPIC_C_EXPORTS)
  biopic_set_target_warnings(biopic_c)
endif()

if(BIOPIC_BUILD_CLI)
  add_executable(biopic-cli apps/biopic-cli/main.cpp)
  target_link_libraries(biopic-cli PRIVATE biopic_core)
  biopic_set_target_warnings(biopic-cli)
endif()

if(BIOPIC_BUILD_TESTS)
  enable_testing()
  find_package(GTest CONFIG REQUIRED)
  add_subdirectory(tests)
endif()

if(BIOPIC_BUILD_BENCHMARKS)
  find_package(benchmark CONFIG REQUIRED)
  add_subdirectory(benchmarks)
endif()
