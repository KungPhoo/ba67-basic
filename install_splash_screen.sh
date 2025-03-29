#!/bin/bash

# Define theme name
THEME_NAME="ba67-theme"

# Install required packages
apt update
apt install -y plymouth

# Create custom theme directory
mkdir -p /usr/share/plymouth/themes/$THEME_NAME

# Copy custom splash image
cp --force ~/ba68/resources/boot-800x480.png /usr/share/plymouth/themes/$THEME_NAME/splash.png

# Create the theme script
cat > /usr/share/plymouth/themes/$THEME_NAME/$THEME_NAME.plymouth <<EOL
[Plymouth Theme]
Name=ba68-theme
Description=BA67 Boot Splash
ModuleName=script

[script]
ImageDir=/usr/share/plymouth/themes/$THEME_NAME
ScriptFile=/usr/share/plymouth/themes/$THEME_NAME/$THEME_NAME.script
EOL

# Create the theme script file
cat > /usr/share/plymouth/themes/$THEME_NAME/$THEME_NAME.script <<EOL
wallpaper_image = Image("splash.png");
wallpaper_sprite = Sprite(wallpaper_image);
EOL

# Set permissions
chmod -R 755 /usr/share/plymouth/themes/$THEME_NAME

# Set the custom theme as default
plymouth-set-default-theme -R $THEME_NAME

# Update initramfs
update-initramfs -u

echo "Custom splash screen installed! Reboot to see the changes."
