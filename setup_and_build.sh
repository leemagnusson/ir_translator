#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

echo "1. Checking for arduino-cli..."
if ! command -v arduino-cli &> /dev/null; then
    echo "arduino-cli not found. Installing..."
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
    export PATH=$PATH:$PWD/bin
fi

echo "2. Configuring Adafruit board URLs..."
# Initialize a config file if one doesn't exist
arduino-cli config init --overwrite
# Add the Adafruit package URL
arduino-cli config add board_manager.additional_urls https://adafruit.github.io/arduino-board-index/package_adafruit_index.json

echo "3. Updating board index and installing Adafruit AVR core..."
arduino-cli core update-index
arduino-cli core install adafruit:avr

echo "4. Installing required libraries..."
arduino-cli lib install "IRremote"

echo "5. Compiling the sketch for Adafruit Metro..."
# The FQBN (Fully Qualified Board Name) for the Adafruit Metro is adafruit:avr:metro
arduino-cli compile --fqbn adafruit:avr:metro .

echo "Build complete! To upload to the board, plug it in and run:"
echo "arduino-cli upload -p /dev/ttyUSB0 --fqbn adafruit:avr:metro ."
