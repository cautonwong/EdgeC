#!/bin/bash
# start_all_debug.sh - 一键拉起仿真 Server 和 IEMS 进程 (无 netstat 兼容版)

export PYTHONPATH=$PYTHONPATH:$(pwd)
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/.local/linux-x86_64/lib
export OMNIS_VBUS_PATH=/tmp/vbus0.sock
export EDGE_REST_PORT=9091

echo "🧹 Cleaning up old sockets and logs..."
rm -f /tmp/vbus0.sock
rm -f build/matrix_server.log

echo "🌐 [1/2] Starting Matrix Server (Standalone)..."
.venv/bin/python3 tools/matrix/main.py --mode server --schema build/products/iems/generated > build/matrix_server.log 2>&1 &
SERVER_PID=$!

# 使用 Python 脚本检测端口
echo "⏳ Waiting for Matrix Control API (9999) to be ready..."
.venv/bin/python3 -c "import socket, time; s = socket.socket(); [time.sleep(1) or s.connect_ex(('127.0.0.1', 9999)) for _ in range(15)]"

echo "🏗️ [2/2] Starting IEMS process..."
./build/products/iems/bin/iems &
IEMS_PID=$!

echo "----------------------------------------------------"
echo "✅ All systems active!"
echo "🎮 Run: .venv/bin/python3 tools/matrix/matrix-cli.py"
echo "🎮 Run: .venv/bin/python3 verify_flow.py"
echo "----------------------------------------------------"

trap "echo 'Stopping...'; kill $SERVER_PID $IEMS_PID; exit" SIGINT SIGTERM
wait
