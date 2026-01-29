#!/usr/bin/env bash

# Get the directory where this script is located
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Change the working directory to that location
cd "$SCRIPT_DIR"

./gradlew assembleDebug || exit 1
