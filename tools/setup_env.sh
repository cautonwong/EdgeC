#!/bin/bash
# EdgeOS Environment Setup Script
# ------------------------------------------------------------------------------
set(e)

VENV_DIR=".venv"

echo "🚀 EdgeOS Development Environment Bootstrapping..."

# 1. 创建虚拟环境 (如果不存在)
if [ ! -d "$VENV_DIR" ]; then
    echo "[Env] Creating virtual environment in $VENV_DIR..."
    python3 -m venv $VENV_DIR
else
    echo "[Env] Virtual environment already exists."
fi

# 2. 激活并安装依赖
echo "[Env] Synchronizing dependencies..."
source $VENV_DIR/bin/activate

# 更新 pip
pip install --upgrade pip -q

# 增量安装
if [ -f "requirements.txt" ]; then
    pip install -r requirements.txt -q
    echo "✅ Dependencies synchronized successfully."
else
    echo "❌ Error: requirements.txt not found!"
    exit 1
fi

echo "🎉 Setup complete!"
echo "👉 To activate manually: source .venv/bin/activate"
echo "👉 IDE Interpreter: ./.venv/bin/python"
