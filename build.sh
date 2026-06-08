#!/bin/sh

# This script sets up and builds the project on Linux/Mac

# Exit on error
set -e

mkdir -p ~/ba67/bin
rm -f ~/ba67/bin/BA67

# Create build directory if it doesn't exist
mkdir -p ~/ba67/out/build
cd ~/ba67/out/build

# Run CMake to configure the project
cmake -DCMAKE_BUILD_TYPE=Release ../..

# Build the project
make clean
make

cd ~/ba67

# Optionally install the project
# sudo make install
if [ ! -d ~/BASIC ]; then
    mkdir ~/BASIC
fi
cp --force --recursive --update ~/ba67/examples/ ~/BASIC/examples/

chmod +x ~/ba67/bin/BA67
echo build finished

cp --force ~/ba67/bin/BA67 /opt/BA67

if [ -d ~/.local/bin ]; then
    ln -s /opt/BA67 ~/.local/bin/BA67
else
    ln -s /opt/BA67 /usr/local/BA67
fi
