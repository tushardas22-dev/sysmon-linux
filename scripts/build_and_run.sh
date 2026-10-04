#!/usr/bin/env bash
# ==============================================================================
# scripts/build_and_run.sh
# One-click compile, test, and launch helper for SysMon
# ==============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."

cd "${ROOT_DIR}"

echo "=== [1/3] Compiling SysMon and Test Suite ==="
make clean
make

echo "=== [2/3] Running Unit & Integration Tests ==="
./bin/sysmon_tests

echo "=== [3/3] Launching SysMon Interactive Monitor ==="
echo "Tip: Press 'h' for help, '1-5' for tabs, 'q' to quit."
sleep 1
./bin/sysmon "$@"
