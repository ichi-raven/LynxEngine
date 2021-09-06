cmake_minimum_required(VERSION 3.11)

project(LynxEngine CXX)

add_definitions("-w -g -std=c++17")

include_directories(
   vulkan
   GLFW
   glm
   stb
   cutlass
   freetype
   portaudio
   "/usr/local/include/assimp"
   "./include"
)

file(GLOB_RECURSE SRCS src/*.cpp)

link_directories(../build)

add_executable(App ${SRCS})


target_link_libraries(App
   vulkan
   glfw
   cutlass
   portaudio
   "/usr/local/lib/libassimp.so"
)
