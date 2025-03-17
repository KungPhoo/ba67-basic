#!/bin/bash
# Exit on error
set -e

# update sources
git pull origin main

# build release
./build.sh


