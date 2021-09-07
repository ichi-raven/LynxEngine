cmake_minimum_required(VERSION 3.11)

project(LynxEngine CXX)

set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra -pedantic -Wcast-align -Wcast-qual -Wconversion -Wdisabled-optimization -Wendif-labels -Wfloat-equal -Winit-self -Winline -Wlogical-op -Wmissing-include-dirs -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wpacked -Wpointer-arith -Wredundant-decls -Wshadow -Wsign-promo -Wswitch-default -Wswitch-enum -Wunsafe-loop-optimizations -Wvariadic-macros -Wwrite-strings ")
set(CMAKE_CXX_FLAGS_DEBUG "-std=c++17 -g3 -O0 -pg")
set(CMAKE_CXX_FLAGS_RELEASE "-std=c++17 -O2 -s -DNDEBUG -march=native")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-std=c++17 -g3 -Og -pg")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-std=c++17 -Os -s -DNDEBUG -march=native")

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
file(GLOB_RECURSE HDRS include/*.hpp)


add_library(
   lynx STATIC
   ${SRCS}
)


target_link_libraries(lynx
   vulkan
   glfw
   cutlass
   portaudio
   "/usr/local/lib/libassimp.so"
)

install(TARGETS lynx ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
install(DIRECTORY include/Lynx DESTINATION include/)

