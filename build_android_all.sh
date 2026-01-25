#!/usr/bin/env bash

# Get the directory where this script is located
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Change the working directory to that location
cd "$SCRIPT_DIR"

cd Common && ./compile_android.sh
cd ..
cd GraphicAPIBroker && ./compile_android_vulkan_1_3.sh
cd ..
cd "Wolf-Engine-2.0" && ./compile_android_vulkan.sh
