#ifndef SYSTEM_MONITOR_APP_HPP
#define SYSTEM_MONITOR_APP_HPP

#include "utils/Config.hpp"
#include "core/CpuMonitor.hpp"
#include "core/MemoryMonitor.hpp"
#include "core/ProcessMonitor.hpp"
#include "core/DiskMonitor.hpp"
#include "core/NetworkMonitor.hpp"
#include "core/ArchitectureMonitor.hpp"
#include "driver_interface/KernelDriverClient.hpp"
#include "ui/TerminalUi.hpp"

#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

namespace sysmon {

class SystemMonitorApp {
public:
    SystemMonitorApp(const utils::Config& config);
    ~SystemMonitorApp();

    int run();

private:
    void dataCollectorLoop();
    void handleUserInput();

    utils::Config config_;
    core::CpuMonitor cpuMonitor_;
    core::MemoryMonitor memMonitor_;
    core::ProcessMonitor procMonitor_;
    core::DiskMonitor diskMonitor_;
    core::NetworkMonitor netMonitor_;
    core::ArchitectureMonitor archMonitor_;
    driver::KernelDriverClient driverClient_;
    ui::TerminalUi ui_;

    std::atomic<bool> running_{false};
    std::mutex dataMutex_;
    std::thread collectorThread_;

    // Cached snapshots for UI
    core::CpuSnapshot latestCpu_;
    core::MemorySnapshot latestMem_;
    core::ProcessSnapshot latestProcs_;
    core::DiskSnapshot latestDisk_;
    core::NetworkSnapshot latestNet_;
    core::ArchitectureSnapshot latestArch_;
    driver::DriverStatus latestDriverStatus_;
    struct sysmon_kernel_metrics latestKernelMetrics_;

    std::chrono::steady_clock::time_point lastSampleTime_;
};

} // namespace sysmon

#endif // SYSTEM_MONITOR_APP_HPP
