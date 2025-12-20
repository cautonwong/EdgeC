#!/bin/bash
set -e

# build.sh: A helper script to build the project in a consistent Docker environment.

# --- Configuration ---
IMAGE_NAME_X64="c-edgex-builder-x64"
DOCKERFILE_X64="build_env/Dockerfile.x64"
IMAGE_NAME_ARM64="c-edgex-builder-arm64"
DOCKERFILE_ARM64="build_env/Dockerfile.arm64"

# --- Helper Functions ---
print_usage() {
  echo "Usage: $0 --preset <PRESET_NAME> [CMake arguments...]"
  echo
  echo "Example: $0 --preset x64-debug"
  echo "         $0 --preset arm64-release --target sim_sensor_app"
  echo "         $0 --preset x64-debug --target test"
  echo
  echo "This script builds the project inside a Docker container to ensure a"
  echo "reproducible build environment."
  echo
  echo "Required Arguments:"
  echo "  --preset <PRESET_NAME>   The CMake preset to use (e.g., x64-debug, arm64-release)."
  echo
  echo "Optional CMake Arguments:"
  echo "  Any additional arguments are passed directly to the 'cmake --build' command."
  echo "  Common examples include:"
  echo "    --target <TARGET_NAME>   Build a specific target (e.g., sdk_tests, sim_sensor_app)."
  echo "    -j<N>                    Run the build with N parallel jobs."
  echo "    -v                       Enable verbose build output."
}

# --- Argument Parsing ---
if [[ "$1" != "--preset" ]] || [[ -z "$2" ]]; then
  print_usage
  exit 1
fi

PRESET_NAME="$2"
shift 2
CMAKE_ARGS=($@)

# Determine which Docker image to use based on the preset name
if [[ "$PRESET_NAME" == "arm64"* ]]; then
  TARGET_IMAGE="$IMAGE_NAME_ARM64"
  BASE_IMAGE="$IMAGE_NAME_X64"
  DOCKERFILE="$DOCKERFILE_ARM64"
  # For ARM, we need to build the base image first if it doesn't exist
  if [[ "$(docker images -q $BASE_IMAGE 2> /dev/null)" == "" ]]; then
    echo "--- Base x64 image not found. Building $BASE_IMAGE... ---"
    docker build -t "$BASE_IMAGE" -f "$DOCKERFILE_X64" .
  fi
else
  TARGET_IMAGE="$IMAGE_NAME_X64"
  DOCKERFILE="$DOCKERFILE_X64"
fi

# --- Main Build Logic ---

# 1. Build the Docker image if it doesn't exist
if [[ "$(docker images -q $TARGET_IMAGE 2> /dev/null)" == "" ]]; then
  echo "--- Building Docker image: $TARGET_IMAGE... ---"
  # Pass the base image name as a build argument for the ARM Dockerfile
  if [[ "$PRESET_NAME" == "arm64"* ]]; then
    docker build --build-arg "BASE_IMAGE_ARG=$BASE_IMAGE" -t "$TARGET_IMAGE" -f "$DOCKERFILE" .
  else
    docker build -t "$TARGET_IMAGE" -f "$DOCKERFILE" .
  fi
  echo "--- Docker image built successfully. ---"
fi

# 2. Configure the project using CMake (if not already configured)
BUILD_DIR="build/$PRESET_NAME"
if [ ! -f "$BUILD_DIR/build.ninja" ]; then
  echo "--- Configuring project with preset '$PRESET_NAME'... ---"
  docker run --rm -it \
    -v "$(pwd)":/work \
    -u "$(id -u):$(id -g)" \
    "$TARGET_IMAGE" \
    cmake --preset "$PRESET_NAME"
  echo "--- Configuration complete. ---"
fi

# 3. Build the project using the specified arguments
echo "--- Building project with preset '$PRESET_NAME'... ---"
docker run --rm -it \
  -v "$(pwd)":/work \
  -u "$(id -u):$(id -g)" \
  "$TARGET_IMAGE" \
  cmake --build "build/$PRESET_NAME" "${CMAKE_ARGS[@]}"

echo "--- Build finished. ---"
echo "Artifacts are in: $BUILD_DIR"
