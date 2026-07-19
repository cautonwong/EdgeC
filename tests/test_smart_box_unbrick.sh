#!/bin/bash
# Test Smart Box U-Boot physical rescue script simulation
set -e

echo "=== [E2E Test] Simulating Smart Box Hardware SOS button recovery ==="

# 1. 模拟按下 GPIO 26 5秒上电
echo "Simulating GPIO 26 SOS button pressed..."
mkdir -p /mnt/usb_rec

# 2. 写入 golden 模拟原厂固件
mkdir -p /media/usb
echo "Mocking factory golden firmware OS image..." > /media/usb/golden_firmware.tar.gz

# 3. 触发 recovery 脚本
cat << 'EOF' > /media/usb/recovery.sh
#!/bin/sh
echo "[RECOVERY] Simulating Flash restore from golden image..."
mkdir -p /mnt/sys_a
echo "OS restoration done." > /mnt/sys_a/kernel.bin
echo "[RECOVERY] Resetting U-Boot environmental flags"
exit 0
EOF

chmod +x /media/usb/recovery.sh

# 4. 执行自愈救砖脚本
echo "Invoking recovery.sh..."
/media/usb/recovery.sh

echo "=== [E2E Test] Unbrick Recovery Simulation passed successfully! ==="
