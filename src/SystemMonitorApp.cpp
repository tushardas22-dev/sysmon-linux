#include "SystemMonitorApp.hpp"
#include "utils/SystemUtils.hpp"
#include "utils/Logger.hpp"
#include "ui/AnsiColors.hpp"

#include <iostream>
#include <chrono>
#include <cstring>

namespace sysmon {

SystemMonitorApp::SystemMonitorApp(const utils::Config& config)
    : config_(config),
      ui_(config) {
    std::memset(&latestKernelMetrics_, 0, sizeof(latestKernelMetrics_));
}

SystemMonitorApp::~SystemMonitorApp() {
    running_.store(false);
    if (collectorThread_.joinable()) {
        collectorThread_.join();
    }
}

int SystemMonitorApp::run() {
    utils::SystemUtils::registerSignalHandlers();
    utils::Logger::getInstance().info("Starting System Resource Monitoring Tool...");

    if (config_.useKernelDriver && !config_.mockMode) {
        driverClient_.initialize();
    }

    running_.store(true);
    lastSampleTime_ = std::chrono::steady_clock::now();

    // Start background collector thread
    collectorThread_ = std::thread(&SystemMonitorApp::dataCollectorLoop, this);

    // Give collector a brief moment to take initial sample
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    if (config_.headless) {
        // Run single iteration in headless mode
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.refreshIntervalMs));
        std::lock_guard<std::mutex> lock(dataMutex_);
        ui_.render(latestCpu_, latestMem_, latestProcs_, latestDisk_, latestNet_, latestArch_, latestDriverStatus_, latestKernelMetrics_);
        running_.store(false);
        if (collectorThread_.joinable()) {
            collectorThread_.join();
        }
        return 0;
    }

    ui_.init();

    // Main interactive UI loop
    while (running_.load() && !utils::SystemUtils::isInterrupted()) {
        handleUserInput();

        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            ui_.render(
                latestCpu_,
                latestMem_,
                latestProcs_,
                latestDisk_,
                latestNet_,
                latestArch_,
                latestDriverStatus_,
                latestKernelMetrics_
            );
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }

    running_.store(false);
    if (collectorThread_.joinable()) {
        collectorThread_.join();
    }

    std::cout << ui::Ansi::CLEAR_SCREEN << ui::Ansi::SHOW_CURSOR;
    std::cout << "SysMon shutdown successfully. Goodbye!\n";
    return 0;
}

void SystemMonitorApp::dataCollectorLoop() {
    while (running_.load()) {
        auto now = std::chrono::steady_clock::now();
        double elapsedSec = std::chrono::duration<double>(now - lastSampleTime_).count();
        if (elapsedSec <= 0.0) elapsedSec = 1.0;
        lastSampleTime_ = now;

        // Perform updates
        cpuMonitor_.update(config_.mockMode);
        memMonitor_.update(config_.mockMode);
        archMonitor_.update(config_.mockMode);
        diskMonitor_.update(elapsedSec, config_.mockMode);
        netMonitor_.update(elapsedSec, config_.mockMode);

        const auto& cpuSnap = cpuMonitor_.getSnapshot();
        const auto& memSnap = memMonitor_.getSnapshot();

        // Calculate approximate delta ticks across all cores for process CPU % calculation
        uint64_t sysTicksDelta = static_cast<uint64_t>(elapsedSec * 100.0 * (cpuSnap.coreCount > 0 ? cpuSnap.coreCount : 1));
        procMonitor_.update(sysTicksDelta, memSnap.totalRamBytes, config_.mockMode);
        procMonitor_.sortProcesses(config_.sortField, config_.sortAscending);

        // Fetch driver telemetry
        struct sysmon_kernel_metrics km;
        bool hasKm = driverClient_.queryMetrics(km, config_.mockMode);

        // Update thread-safe snapshots
        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            latestCpu_ = cpuSnap;
            latestMem_ = memSnap;
            latestProcs_ = procMonitor_.getSnapshot();
            latestDisk_ = diskMonitor_.getSnapshot();
            latestNet_ = netMonitor_.getSnapshot();
            latestArch_ = archMonitor_.getSnapshot();
            latestDriverStatus_ = driverClient_.getStatus();
            if (hasKm) {
                latestKernelMetrics_ = km;
            }
        }

        // Sleep for configured refresh interval
        auto sleepTime = std::chrono::milliseconds(config_.refreshIntervalMs);
        auto sleepStart = std::chrono::steady_clock::now();
        while (running_.load() && (std::chrono::steady_clock::now() - sleepStart) < sleepTime) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void SystemMonitorApp::handleUserInput() {
    while (utils::SystemUtils::isKeyPressed()) {
        char c = utils::SystemUtils::readKey();
        if (c == 'q' || c == 'Q' || c == 27) { // 27 = ESC
            running_.store(false);
            break;
        } else if (c >= '1' && c <= '5') {
            ui_.setTab(static_cast<ui::ViewTab>(c - '0'));
        } else if (c == '\t') {
            ui_.nextTab();
        } else if (c == 'c' || c == 'C') {
            config_.sortField = utils::ProcessSortField::CPU;
            ui_.setStatusMessage("Sorting processes by CPU% (Descending)");
        } else if (c == 'm' || c == 'M') {
            config_.sortField = utils::ProcessSortField::MEMORY;
            ui_.setStatusMessage("Sorting processes by Memory consumption (Descending)");
        } else if (c == 'p' || c == 'P') {
            config_.sortField = utils::ProcessSortField::PID;
            ui_.setStatusMessage("Sorting processes by PID");
        } else if (c == 'n' || c == 'N') {
            config_.sortField = utils::ProcessSortField::NAME;
            ui_.setStatusMessage("Sorting processes by Name");
        } else if (c == 'h' || c == 'H') {
            ui_.setTab(ui::ViewTab::HELP);
        } else if (c == 'k' || c == 'K') {
            ui_.setStatusMessage("Process Signal interface: [Press PID in Process tab]");
        }
    }
}

} // namespace sysmon
