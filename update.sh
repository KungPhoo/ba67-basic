#!/bin/bash
# Exit on error
set -e

sudo apt-get update
sudo apt install git g++ cmake pulseaudio libasound2-dev libx11-dev libglew-dev 

cd ~
if [ ! -d ./ba68 ] ; then
    git -c user.name=JohnDoe -c user.email=me@privacy.net clone --recurse-submodules --remote-submodules https://github.com/KungPhoo/ba67-basic.git ba67
fi

cd ~/ba68

# save and remove all manual changes
git stash

# update sources
git fetch origin
git reset --hard origin/main

# updating from sources...
chmod +x build.sh
./build.sh


