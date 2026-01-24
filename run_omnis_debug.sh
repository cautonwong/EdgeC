#!/bin/bash
# [Debug] Omnis 全链路启动调试脚本

PROD="iems"
PROJECT_ROOT=$(pwd)
export PYTHONPATH=$PROJECT_ROOT
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PROJECT_ROOT/.local/linux-x86_64/lib

# 1. 彻底清理
echo "🧹 Cleaning up..."
pkill -9 -f $PROD || true
pkill -9 -f server.py || true
rm -f iems.db iems_run.log matrix_server.log
rm -f /tmp/vbus0.sock /tmp/v_adc.sock

# 2. 启动 Matrix Server
echo "🚀 Starting Matrix Server..."
.venv/bin/python3 -u tools/matrix/server.py --product $PROD > matrix_server.log 2>&1 &
SRV_PID=$!

# 等待环境准备就绪
echo "⏳ Waiting for Matrix backplane..."
MAX_RETRY=10
while [ $MAX_RETRY -gt 0 ]; do
    if grep -q "Matrix is UP" matrix_server.log; then
        break
    fi
    sleep 1
    MAX_RETRY=$((MAX_RETRY-1))
done

if [ $MAX_RETRY -eq 0 ]; then
    echo "❌ Matrix Server failed to start or timeout."
    cat matrix_server.log
    exit 1
fi

# 3. 提取环境变量并启动 IEMS
echo "🔗 Extracting environment..."
eval $(grep "export OMNIS_BUS_" matrix_server.log | sed 's/  //g')
export EDGE_DB_PATH="$PROJECT_ROOT/iems.db"
export OMNIS_SERVER="localhost:50051"

echo "📡 Env: BUS_0=$OMNIS_BUS_0_DEV, DB=$EDGE_DB_PATH"

echo "🚀 Launching IEMS..."
./build/products/iems/bin/iems > iems_run.log 2>&1 &
IEMS_PID=$!

# 等待 IEMS 启动并连接
sleep 5

echo "💉 Injecting Simulator Data: Tag 1001 -> 235.5V, Tag 1003 -> 5000W"
curl -s -X POST http://localhost:50051/api/v1/actor/set \
     -H "Content-Type: application/json" \
     -d '{"dev_id": "METER_MAIN", "tag": "1001", "value": 235.5}'
curl -s -X POST http://localhost:50051/api/v1/actor/set \
     -H "Content-Type: application/json" \
     -d '{"dev_id": "METER_MAIN", "tag": "1003", "value": 5000.0}'

# 4. 持续监控日志 (带颜色区分)
echo "🛰️  Monitoring Data Flow (15s)..."
tail -f iems_run.log | sed "s/^/$(printf '\033[34m[IEMS]\033[0m ')/" &
TAIL_IEMS_PID=$!
tail -f matrix_server.log | sed "s/^/$(printf '\033[35m[SRV ]\033[0m ')/" &
TAIL_SRV_PID=$!

sleep 15

# 5. 检查数据库任务状态
echo "📊 Checking DB internal state..."
python3 -c "import sqlite3; conn = sqlite3.connect('iems.db'); print('Tasks:', conn.execute('SELECT count(*) FROM poll_tasks').fetchone()[0]); conn.close()"

# 6. 清理
echo "🛑 Shutting down..."
kill $IEMS_PID $SRV_PID $TAIL_IEMS_PID $TAIL_SRV_PID 2>/dev/null || true
