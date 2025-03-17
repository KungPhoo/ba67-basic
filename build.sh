#!/bin/bash

# This script sets up and builds the project on Linux/macOS

# Exit on error
set -e

# Create build directory if it doesn't exist
mkdir -p out/build
pushd out/build

# Run CMake to configure the project
cmake -DCMAKE_BUILD_TYPE=Release ../..

# Build the project
make

popd

# Optionally install the project
# sudo make install

mkdir ~/BASIC
cp --force --recursive --update ./examples/ ~/BASIC/examples/

