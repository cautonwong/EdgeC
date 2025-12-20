# Getting Started with the C-EdgeX Boilerplate

## 1. Introduction

Welcome! This repository is a professional boilerplate for building robust, long-term, and reproducible C applications, especially for edge computing and IoT. It is designed around several core engineering principles:

*   **Platform/Product Separation**: The code is split into a stable `platform` (containing the core SDK, drivers, and middleware) and agile `products` (containing specific business logic).
*   **Reproducible Builds**: The entire build environment (compiler, libraries, tools) is containerized using **Docker**. This guarantees that a specific version of your code will produce the exact same binary, today or 3 years from now.
*   **Modern CMake**: We use `CMakePresets.json` to provide a single, standardized way to configure and build the project for different platforms (x64, ARM64) and configurations (Debug, Release).
*   **CI/CD Ready**: A complete GitHub Actions workflow is included in `.github/workflows/ci.yml` that automatically builds, tests, and analyzes the code.

## 2. Prerequisites

You only need one piece of software installed on your host machine:

*   **Docker Engine**

All other tools (GCC, CMake, Ninja, etc.) are provided inside the build container.

## 3. Directory Structure

```
.
├── apps/                  # "Product" applications that consume the SDK
│   └── sim_sensor_app/
├── platform/              # "Platform" containing all core, stable assets
│   ├── sdk/               # The core C-EdgeX SDK (builds as a library)
│   ├── tests/             # Tests for the platform SDK
│   └── external/          # Vendored third-party dependencies (cJSON, cmocka)
├── build_env/             # Dockerfiles defining the build environments
├── build.sh               # The main script for building the project
└── CMakeLists.txt         # Top-level CMake file
```

## 4. Building the Project

All builds are performed via the `build.sh` script, which automatically handles the Docker environment. You do not need to run `cmake` or `docker` commands manually.

The build configuration is selected using presets defined in `CMakePresets.json`.

### Build for Local Development (x64 Debug)

This is the most common command you will use. It builds a debug version with AddressSanitizer (ASan) enabled.

```bash
./build.sh --preset x64-debug
```

### Build a Release Version (x64 Release)

This builds an optimized release version for the x64 architecture.

```bash
./build.sh --preset x64-release
```

### Cross-Compile for ARM64

This cross-compiles a release version for `aarch64`.

```bash
./build.sh --preset arm64-release
```
*Note: The cross-compiled binaries can only be run on an ARM64 target machine, not on your x64 build machine.*


## 5. Running the Application

After a successful build, the executables are placed in the `build/<preset-name>/bin/` directory.

To run the example simulated sensor application (after building with `x64-debug`):

```bash
./build/x64-debug/bin/sim_sensor_app
```

You should see the following output, with a new line appearing every 2 seconds:
```
C-EdgeX Application Started. Press Ctrl+C to exit.
[Driver] Initializing Sensor-001...
[App] Event: Device=Sensor-001, Resource=temperature, Value=25.43
[App] Event: Device=Sensor-001, Resource=temperature, Value=28.19
...
```

## 6. Running Tests

The test suite validates the functionality of the Platform SDK.

### Option 1: Using the Build Script (Recommended)

You can run the tests by building the `test` target.

```bash
./build.sh --preset x64-debug --target test
```
This will build and immediately run the tests, showing you the output.

### Option 2: Using CTest Manually

Alternatively, after a build, you can invoke `ctest` directly within the build directory.

```bash
cd build/x64-debug/
ctest --output-on-failure
```

A successful test run will look like this:
```
Test project /work/build/x64-debug
    Start 1: Platform.SDK.Tests
1/1 Test #1: Platform.SDK.Tests ...............   Passed    0.00 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.00 sec
```

## 7. How to Extend the Project

### Adding a New Product Application

1.  Create a new directory under `products/` (e.g., `products/my_new_app`).
2.  Inside it, create your `main.c` and any other source files.
3.  Create a `CMakeLists.txt` in your new directory. You can copy `products/sim_sensor_app/CMakeLists.txt` as a template. Make sure to change the `add_executable` name.
4.  Finally, add `add_subdirectory(products/my_new_app)` to the root `CMakeLists.txt` file.

That's it! The build system will automatically pick up your new application.
