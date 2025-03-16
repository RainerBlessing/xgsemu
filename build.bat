@echo off
REM Simple build script for XGS Emulator on Windows

REM Create build directory if it doesn't exist
if not exist build mkdir build
cd build

REM Configure with CMake
echo Configuring with CMake...
cmake .. -G "MinGW Makefiles" || (echo CMake configuration failed! && exit /b 1)

REM Build the project
echo Building XGS Emulator...
cmake --build . || (echo Build failed! && exit /b 1)

echo Build successful! The executable is in the build directory.
echo Run with: xgsemu.exe
