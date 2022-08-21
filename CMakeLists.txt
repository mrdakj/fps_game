cmake_minimum_required (VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DFPS_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

project(OpenGL)

include_directories(include)
add_subdirectory(src)
