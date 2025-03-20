#!/bin/bash
# Exit on error
set -e

sudo apt-get update
sudo apt install git g++ cmake pulseaudio libasound2-dev libx11-dev

cd ~
if [ ! -d ~/ba68 ] ; then
    git -c user.name=JohnDoe -c user.email=me@privacy.net clone --recursive-submodules --remote-submodules https://github.com/KungPhoo/ba67-basic.git ba67
fi

cd ~/ba68

# remove all manual changes
git reset
git checkout .
git clean -fdx

# update sources
git -c user.name=JohnDoe -c user.email=me@privacy.net -c pull.rebase=true pull origin main

# updating from sources...
chmod +x build.sh
./build.sh


