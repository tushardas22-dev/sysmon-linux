#include "utils/Config.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>

namespace sysmon {
namespace utils {

void Config::printHelp(const char* progName) {
    std::cout << "SysMon - System Resource Monitoring Tool\n"
              << "A lightweight system monitor in C++ (C++17) for Linux and WSL2\n\n"
              << "Usage: " << progName << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  -i, --interval <ms>    Set update refresh interval in ms (default: 1000)\n"
              << "  -m, --max-procs <num>  Maximum number of processes to display in table (default: 20)\n"
              << "  -s, --sort <field>     Sort processes by: cpu, mem, pid, name (default: cpu)\n"
              << "  --no-driver            Disable Linux kernel driver connection and use /proc only\n"
              << "  --mock                 Run with synthetic mock telemetry (for non-root/cross-platform demo)\n"
              << "  --headless             Run one iteration in non-interactive batch mode\n"
              << "  -l, --log <path>       Log file output path (default: sysmon.log)\n"
              << "  -h, --help             Show this help screen and exit\n"
              << "  -v, --version          Show version information\n\n";
}

Config Config::parseCommandLine(int argc, char* argv[]) {
    Config cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp(argv[0]);
            std::exit(0);
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "SysMon v1.0.0 (Linux & WSL2 System Programming Project)\n";
            std::exit(0);
        } else if ((arg == "-i" || arg == "--interval") && i + 1 < argc) {
            cfg.refreshIntervalMs = static_cast<uint32_t>(std::atoi(argv[++i]));
            if (cfg.refreshIntervalMs < 100) cfg.refreshIntervalMs = 100;
        } else if ((arg == "-m" || arg == "--max-procs") && i + 1 < argc) {
            cfg.maxProcessesDisplayed = static_cast<size_t>(std::atoi(argv[++i]));
        } else if ((arg == "-s" || arg == "--sort") && i + 1 < argc) {
            std::string s = argv[++i];
            if (s == "cpu") cfg.sortField = ProcessSortField::CPU;
            else if (s == "mem") cfg.sortField = ProcessSortField::MEMORY;
            else if (s == "pid") cfg.sortField = ProcessSortField::PID;
            else if (s == "name") cfg.sortField = ProcessSortField::NAME;
        } else if (arg == "--no-driver") {
            cfg.useKernelDriver = false;
        } else if (arg == "--mock") {
            cfg.mockMode = true;
        } else if (arg == "--headless") {
            cfg.headless = true;
        } else if ((arg == "-l" || arg == "--log") && i + 1 < argc) {
            cfg.logFilePath = argv[++i];
        }
    }

    return cfg;
}

} // namespace utils
} // namespace sysmon
