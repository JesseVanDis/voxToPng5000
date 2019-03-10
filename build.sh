#!/bin/bash
cd "$(dirname "$0")"

cd ./src
g++ -O2 main.cpp arguments.cpp dictionary.cpp loader.cpp model.cpp node.cpp node_group.cpp node_shape.cpp node_transform.cpp palette.cpp scene.cpp -o ../voxToPng
cd ../

