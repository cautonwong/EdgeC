#!/bin/bash

# Shared environment setup for EdgeC scripts
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 1. Python Virtual Environment
if [ ! -d "$PROJECT_ROOT/.venv" ]; then
    echo "[Env] Creating virtual environment..."
    python3 -m venv "$PROJECT_ROOT/.venv"
fi

# 显式解释器路径
ENV_PYTHON="$PROJECT_ROOT/.venv/bin/python3"
export ENV_PYTHON

# 2. Dependencies
if [ -f "$PROJECT_ROOT/requirements.txt" ]; then
    if ! "$ENV_PYTHON" -c "import numpy" 2>/dev/null; then
        echo "[Env] Dependency 'numpy' missing. Installing..."
        "$ENV_PYTHON" -m pip install -q --upgrade pip
        "$ENV_PYTHON" -m pip install -q -r "$PROJECT_ROOT/requirements.txt"
    fi
fi

# 3. Paths
export PYTHONPATH="$PROJECT_ROOT"
# Support both local architectures
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PROJECT_ROOT/.local/linux-x86_64/lib:$PROJECT_ROOT/.local/linux-aarch64/lib"

# 4. Binary Permissions
chmod +x "$PROJECT_ROOT"/.local/linux-*/bin/contract-compiler 2>/dev/null || true
