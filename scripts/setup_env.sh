#!/bin/bash

# Shared environment setup for EdgeC scripts
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 1. Python Virtual Environment
if [ ! -d "$PROJECT_ROOT/.venv" ]; then
    echo "[Env] Creating virtual environment..."
    python3 -m venv "$PROJECT_ROOT/.venv"
fi

# Activate venv
export PATH="$PROJECT_ROOT/.venv/bin:$PATH"

# 2. Dependencies
if [ -f "$PROJECT_ROOT/requirements.txt" ]; then
    # Use a flag to avoid slow pip checks every time
    if [ ! -f "$PROJECT_ROOT/.venv/.deps_installed" ] || [ "$PROJECT_ROOT/requirements.txt" -nt "$PROJECT_ROOT/.venv/.deps_installed" ]; then
        echo "[Env] Installing/Updating Python dependencies..."
        pip install -q --upgrade pip
        pip install -q -r "$PROJECT_ROOT/requirements.txt"
        touch "$PROJECT_ROOT/.venv/.deps_installed"
    fi
fi

# 3. Paths
export PYTHONPATH="$PROJECT_ROOT"
# Support both local architectures
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PROJECT_ROOT/.local/linux-x86_64/lib:$PROJECT_ROOT/.local/linux-aarch64/lib"

# 4. Binary Permissions
chmod +x "$PROJECT_ROOT"/.local/linux-*/bin/contract-compiler 2>/dev/null || true
