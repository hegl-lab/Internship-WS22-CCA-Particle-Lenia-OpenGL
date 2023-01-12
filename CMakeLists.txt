cmake_minimum_required(VERSION 3.22)
project(particle_lenia)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src/glfw-abstraction)

# abstractions used to interact with opengl
file(GLOB glfw-abstraction_SRC "src/glfw-abstraction/*.cpp")
add_library(glfw-abstraction ${glfw-abstraction_SRC})