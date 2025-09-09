#!/bin/bash

# install BA67 to autostart when booting.
# This is still a WIP.


# Exit on error
set -e


if [ -f ~/ba67/bin/BA67 ]; then
    # install program
    sudo cp  --force ~/ba67/bin/BA67 /usr/local/bin/BA67

    echo installing autostart of BA67 for openbox
    mkdir -p ~/.config/openbox
    echo '/usr/local/bin/BA67 --fullscreen --video opengl' > ~/.config/openbox/autostart
    chmod +x ~/.config/openbox/autostart

    echo installing autostart of openbox in .bash_profile
    echo 'if [ -z "$DISPLAY" ] && [ "$(tty)" = "/dev/tty1" ]; then' > ~/.bash_profile
    echo '    startx /usr/bin/openbox-session &'                    >> ~/.bash_profile
    echo 'fi'                                                       >> ~/.bash_profile
    


    # install terminal colors
    sudo cp --force vt_colors.sh /usr/local/bin/vt_colors.sh
    sudo chmod +x /usr/local/bin/vt_colors.sh
    sudo cp --force vt_colors.service /etc/systemd/system/vt_colors.service
    sudo systemctl daemon-reload
    sudo systemctl enable vt_colors.service
    sudo systemctl start  vt_colors.service
fi

