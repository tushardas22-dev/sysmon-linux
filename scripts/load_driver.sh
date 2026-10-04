#!/usr/bin/env bash
# ==============================================================================
# scripts/load_driver.sh
# Build, Load, and Inspect the sysmon_driver character kernel module
# ==============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DRIVER_DIR="${SCRIPT_DIR}/../driver"

echo "=== Building and Loading sysmon_driver Kernel Module ==="

cd "${DRIVER_DIR}"

echo "[1/4] Building kernel module..."
make

echo "[2/4] Inserting module into Linux kernel..."
sudo insmod sysmon_driver.ko

echo "[3/4] Setting permissions for user-space access..."
sudo chmod 666 /dev/sysmon_driver

echo "[4/4] Verifying device node and kernel dmesg log..."
ls -l /dev/sysmon_driver
dmesg | tail -n 10 | grep -i sysmon || true

echo "=============================================================================="
echo " Module sysmon_driver loaded successfully!"
echo " SysMon will now automatically detect and communicate with /dev/sysmon_driver."
echo " To unload when finished: sudo rmmod sysmon_driver"
echo "=============================================================================="
