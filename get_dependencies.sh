#!/bin/bash
# Exit on error
set -e

echo update system and install runtime libraries
sudo apt-get update

# runtime dependencies
sudo apt install -y git g++ cmake make ninja-build xserver-xorg xinit x11-xserver-utils alsa-utils openbox python3-xdg pulseaudio fluidsynth abcmidi pi-bluetooth bluez bluez-tools

# get dev packages
sudo apt install -y libpthread-stubs0-dev libasound2-dev libx11-dev libglew-dev libbluetooth-dev libcurl4-openssl-dev

if [ ! -e /usr/lib/aarch64-linux-gnu/libpthread.so ]; then
    sudo ln -s /usr/lib/aarch64-linux-gnu/libpthread.so.0 /usr/lib/aarch64-linux-gnu/libpthread.so
    sudo ldconfig
fi
