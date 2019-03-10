cmake_minimum_required(VERSION 3.13)
project(voxToPng)

set(CMAKE_CXX_STANDARD 14)

add_definitions (-D_DEBUG)

add_executable(voxToPng main.cpp scene.cpp arguments.cpp loader.cpp palette.cpp model.cpp dictionary.cpp node.cpp node_group.cpp node_shape.cpp node_transform.cpp)