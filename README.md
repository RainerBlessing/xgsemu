# XGS Emulator

XGS Emulator is an emulator for the Parallax SX microcontroller, used in the XGameStation system.

## Building from Source

### Prerequisites

To build XGS Emulator, you need:

- CMake (version 3.10 or later)
- C++ compiler with C++11 support
- SDL library (version 1.2)
- wxWidgets library

### Build Instructions

1. Create a build directory:

```bash
mkdir build
cd build
```

2. Run CMake to configure the project:

```bash
# Standard Debug build (default)
cmake ..

# Or for Release (optimized) build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

3. Build the project:

```bash
cmake --build .
```

Or, if you're using make directly:

```bash
make
```

4. The executable will be created in the build directory.

### Additional Build Options

#### Clean Build

To clean the build directory:

```bash
make clean
```

For a complete cleanup, you can remove the build directory:

```bash
rm -rf build
```

#### Creating Distribution Archives

If you have Git installed, you can create distribution archives:

```bash
# Create a ZIP archive from the main branch
make git-archive

# Create a ZIP archive with version information
make git-archive-tagged

# Create a source tarball (.tar.gz)
make dist-tarball
```

The archives will be created in the build directory.

## Running the Emulator

After building, run the emulator with:

```bash
./xgsemu
```

To load a ROM file, use File > Load from the menu and select a .hex or .obj file.

## Controls

The emulator simulates the XGameStation controller with these keys:

- Arrow keys: Directional controls
- Ctrl: Fire button

## License

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation.
