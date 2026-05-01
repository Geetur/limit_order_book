cmake_minimum_required(VERSION 3.23)

project(lob-engine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3") # Max optimization
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native") # Enable all warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto") # Enable link-time optimization
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror") # Enable all warnings

# Make the include directory public
include_directories(include)
# Create an executable for testing purposes
add_executable(engine_main apps/engine_main.cpp)
#Link POSIX threads for core pinning
target_link_libraries(engine_main pthread)
