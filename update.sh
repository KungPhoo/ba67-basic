#!/bin/bash
# Exit on error
set -e

# update sources
git pull origin main

# build release
chmod +x build.sh
./build.sh


