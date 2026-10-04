#include "ui/TerminalUi.hpp"
#include "ui/AnsiColors.hpp"
#include "utils/SystemUtils.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace sysmon {
namespace ui {

TerminalUi::TerminalUi(const utils::Config& config)
    : config_(config) {}

TerminalUi::~TerminalUi() {
    std::cout << Ansi::SHOW_CURSOR << std::flush;
    utils::SystemUtils::disableRawMode();
}

void TerminalUi::init() {
    utils::SystemUtils::getTerminalSize(termRows_, termCols_);
    std::cout << Ansi::HIDE_CURSOR << std::flush;
    utils::SystemUtils::enableRawMode();
}

void TerminalUi::setTab(ViewTab tab) {
    currentTab_ = tab;
}

ViewTab TerminalUi::getTab() const {
    return currentTab_;
}

void TerminalUi::nextTab() {
    int next = static_cast<int>(currentTab_) + 1;
    if (next > 5) next = 1;
    currentTab_ = static_cast<ViewTab>(next);
}

void TerminalUi::prevTab() {
    int prev = static_cast<int>(currentTab_) - 1;
    if (prev < 1) prev = 5;
    currentTab_ = static_cast<ViewTab>(prev);
}

void TerminalUi::setStatusMessage(const std::string& msg) {
    statusMessage_ = msg;
    statusMessageTimer_ = 5;
}

void TerminalUi::render(
    const core::CpuSnapshot& cpu,
    const core::MemorySnapshot& mem,
    const core::ProcessSnapshot& procs,
    const core::DiskSnapshot& disk,
    const core::NetworkSnapshot& net,
    const core::ArchitectureSnapshot& arch,
    const driver::DriverStatus& driverStatus,
    const struct sysmon_kernel_metrics& kMetrics
) {
    utils::SystemUtils::getTerminalSize(termRows_, termCols_);

    std::string buffer;
    buffer.reserve(8192);

    // Position cursor at top-left
    buffer += "\033[H";

    renderHeader(buffer, arch, driverStatus);
    renderTabBar(buffer);

    switch (currentTab_) {
        case ViewTab::DASHBOARD:
            renderDashboardView(buffer, cpu, mem, disk, net, procs);
            break;
        case ViewTab::ARCHITECTURE:
            renderArchitectureView(buffer, arch, cpu);
            break;
        case ViewTab::PROCESSES:
            renderProcessesView(buffer, procs);
            break;
        case ViewTab::KERNEL_DRIVER:
            renderKernelDriverView(buffer, driverStatus, kMetrics, mem);
            break;
        case ViewTab::HELP:
            renderHelpView(buffer);
            break;
    }

    renderFooter(buffer);

    std::cout << buffer << std::flush;
}

void TerminalUi::renderHeader(std::string& buffer, const core::ArchitectureSnapshot& arch, const driver::DriverStatus& drv) {
    buffer += Ansi::colorize("=== SysMon :: Linux & WSL2 System Resource Monitoring Tool ===", Ansi::BOLD);
    buffer += "  ";

    if (drv.connected) {
        buffer += Ansi::colorize("[DRIVER: CONNECTED (/dev/sysmon_driver)]", Ansi::GREEN);
    } else {
        buffer += Ansi::colorize("[DRIVER: FALLBACK (/proc)]", Ansi::YELLOW);
    }
    buffer += "\n";

    std::string archLine = "Arch: " + arch.architecture + " | Platform: " + arch.virtualizationType;
    if (arch.hasTemperatureSensor) {
        archLine += " | CPU Temp: " + std::to_string(static_cast<int>(arch.cpuTemperatureCelsius)) + " C";
    }
    buffer += Ansi::colorize(archLine, Ansi::DIM) + "\n";
    buffer += std::string(termCols_ > 20 ? termCols_ - 2 : 78, '-') + "\n";
}

void TerminalUi::renderTabBar(std::string& buffer) {
    auto tabItem = [this](int id, const std::string& name) -> std::string {
        if (static_cast<int>(currentTab_) == id) {
            return Ansi::colorize("[" + std::to_string(id) + ":" + name + "]", Ansi::BRIGHT_CYAN);
        } else {
            return "[" + std::to_string(id) + ":" + name + "]";
        }
    };

    buffer += tabItem(1, "Dashboard") + "  "
           + tabItem(2, "Hardware & Arch") + "  "
           + tabItem(3, "Processes") + "  "
           + tabItem(4, "Kernel Driver") + "  "
           + tabItem(5, "Help") + "\n\n";
}

void TerminalUi::renderDashboardView(
    std::string& buffer,
    const core::CpuSnapshot& cpu,
    const core::MemorySnapshot& mem,
    const core::DiskSnapshot& disk,
    const core::NetworkSnapshot& net,
    const core::ProcessSnapshot& procs
) {
    // Top Section: CPU & Memory Overview
    std::ostringstream ss;
    ss << Ansi::colorize("--- CPU & MEMORY SUBSYSTEMS ---", Ansi::BOLD) << "\n";
    ss << "Total CPU: " << Ansi::progressBar(cpu.totalUsagePercent, 25)
       << "  User: " << std::fixed << std::setprecision(1) << cpu.userPercent << "%"
       << "  Sys: " << cpu.systemPercent << "%"
       << "  Idle: " << cpu.idlePercent << "%"
       << "  IOWait: " << cpu.iowaitPercent << "%\n";

    ss << "RAM Usage: " << Ansi::progressBar(mem.ramUsagePercent, 25)
       << "  " << Ansi::formatBytes(mem.usedRamBytes) << " / " << Ansi::formatBytes(mem.totalRamBytes)
       << " (Avail: " << Ansi::formatBytes(mem.availableRamBytes) << ")\n";

    ss << "Swap     : " << Ansi::progressBar(mem.swapUsagePercent, 25)
       << "  " << Ansi::formatBytes(mem.usedSwapBytes) << " / " << Ansi::formatBytes(mem.totalSwapBytes)
       << " (Free: " << Ansi::formatBytes(mem.freeSwapBytes) << ")\n\n";

    // Per-Core Bars (SMP)
    if (!cpu.cores.empty()) {
        ss << Ansi::colorize("--- PER-CORE CPU LOAD (SMP) ---", Ansi::BOLD) << "\n";
        for (size_t i = 0; i < cpu.cores.size(); ++i) {
            ss << std::left << std::setw(6) << cpu.cores[i].name << " "
               << Ansi::progressBar(cpu.cores[i].totalUsagePercent, 18) << "  ";
            if ((i + 1) % 2 == 0 || i == cpu.cores.size() - 1) {
                ss << "\n";
            }
        }
        ss << "\n";
    }

    // Disk & Network I/O
    ss << Ansi::colorize("--- I/O THROUGHPUT & KERNEL EVENTS ---", Ansi::BOLD) << "\n";
    ss << "Disk Read : " << std::left << std::setw(12) << (Ansi::formatBytes(static_cast<uint64_t>(disk.totalReadRateBytesSec)) + "/s")
       << "  Disk Write: " << (Ansi::formatBytes(static_cast<uint64_t>(disk.totalWriteRateBytesSec)) + "/s") << "\n";
    ss << "Net RX    : " << std::left << std::setw(12) << (Ansi::formatBytes(static_cast<uint64_t>(net.totalRxRateBytesSec)) + "/s")
       << "  Net TX    : " << (Ansi::formatBytes(static_cast<uint64_t>(net.totalTxRateBytesSec)) + "/s") << "\n";
    ss << "Context Sw: " << std::left << std::setw(12) << (std::to_string(cpu.contextSwitchesDelta) + "/s")
       << "  Interrupts: " << (std::to_string(cpu.interruptsDelta) + "/s")
       << "  PgFaults: " << mem.pageFaultsDelta << "/s\n\n";

    // Top Processes
    ss << Ansi::colorize("--- TOP ACTIVE PROCESSES (By CPU%) ---", Ansi::BOLD) << "\n";
    ss << std::left
       << std::setw(7) << "PID"
       << std::setw(10) << "USER"
       << std::setw(4) << "S"
       << std::setw(8) << "CPU%"
       << std::setw(8) << "MEM%"
       << std::setw(10) << "RSS"
       << std::setw(6) << "THR"
       << "COMMAND\n";

    size_t count = std::min(procs.processes.size(), size_t(8));
    for (size_t i = 0; i < count; ++i) {
        const auto& p = procs.processes[i];
        std::string cmd = p.cmdline.empty() ? ("[" + p.name + "]") : p.cmdline;
        if (cmd.length() > 32) cmd = cmd.substr(0, 29) + "...";

        ss << std::left
           << std::setw(7) << p.pid
           << std::setw(10) << (p.user.length() > 9 ? p.user.substr(0, 8) : p.user)
           << std::setw(4) << p.state
           << std::fixed << std::setprecision(1)
           << std::setw(8) << p.cpuPercent
           << std::setw(8) << p.memPercent
           << std::setw(10) << Ansi::formatBytes(p.rssBytes)
           << std::setw(6) << p.numThreads
           << cmd << "\n";
    }

    buffer += ss.str();
}

void TerminalUi::renderArchitectureView(
    std::string& buffer,
    const core::ArchitectureSnapshot& arch,
    const core::CpuSnapshot& cpu
) {
    std::ostringstream ss;
    ss << Ansi::colorize("=== COMPUTER ARCHITECTURE & HARDWARE TELEMETRY ===", Ansi::BOLD) << "\n\n";

    ss << Ansi::colorize("[Processor Information]", Ansi::CYAN) << "\n";
    ss << "  Model Name   : " << arch.modelName << "\n";
    ss << "  Vendor ID    : " << arch.vendorId << "\n";
    ss << "  CPU Family   : " << arch.cpuFamily << " | Model: " << arch.model << " | Stepping: " << arch.stepping << "\n";
    ss << "  Online Cores : " << cpu.coreCount << "\n";
    ss << "  Platform     : " << arch.virtualizationType << "\n\n";

    ss << Ansi::colorize("[Memory Hierarchy & Cache Telemetry]", Ansi::CYAN) << "\n";
    if (arch.caches.empty()) {
        ss << "  Cache information not available in current environment.\n";
    } else {
        ss << "  LEVEL  TYPE             SIZE       ASSOCIATIVITY  LINE SIZE\n";
        ss << "  ----------------------------------------------------------\n";
        for (const auto& c : arch.caches) {
            ss << "  L" << c.level << "     "
               << std::left << std::setw(17) << c.type
               << std::left << std::setw(11) << c.sizeStr
               << std::left << std::setw(15) << (std::to_string(c.associativity) + "-way")
               << c.lineSize << " bytes\n";
        }
    }
    ss << "\n";

    ss << Ansi::colorize("[CPU Core Clock Frequencies]", Ansi::CYAN) << "\n";
    if (arch.frequencies.empty()) {
        ss << "  Frequency scaling info not exposed by hypervisor.\n";
    } else {
        for (const auto& f : arch.frequencies) {
            ss << "  Core " << std::left << std::setw(2) << f.coreId << ": "
               << "Cur: " << Ansi::formatFrequency(f.curFreqKhz)
               << " (Min: " << Ansi::formatFrequency(f.minFreqKhz)
               << " / Max: " << Ansi::formatFrequency(f.maxFreqKhz) << ")\n";
        }
    }
    ss << "\n";

    ss << Ansi::colorize("[Instruction Set Extensions / CPU Flags]", Ansi::CYAN) << "\n";
    ss << "  ";
    for (size_t i = 0; i < std::min(arch.featureFlags.size(), size_t(24)); ++i) {
        ss << arch.featureFlags[i] << " ";
    }
    if (arch.featureFlags.size() > 24) ss << "...";
    ss << "\n";

    buffer += ss.str();
}

void TerminalUi::renderProcessesView(std::string& buffer, const core::ProcessSnapshot& procs) {
    std::ostringstream ss;
    ss << Ansi::colorize("=== PROCESS EXPLORER & TASK SCHEDULING ===", Ansi::BOLD)
       << " (Total: " << procs.totalProcesses
       << " | Running: " << procs.runningProcesses
       << " | Sleeping: " << procs.sleepingProcesses
       << " | Zombie: " << procs.zombieProcesses << ")\n\n";

    ss << std::left
       << std::setw(7) << "PID"
       << std::setw(6) << "PPID"
       << std::setw(10) << "USER"
       << std::setw(4) << "S"
       << std::setw(5) << "PRI"
       << std::setw(4) << "NI"
       << std::setw(8) << "CPU%"
       << std::setw(8) << "MEM%"
       << std::setw(10) << "RSS"
       << std::setw(6) << "THR"
       << "COMMAND\n";
    ss << std::string(termCols_ > 20 ? termCols_ - 2 : 78, '-') << "\n";

    size_t limit = std::min(procs.processes.size(), config_.maxProcessesDisplayed);
    for (size_t i = 0; i < limit; ++i) {
        const auto& p = procs.processes[i];
        std::string cmd = p.cmdline.empty() ? ("[" + p.name + "]") : p.cmdline;
        int maxCmdLen = termCols_ - 68;
        if (maxCmdLen > 5 && static_cast<int>(cmd.length()) > maxCmdLen) {
            cmd = cmd.substr(0, maxCmdLen - 3) + "...";
        }

        ss << std::left
           << std::setw(7) << p.pid
           << std::setw(6) << p.ppid
           << std::setw(10) << (p.user.length() > 9 ? p.user.substr(0, 8) : p.user)
           << std::setw(4) << p.state
           << std::setw(5) << p.priority
           << std::setw(4) << p.nice
           << std::fixed << std::setprecision(1)
           << std::setw(8) << p.cpuPercent
           << std::setw(8) << p.memPercent
           << std::setw(10) << Ansi::formatBytes(p.rssBytes)
           << std::setw(6) << p.numThreads
           << cmd << "\n";
    }

    buffer += ss.str();
}

void TerminalUi::renderKernelDriverView(
    std::string& buffer,
    const driver::DriverStatus& drv,
    const struct sysmon_kernel_metrics& kMetrics,
    const core::MemorySnapshot& mem
) {
    std::ostringstream ss;
    ss << Ansi::colorize("=== LINUX CHARACTER DEVICE DRIVER TELEMETRY (/dev/sysmon_driver) ===", Ansi::BOLD) << "\n\n";

    ss << "Device Node Status : " << (drv.connected ? Ansi::colorize("ONLINE / ACTIVE", Ansi::GREEN) : Ansi::colorize("OFFLINE / FALLBACK", Ansi::YELLOW)) << "\n";
    ss << "Device File Path   : " << drv.devicePath << "\n";
    ss << "Status Message     : " << drv.message << "\n";
    ss << "Successful IOCTLs  : " << drv.successfulIoctls << " | Failed IOCTLs: " << drv.failedIoctls << "\n\n";

    ss << Ansi::colorize("[Kernel Space Telemetry Received via IOCTL]", Ansi::CYAN) << "\n";
    ss << "  Kernel Uptime      : " << utils::SystemUtils::formatUptime(kMetrics.uptime_seconds) << "\n";
    ss << "  Active CPUs (num)  : " << kMetrics.active_cpus << "\n";
    ss << "  Total RAM (kernel) : " << Ansi::formatBytes(kMetrics.total_ram_bytes) << "\n";
    ss << "  Free RAM (kernel)  : " << Ansi::formatBytes(kMetrics.free_ram_bytes) << "\n";
    ss << "  Buffer RAM (kernel): " << Ansi::formatBytes(kMetrics.buffer_ram_bytes) << "\n";
    ss << "  Total Task Count   : " << kMetrics.process_count << " tasks\n";
    ss << "  Context Switches   : " << kMetrics.context_switches << "\n";
    ss << "  Interrupts Handled : " << kMetrics.interrupts_total << "\n";
    ss << "  Load Averages      : " << std::fixed << std::setprecision(2)
       << (kMetrics.load_avg[0] / 65536.0) << ", "
       << (kMetrics.load_avg[1] / 65536.0) << ", "
       << (kMetrics.load_avg[2] / 65536.0) << "\n\n";

    ss << Ansi::colorize("[Kernel Space vs User Space (/proc) Comparison]", Ansi::CYAN) << "\n";
    ss << "  Metric               Kernel Driver IOCTL          User Space /proc Parsing\n";
    ss << "  --------------------------------------------------------------------------\n";
    ss << "  Total RAM:           " << std::left << std::setw(28) << Ansi::formatBytes(kMetrics.total_ram_bytes)
       << Ansi::formatBytes(mem.totalRamBytes) << "\n";
    ss << "  Free RAM:            " << std::left << std::setw(28) << Ansi::formatBytes(kMetrics.free_ram_bytes)
       << Ansi::formatBytes(mem.freeRamBytes) << "\n";
    ss << "  Acquisition Method:  Direct Kernel C Struct       Text Formatting + String Parsing\n";
    ss << "  Syscall Overhead:    1 IOCTL syscall              Multiple open(), read(), close()\n";

    buffer += ss.str();
}

void TerminalUi::renderHelpView(std::string& buffer) {
    std::ostringstream ss;
    ss << Ansi::colorize("=== KEYBOARD SHORTCUTS & HELP GUIDE ===", Ansi::BOLD) << "\n\n";
    ss << "  [1]           Switch to Overview Dashboard\n";
    ss << "  [2]           Switch to Computer Architecture & Hardware View\n";
    ss << "  [3]           Switch to Detailed Process Explorer\n";
    ss << "  [4]           Switch to Kernel Driver & IOCTL Telemetry View\n";
    ss << "  [5] or [h]    Switch to this Help Screen\n";
    ss << "  [Tab]         Cycle to next tab\n";
    ss << "  [c]           Sort processes by CPU Utilization (%)\n";
    ss << "  [m]           Sort processes by Memory Consumption (RSS)\n";
    ss << "  [p]           Sort processes by Process ID (PID)\n";
    ss << "  [n]           Sort processes by Process Name\n";
    ss << "  [k]           Send termination signal (SIGTERM) to a PID\n";
    ss << "  [q] / [Ctrl+C] Cleanly exit application\n\n";

    ss << Ansi::colorize("=== ABOUT THIS PROJECT ===", Ansi::CYAN) << "\n";
    ss << "  SysMon - System Resource Monitoring Tool\n";
    ss << "  Platform: Linux / Ubuntu (WSL2)\n";
    ss << "  Implementation: Modern C++ (C++17), POSIX APIs, Linux Character Driver\n";

    buffer += ss.str();
}

void TerminalUi::renderFooter(std::string& buffer) {
    buffer += "\n" + std::string(termCols_ > 20 ? termCols_ - 2 : 78, '-') + "\n";
    if (!statusMessage_.empty() && statusMessageTimer_ > 0) {
        buffer += Ansi::colorize("MESSAGE: " + statusMessage_, Ansi::BRIGHT_YELLOW) + "\n";
        statusMessageTimer_--;
    } else {
        buffer += Ansi::colorize("[1-5]:Tabs | [c]:Sort CPU | [m]:Sort Mem | [p]:Sort PID | [k]:Kill | [h]:Help | [q]:Quit", Ansi::DIM) + "\n";
    }
}

} // namespace ui
} // namespace sysmon
