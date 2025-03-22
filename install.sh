#!/bin/bash
# Exit on error
set -e


if [ -f ~/ba67/bin/BA67 ]; then
    # use first screen
    # export DISPLAY=:0

    echo detect resolution
    # xrandr
    # echo RESOLUTION = $(xrandr | grep '*' | awk '{print $1}')
    # echo OUTPUT = $(xrandr | grep " connected" | awk '{print $1}')

    # Automatically detect the screen resolution
    # RESOLUTION=$(xrandr | grep '*' | awk '{print $1}')
    # OUTPUT=$(xrandr | grep " connected" | awk '{print $1}')
    
    # Apply the detected resolution
    # xrandr --output $OUTPUT --mode $RESOLUTION

    echo ~/ba67/bin/BA67 --fullscreen > ~/.config/openbox/autostart

    # startx ~/ba67/bin/BA67 --fullscreen --video opengl &
    startx /usr/bin/openbox-session &
fi

