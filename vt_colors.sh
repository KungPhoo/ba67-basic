#!/bin/bash

# Define the color palette (hex values for the closest VT colors)
echo -en "\e]P0000000"  # Black
echo -en "\e]P196282e"  # Red
echo -en "\e]P241b936"  # Green
echo -en "\e]P39f4815"  # Orange
echo -en "\e]P42724c4"  # Blue
echo -en "\e]P55e3500"  # Brown
echo -en "\e]P65bd6ce"  # Cyan
echo -en "\e]P7aeaeae"  # Light Gray

echo -en "\e]P8474747"  # Dark Gray
echo -en "\e]P9da5f66"  # Light Red
echo -en "\e]PA91ff84"  # Light Green
echo -en "\e]PBeff347"  # Yellow
echo -en "\e]PC6864ff"  # Light Blue
echo -en "\e]PD9f2dad"  # Purple
echo -en "\e]PE787878"  # Medium Gray
echo -en "\e]PFffffff"  # White

# X window terminal window colors
echo "*.color0: #000000"  >  ~/.Xresources
echo "*.color1: #96282e"  >> ~/.Xresources
echo "*.color2: #41b936"  >> ~/.Xresources
echo "*.color3: #9f4815"  >> ~/.Xresources
echo "*.color4: #2724c4"  >> ~/.Xresources
echo "*.color5: #5e3500"  >> ~/.Xresources
echo "*.color6: #5bd6ce"  >> ~/.Xresources
echo "*.color7: #aeaeae"  >> ~/.Xresources
echo "*.color8: #474747"  >> ~/.Xresources
echo "*.color9: #da5f66"  >> ~/.Xresources
echo "*.color10: #91ff84"  >> ~/.Xresources
echo "*.color11: #eff347"  >> ~/.Xresources
echo "*.color12: #6864ff"  >> ~/.Xresources
echo "*.color13: #9f2dad"  >> ~/.Xresources
echo "*.color14: #787878"  >> ~/.Xresources
echo "*.color15: #ffffff"  >> ~/.Xresources
echo "*.foreground: #ffffff"  >> ~/.Xresources
echo "*.backgound: #474747"  >> ~/.Xresources
echo "*.reverseVideo: off"  >> ~/.Xresources

# echo "Testing Terminal Colors with Escape Codes:" 
# echo -e "\e[30m ESC[30m (Black) \e[0m"
# echo -e "\e[31m ESC[31m (Red) \e[0m"
# echo -e "\e[32m ESC[32m (Green) \e[0m"
# echo -e "\e[33m ESC[33m (Yellow) \e[0m"
# echo -e "\e[34m ESC[34m (Blue) \e[0m"
# echo -e "\e[35m ESC[35m (Purple/Magenta) \e[0m"
# echo -e "\e[36m ESC[36m (Cyan) \e[0m"
# echo -e "\e[37m ESC[37m (White) \e[0m"
# echo ""
# echo -e "\e[90m ESC[90m (Bright Black/Gray) \e[0m"
# echo -e "\e[91m ESC[91m (Bright Red) \e[0m"
# echo -e "\e[92m ESC[92m (Bright Green) \e[0m"
# echo -e "\e[93m ESC[93m (Bright Yellow) \e[0m"
# echo -e "\e[94m ESC[94m (Bright Blue) \e[0m"
# echo -e "\e[95m ESC[95m (Bright Purple/Magenta) \e[0m"
# echo -e "\e[96m ESC[96m (Bright Cyan) \e[0m"
# echo -e "\e[97m ESC[97m (Bright White) \e[0m"

# Set background to Dark Gray and text to Light Green
export PS1="\033[92m$PWD:\033[97m"

clear
echo -e "\e[92m"
echo " _____ _____   ___  ___ "
echo "| __  |  _  | |  _||_  |"
echo "| __ -|     | | . |  | |"
echo "|_____|__|__| |___|  |_|"
echo "         B A S I C"
echo -e "\e[97m"



