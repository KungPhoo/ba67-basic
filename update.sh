#!/bin/bash
# Exit on error
set -e

echo update system and install libraries
sudo apt-get update
sudo apt install git g++ cmake pulseaudio libasound2-dev libx11-dev libglew-dev 

echo do we need to clone?
cd ~
if [ ! -d ./ba68 ] ; then
    git -c user.name=JohnDoe -c user.email=me@privacy.net clone --recurse-submodules --remote-submodules https://github.com/KungPhoo/ba67-basic.git ba67
fi

echo cd into directory
cd ~/ba68

echo save and remove all manual changes
git stash

echo update sources
git fetch origin
git reset --hard origin/main

echo updating from sources...
chmod +x build.sh
./build.sh


