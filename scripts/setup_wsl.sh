#!/usr/bin/env bash
# ==============================================================================
# scripts/setup_wsl.sh
# System Environment Setup for Ubuntu / WSL2
# ==============================================================================
set -e

echo "=== Setting up SysMon Development Environment on Ubuntu/WSL2 ==="

echo "[1/4] Updating package index..."
sudo apt-get update -y

echo "[2/4] Installing C++ build essentials, CMake, and debugging tools..."
sudo apt-get install -y \
    build-essential \
    g++ \
    cmake \
    make \
    git \
    valgrind \
    gdb \
    htop \
    tree

echo "[3/4] Installing Linux kernel headers (if available for module build)..."
sudo apt-get install -y linux-headers-$(uname -r) 2>/dev/null || {
    echo "Note: Standard WSL2 kernel headers might require custom kernel configuration for out-of-tree kernel modules."
    echo "SysMon will operate seamlessly using procfs fallback if kernel module is not loaded."
}

echo "[4/4] Verifying compiler and tool versions..."
g++ --version | head -n 1
cmake --version | head -n 1
make --version | head -n 1

echo "=============================================================================="
echo " Environment setup completed successfully!"
echo " You can now compile and run SysMon:"
echo "   make"
echo "   ./bin/sysmon"
echo "=============================================================================="
