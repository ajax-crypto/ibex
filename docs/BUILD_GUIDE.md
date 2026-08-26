# Build Configuration Guide

## System Requirements

### Windows (MSVC)
- Visual Studio 2022 or later
- C++23 support
- CMake 3.22+

### Linux (GCC)
- GCC 12 or later
- C++23 support (std=c++23 flag)
- CMake 3.22+
- Standard C++ library

## Quick Start

### Windows

```bash
cd c:\Dev\ibex
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

Binaries will be in `build\bin\`:
- `ibexc.exe` - Compiler
- `ibex_ide.exe` - IDE (when implemented)

### Linux

```bash
cd ~/Dev/ibex
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

Binaries will be in `build/bin/`:
- `ibexc` - Compiler
- `ibex_ide` - IDE (when implemented)

## Build Variants

### Debug Build (Development)

```bash
# Windows
cmake -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug ..

# Linux
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

### Release Build (Production)

```bash
# Windows
cmake -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release ..

# Linux
cmake -DCMAKE_BUILD_TYPE=Release ..
```

## CMake Options

### Disable IDE Build

```bash
cmake .. -DBUILD_IDE=OFF
```

### Disable Tests

```bash
cmake .. -DBUILD_TESTS=OFF
```

### Disable Examples

```bash
cmake .. -DBUILD_EXAMPLES=OFF
```

### Build Shared Libraries (default is static)

```bash
cmake .. -DBUILD_SHARED_LIBS=ON
```

### Combine Options

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_IDE=ON \
    -DBUILD_TESTS=ON \
    -DBUILD_EXAMPLES=ON
```

## Compiler-Specific Notes

### MSVC (Windows)

- C++23 is enabled through `/std:c++latest` (MSVC doesn't have stable C++23 yet)
- Parallel builds: `/MP` flag is automatically applied
- Warning level 4 with strict conformance: `/W4 /permissive-`

### GCC (Linux)

- C++23 enabled with `-std=c++2c` or `-std=c++23`
- Additional warnings: `-Wall -Wextra -Wpedantic`
- Optimization: Default is Release mode with `-O3`

## Verbose Build Output

To see compilation commands:

```bash
cmake --build . --verbose
```

Or with make:

```bash
make VERBOSE=1
```

## Building Individual Targets

### Compiler Only

```bash
cmake --build . --target ibexc
```

### Tests Only

```bash
cmake --build . --target test_lexer test_parser test_type_registry
```

### IDE Only

```bash
cmake --build . --target ibex_ide
```

### All Examples

```bash
cmake --build . --target example_hello
```

## Running Tests

After building:

```bash
# Windows
ctest --output-on-failure

# Linux
ctest --output-on-failure
```

Or run individual test executables:

```bash
./bin/test_lexer
./bin/test_parser
./bin/test_type_registry
```

## Using the Compiler

```bash
# Compile a program
./bin/ibexc program.ibex -o program.obj

# With verbose output
./bin/ibexc program.ibex -v
```

## Troubleshooting

### CMake Not Found

Make sure CMake is in your PATH:
- Windows: Install from https://cmake.org/download/
- Linux: `sudo apt install cmake` (Ubuntu/Debian)

### Missing C++23 Support

Ensure you have:
- Visual Studio 2022 or later (Windows)
- GCC 12+ or Clang 15+ (Linux)

If you get "C++23 not supported" errors:
- Update your compiler
- Or modify `CMakeLists.txt` to use C++20: `set(CMAKE_CXX_STANDARD 20)`

### Build Fails on Linux

Try cleaning and rebuilding:

```bash
cd build
rm -rf *
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --verbose
```

### Permission Denied (Linux)

If you get permission errors, ensure you have write access to the build directory:

```bash
chmod -R u+w build
```

## IDE Integration

### Visual Studio Code

Install C++ extension and configure:

```json
{
    "cmake.configureOnOpen": true,
    "cmake.generator": "Ninja",
    "cmake.buildDirectory": "${workspaceFolder}/build"
}
```

### Visual Studio

Open the generated solution file:
- `build/Ibex.sln`

### Qt Creator

- Open `CMakeLists.txt`
- Configure as new project
- Select build directory

### CLion

- Open project root
- CMake settings auto-detected
- Select build preset from configuration menu

## Platform-Specific Details

### Windows Paths

The build will create:
```
build/
├── bin/
│   ├── ibexc.exe
│   ├── ibex_ide.exe
│   └── test_*.exe
├── lib/
│   ├── ibex_compiler.lib
│   └── ibex_runtime.lib
└── CMakeFiles/
```

### Linux Paths

The build will create:
```
build/
├── bin/
│   ├── ibexc
│   ├── ibex_ide
│   └── test_*
├── lib/
│   ├── libibex_compiler.a
│   └── libibex_runtime.a
└── CMakeFiles/
```

## Advanced Configuration

### Custom Compiler Flags

Edit `CMakeLists.txt` or pass via command line:

```bash
cmake .. -DCMAKE_CXX_FLAGS="-O3 -march=native"
```

### Static Analysis

Enable compiler warnings:

```bash
cmake .. -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic"
```

### Link-Time Optimization (LTO)

```bash
cmake .. -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

## Installation

After building, install to a directory:

```bash
cmake --install . --prefix $HOME/ibex_install --config Release
```

This will create:
```
$HOME/ibex_install/
├── bin/
├── lib/
├── include/
└── share/
```

## Cleaning Build

Remove the build directory:

```bash
rm -rf build
```

Or with CMake:

```bash
cmake --build . --target clean
```

## Next Steps

1. Build the project using these instructions
2. Run `./bin/ibexc examples/hello.ibex -o hello.obj`
3. Check compiler output and error messages
4. Review test results with `ctest`
5. Start implementing missing components
