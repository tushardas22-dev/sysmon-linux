#include "core/CpuMonitor.hpp"
#include "core/MemoryMonitor.hpp"
#include "core/ProcessMonitor.hpp"
#include "core/ArchitectureMonitor.hpp"
#include "core/DiskMonitor.hpp"
#include "core/NetworkMonitor.hpp"
#include "driver_interface/KernelDriverClient.hpp"
#include "utils/ProcReader.hpp"
#include "ui/AnsiColors.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <chrono>

using namespace sysmon;

static int g_passCount = 0;
static int g_failCount = 0;

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << ui::Ansi::RED << "  [ASSERT FAILED] " << #cond \
                      << " at " << __FILE__ << ":" << __LINE__ << ui::Ansi::RESET << "\n"; \
            return false; \
        } \
    } while (0)

#define ASSERT_EQUAL(a, b) \
    do { \
        if ((a) != (b)) { \
            std::cerr << ui::Ansi::RED << "  [ASSERT FAILED] " << #a << " (" << (a) \
                      << ") != " << #b << " (" << (b) << ")" \
                      << " at " << __FILE__ << ":" << __LINE__ << ui::Ansi::RESET << "\n"; \
            return false; \
        } \
    } while (0)

void runTest(const std::string& testName, std::function<bool()> testFunc) {
    std::cout << "[ RUN      ] " << testName << "..." << std::flush;
    auto start = std::chrono::high_resolution_clock::now();
    bool passed = false;
    try {
        passed = testFunc();
    } catch (const std::exception& e) {
        std::cerr << ui::Ansi::RED << " Exception: " << e.what() << ui::Ansi::RESET << "\n";
        passed = false;
    } catch (...) {
        std::cerr << ui::Ansi::RED << " Unknown exception" << ui::Ansi::RESET << "\n";
        passed = false;
    }
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();

    if (passed) {
        std::cout << "\r" << ui::Ansi::GREEN << "[       OK ] " << ui::Ansi::RESET
                  << testName << " (" << ms << " ms)\n";
        g_passCount++;
    } else {
        std::cout << "\r" << ui::Ansi::RED << "[  FAILED  ] " << ui::Ansi::RESET
                  << testName << " (" << ms << " ms)\n";
        g_failCount++;
    }
}

// Test 1: CPU Calculation and SMP Cores
bool testCpuMonitor() {
    core::CpuMonitor cpu;
    cpu.update(true); // First sample establishes baseline
    cpu.update(true); // Second sample computes deltas

    const auto& snap = cpu.getSnapshot();
    ASSERT_TRUE(snap.totalUsagePercent >= 0.0 && snap.totalUsagePercent <= 100.0);
    ASSERT_TRUE(snap.coreCount > 0);
    ASSERT_TRUE(snap.cores.size() == snap.coreCount);
    for (const auto& c : snap.cores) {
        ASSERT_TRUE(c.totalUsagePercent >= 0.0 && c.totalUsagePercent <= 100.0);
    }
    ASSERT_TRUE(!snap.historySparkline.empty());
    return true;
}

// Test 2: Memory & Swap Calculations
bool testMemoryMonitor() {
    core::MemoryMonitor mem;
    mem.update(true);

    const auto& snap = mem.getSnapshot();
    ASSERT_TRUE(snap.totalRamBytes > 0);
    ASSERT_TRUE(snap.usedRamBytes <= snap.totalRamBytes);
    ASSERT_TRUE(snap.freeRamBytes <= snap.totalRamBytes);
    ASSERT_TRUE(snap.ramUsagePercent >= 0.0 && snap.ramUsagePercent <= 100.0);
    ASSERT_TRUE(snap.totalSwapBytes > 0);
    ASSERT_TRUE(snap.swapUsagePercent >= 0.0 && snap.swapUsagePercent <= 100.0);
    return true;
}

// Test 3: Process Enumeration and Sorting
bool testProcessMonitor() {
    core::ProcessMonitor pm;
    pm.update(1000, 16ULL * 1024 * 1024 * 1024, true);
    pm.update(1000, 16ULL * 1024 * 1024 * 1024, true);

    const auto& snap = pm.getSnapshot();
    ASSERT_TRUE(snap.totalProcesses > 0);
    ASSERT_TRUE(!snap.processes.empty());

    // Test sort by CPU descending
    pm.sortProcesses(utils::ProcessSortField::CPU, false);
    for (size_t i = 1; i < snap.processes.size(); ++i) {
        ASSERT_TRUE(snap.processes[i - 1].cpuPercent >= snap.processes[i].cpuPercent);
    }

    // Test sort by PID ascending
    pm.sortProcesses(utils::ProcessSortField::PID, true);
    for (size_t i = 1; i < snap.processes.size(); ++i) {
        ASSERT_TRUE(snap.processes[i - 1].pid <= snap.processes[i].pid);
    }

    // Test filter
    auto filtered = pm.filterByName("sysmon");
    ASSERT_TRUE(!filtered.empty());
    ASSERT_EQUAL(filtered[0].name, std::string("sysmon"));

    return true;
}

// Test 4: Architecture & Cache Hierarchy
bool testArchitectureMonitor() {
    core::ArchitectureMonitor arch;
    arch.update(true);

    const auto& snap = arch.getSnapshot();
    ASSERT_TRUE(!snap.vendorId.empty());
    ASSERT_TRUE(!snap.modelName.empty());
    ASSERT_TRUE(!snap.caches.empty());
    ASSERT_TRUE(snap.caches.size() >= 3); // L1, L2, L3 caches
    ASSERT_TRUE(!snap.featureFlags.empty());
    return true;
}

// Test 5: Disk & Network I/O
bool testDiskAndNetwork() {
    core::DiskMonitor disk;
    disk.update(1.0, true);
    disk.update(1.0, true);
    const auto& dSnap = disk.getSnapshot();
    ASSERT_TRUE(!dSnap.devices.empty());
    ASSERT_TRUE(!dSnap.filesystems.empty());

    core::NetworkMonitor net;
    net.update(1.0, true);
    net.update(1.0, true);
    const auto& nSnap = net.getSnapshot();
    ASSERT_TRUE(!nSnap.interfaces.empty());
    return true;
}

// Test 6: Kernel Driver Client & Shared Structs
bool testKernelDriverClient() {
    driver::KernelDriverClient client("/dev/sysmon_driver_mock");
    struct sysmon_kernel_metrics km;
    bool resMetrics = client.queryMetrics(km, true);
    ASSERT_TRUE(resMetrics);
    ASSERT_TRUE(km.total_ram_bytes > 0);
    ASSERT_TRUE(km.active_cpus > 0);
    ASSERT_TRUE(km.uptime_seconds > 0);

    struct sysmon_proc_table pt;
    bool resProcs = client.queryProcessTable(pt, true);
    ASSERT_TRUE(resProcs);
    ASSERT_TRUE(pt.count > 0);
    ASSERT_TRUE(pt.entries[0].pid > 0);
    return true;
}

// Test 7: ANSI Formatters & Formatting
bool testAnsiFormatting() {
    std::string bar = ui::Ansi::progressBar(50.0, 10);
    ASSERT_TRUE(!bar.empty());
    ASSERT_TRUE(bar.find("50.0%") != std::string::npos);

    std::string bytes1 = ui::Ansi::formatBytes(1024);
    ASSERT_TRUE(bytes1.find("1.00 KB") != std::string::npos);

    std::string bytes2 = ui::Ansi::formatBytes(1024ULL * 1024 * 1024);
    ASSERT_TRUE(bytes2.find("1.00 GB") != std::string::npos);

    std::string freq = ui::Ansi::formatFrequency(2400000);
    ASSERT_TRUE(freq.find("2.40 GHz") != std::string::npos);
    return true;
}

int main() {
    std::cout << ui::Ansi::BOLD << "========================================\n"
              << " SysMon Unit & Integration Test Suite   \n"
              << "========================================\n" << ui::Ansi::RESET;

    runTest("CpuMonitorTest.UsageAndCoreCalculations", testCpuMonitor);
    runTest("MemoryMonitorTest.RamAndSwapMetrics", testMemoryMonitor);
    runTest("ProcessMonitorTest.ProcessFilteringAndSorting", testProcessMonitor);
    runTest("ArchitectureMonitorTest.CachesAndHardware", testArchitectureMonitor);
    runTest("DiskAndNetworkTest.ThroughputMetrics", testDiskAndNetwork);
    runTest("KernelDriverClientTest.IoctlStructsAndFallback", testKernelDriverClient);
    runTest("AnsiFormattingTest.BarsAndUnits", testAnsiFormatting);

    std::cout << ui::Ansi::BOLD << "\n----------------------------------------\n"
              << "Summary: " << g_passCount << " Passed, " << g_failCount << " Failed\n"
              << "----------------------------------------\n" << ui::Ansi::RESET;

    return (g_failCount == 0) ? 0 : 1;
}
