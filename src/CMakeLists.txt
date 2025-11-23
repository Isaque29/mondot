cmake_minimum_required(VERSION 3.10)
project(mondot LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

file(GLOB_RECURSE MONDOT_SOURCES
     "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(mondot ${MONDOT_SOURCES})
install(TARGETS mondot RUNTIME DESTINATION bin)
