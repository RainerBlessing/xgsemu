#!/bin/sh
# Simple build script for XGS Emulator

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. || {
    echo "CMake configuration failed!"
    exit 1
}

# Build the project
echo "Building XGS Emulator..."
cmake --build . || {
    echo "Build failed!"
    exit 1
}

echo "Build successful! The executable is in the build directory."
echo "Run with: ./xgsemu"
