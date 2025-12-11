cmake_minimum_required(VERSION 3.10)
project(mondot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MONDOT_DEBUG "Enable debug logging (defines MONDOT_DEBUG)" OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

file(GLOB_RECURSE MONDOT_SOURCES
     "${SRC_DIR}/*.cpp"
)

add_executable(mondot ${MONDOT_SOURCES})

target_include_directories(mondot PRIVATE "${SRC_DIR}")

if (MSVC)
  target_compile_options(mondot PRIVATE /W4 /permissive-)
else()
  target_compile_options(mondot PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

if(MONDOT_DEBUG)
  target_compile_definitions(mondot PRIVATE MONDOT_DEBUG=1)
endif()

if (NOT MSVC)
  target_compile_options(mondot PRIVATE $<$<CONFIG:Release>:-O3>)
else()
  target_compile_options(mondot PRIVATE $<$<CONFIG:Release>:/O2>)
endif()

if (NOT MSVC)
  target_compile_options(mondot PRIVATE $<$<CONFIG:Debug>:-g>)
endif()

install(TARGETS mondot RUNTIME DESTINATION bin)
