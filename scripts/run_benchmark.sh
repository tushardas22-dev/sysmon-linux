#!/usr/bin/env bash
# ==============================================================================
# scripts/run_benchmark.sh
# Synthetic CPU and Memory Stress Generator for SysMon Demonstration
# ==============================================================================

echo "=== SysMon Live Stress Test Generator ==="
echo "This script generates synthetic CPU and Memory load to observe real-time spikes in SysMon."

# Function to spawn CPU workers
stress_cpu() {
    echo "Starting CPU load workers for 15 seconds..."
    for i in $(seq 1 $(nproc)); do
        (while :; do :; done) &
    done
    PIDS=$(jobs -p)
    echo "Spawned CPU workers with PIDs: $PIDS"
    sleep 15
    echo "Stopping CPU workers..."
    kill $PIDS 2>/dev/null || true
}

echo "Select benchmark mode:"
echo "  1) CPU Stress (all available cores for 15 seconds)"
echo "  2) Memory Allocation Pulse"
echo "  3) Combined Test"
echo "  4) Exit"

read -p "Enter choice [1-4]: " choice

case "$choice" in
    1)
        stress_cpu
        ;;
    2)
        echo "Allocating 256MB RAM in Python/C++ for 10 seconds..."
        python3 -c "import time; x = bytearray(256 * 1024 * 1024); time.sleep(10)" 2>/dev/null || \
        perl -e '$x = "x" x (256 * 1024 * 1024); sleep 10;'
        ;;
    3)
        stress_cpu &
        python3 -c "import time; x = bytearray(256 * 1024 * 1024); time.sleep(15)" 2>/dev/null || true
        wait
        ;;
    *)
        echo "Exiting."
        ;;
esac

echo "Benchmark test completed."
