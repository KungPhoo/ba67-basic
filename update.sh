#!/bin/bash
# Exit on error
set -e

if [ ! -f ~/ba67/get_dependecies.sh ]; then
    echo "ensure we have git"
    sudo apt-get update
    sudo apt install -y git
fi

echo "do we need to clone?"
cd ~
if [ ! -d ./ba67 ]; then
    echo Clone 1st time
    git -c user.name=JohnDoe -c user.email=me@privacy.net clone --recurse-submodules --remote-submodules https://github.com/KungPhoo/ba67-basic.git ba67
fi

echo "cd into directory"
cd ~/ba67

echo save and remove all manual changes
git stash

echo update sources
git fetch origin
git reset --hard origin/main
git pull --recurse-submodules origin main


if [ -f ~/ba67/get_dependencies.sh ]; then
    echo "Updating dependencies"
    chmod +x get_dependencies.sh
    ./get_dependencies.sh && echo .
fi

echo "Building from sources..."
chmod +x build.sh
./build.sh && echo .

# echo "Installing..."
# chmod +x install.sh
# ./install.sh && echo .

# echo "Starting..."

echo "end of update"