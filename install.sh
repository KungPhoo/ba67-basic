#!/bin/bash
# Exit on error
set -e


if [ -f ~/ba67/bin/BA67 ]; then
    # use first screen
    # export DISPLAY=:0

    # echo detect resolution
    # xrandr
    # echo RESOLUTION = $(xrandr | grep '*' | awk '{print $1}')
    # echo OUTPUT = $(xrandr | grep " connected" | awk '{print $1}')

    # Automatically detect the screen resolution
    # RESOLUTION=$(xrandr | grep '*' | awk '{print $1}')
    # OUTPUT=$(xrandr | grep " connected" | awk '{print $1}')
    
    # Apply the detected resolution
    # xrandr --output $OUTPUT --mode $RESOLUTION

    echo installing autostart of BA67 for openbox
    mkdir -p ~/.config/openbox
    echo ~/ba67/bin/BA67 --fullscreen > ~/.config/openbox/autostart
    chmod +x ~/.config/openbox/autostart

    echo installing autostart of openbox in .bash_profile
    echo if [ -z "$DISPLAY" ] && [ "$(tty)" = "/dev/tty1" ]; then > ~/.bash_profile
    echo     startx /usr/bin/openbox-session &                    > ~/.bash_profile
    echo fi                                                       > ~/.bash_profile
    
    # startx /usr/bin/openbox-session &
fi

