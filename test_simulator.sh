#!/bin/bash
set -e

# --- Test Configuration ---
PROD_NAME="iems"
PROTOCOL="DLT645-2007"
PORT_MASTER="/tmp/vport-master"
PORT_SLAVE="/tmp/vport-slave"
SIM_LOG="simulator.log"
SIM_PATH="./build/products/device_simulator/bin/device_simulator"
REQUEST_STRING="\x68\x89\x67\x45\x23\x01\x00\x68\x11\x04\x33\x34\x34\x35\x72\x16"
LD_PATH="./.local/linux-x86_64/lib"

# --- Helper Functions ---
function cleanup() {
    echo "[TEST] Cleaning up simulator process..."
    pkill -f device_simulator || true
    rm -f $SIM_LOG
}

function wait_for_log() {
    local log_file=$1
    local pattern=$2
    local timeout=$3
    echo "[TEST] Waiting for log pattern '$pattern' in $log_file..."
    for i in $(seq 1 $timeout); do
        if [ -f "$log_file" ] && grep -q "$pattern" "$log_file"; then
            echo "[TEST] Found log pattern."
            return 0
        fi
        sleep 1
    done
    echo "[TEST] Error: Timed out waiting for log pattern."
    return 1
}


# --- Main Test Execution ---
trap cleanup EXIT
cleanup

echo "[TEST] Starting device simulator..."
env LD_LIBRARY_PATH=$LD_PATH $SIM_PATH --product $PROD_NAME --protocol "$PROTOCOL" --port $PORT_SLAVE > $SIM_LOG 2>&1 &
wait_for_log $SIM_LOG "Listening..." 5

echo "[TEST] Sending DLT-645 request for VOLTAGE_PHASE_A..."
echo -ne "$REQUEST_STRING" > $PORT_MASTER

RESPONSE_HEX=$(timeout 3s cat $PORT_MASTER | xxd -p)

# --- Reporting ---
echo "----------------------------------------"
echo "--- Final Simulator Log ---"
cat $SIM_LOG
echo
echo "--- Response Received (hex) ---"
if [ -n "$RESPONSE_HEX" ]; then
    echo "$RESPONSE_HEX"
    # Verification: expected value 221.7 -> 22170 -> BCD 70 21 02 00
    # The response frame echoes the DI (33343435) and then the value.
    # We are checking for the value part: 70+33=a3, 21+33=54
    if [[ "$RESPONSE_HEX" == *"a354"* ]]; then
        echo "[SUCCESS] Response contains correct value."
    else
        echo "[FAILURE] Response does not contain expected value."
        exit 1
    fi
else
    echo "(No response received)"
    exit 1
fi
echo "----------------------------------------"
echo "[SUCCESS] Test completed successfully."
exit 0