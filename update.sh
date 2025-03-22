#!/bin/bash
# Exit on error
set -e

echo updating from sources...
chmod +x get_dependecies.sh
./get_dependecies.sh && echo "Updating GIT repository..."

echo do we need to clone?
cd ~
if [ ! -d ./ba67 ]; then
    echo Clone 1st time
    git -c user.name=JohnDoe -c user.email=me@privacy.net clone --recurse-submodules --remote-submodules https://github.com/KungPhoo/ba67-basic.git ba67
fi

echo cd into directory
cd ~/ba67

echo save and remove all manual changes
git stash

echo update sources
git fetch origin
git reset --hard origin/main
git pull origin main

echo updating from sources...
chmod +x build.sh
./build.sh && echo "Installing..."

chmod +x install.sh
./install.sh && echo "Starting..."

