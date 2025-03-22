#!/bin/bash
# Exit on error
set -e

echo update system and install libraries
sudo apt-get update
sudo apt install git g++ cmake xserver-xorg xinit x11-xserver-utils openbox pulseaudio libpthread-stubs0-dev libasound2-dev libx11-dev libglew-dev -y

if [ ! -e /usr/lib/aarch64-linux-gnu/libpthread.so ]; then
    sudo ln -s /usr/lib/aarch64-linux-gnu/libpthread.so.0 /usr/lib/aarch64-linux-gnu/libpthread.so
    sudo ldconfig
fi

echo do we need to clone?
cd ~
if [ ! -d ./ba67 ]; then
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

