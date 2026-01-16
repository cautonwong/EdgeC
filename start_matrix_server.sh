#!/bin/bash
# start_matrix_server.sh - 启动 Matrix 仿真服务器 (Standalone)

export PYTHONPATH=$PYTHONPATH:$(pwd)
PRODUCT=${1:-iems} # 默认启动 iems

echo "🌐 Starting Matrix Simulation Server for $PRODUCT..."
echo "📍 Schema: build/products/$PRODUCT/generated"
echo "🎮 Control Port: 9999"

.venv/bin/python3 tools/matrix/main.py --mode server --schema build/products/$PRODUCT/generated
