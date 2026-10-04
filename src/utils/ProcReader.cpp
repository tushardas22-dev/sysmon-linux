#include "utils/ProcReader.hpp"
#include "utils/Logger.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <chrono>

#if !defined(_WIN32)
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#endif

namespace sysmon {
namespace utils {

uint64_t ProcReader::mockTickCounter_ = 100;

bool ProcReader::isProcAvailable() {
#if defined(_WIN32)
    return false;
#else
    return std::filesystem::exists("/proc/stat");
#endif
}

std::string ProcReader::readFileContents(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::vector<std::string> ProcReader::readFileLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    if (!file.is_open()) {
        return lines;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    return lines;
}

std::vector<CpuStatRaw> ProcReader::readCpuStats(bool mock) {
    std::vector<CpuStatRaw> stats;

    if (mock || !isProcAvailable()) {
        mockTickCounter_++;
        // Produce total cpu + 4 cores mock
        for (int i = -1; i < 4; ++i) {
            CpuStatRaw r;
            r.cpuName = (i == -1) ? "cpu" : ("cpu" + std::to_string(i));
            uint64_t base = mockTickCounter_ * 100 + (i + 1) * 20;
            double loadFactor = 0.25 + 0.15 * std::sin((mockTickCounter_ + i * 2) * 0.2);
            if (loadFactor < 0.05) loadFactor = 0.05;
            r.user = static_cast<uint64_t>(base * loadFactor);
            r.nice = 50;
            r.system = static_cast<uint64_t>(base * (loadFactor * 0.3));
            r.idle = base - (r.user + r.system + r.nice);
            r.iowait = 30;
            r.irq = 10;
            r.softirq = 15;
            r.steal = 0;
            stats.push_back(r);
        }
        return stats;
    }

    std::ifstream file("/proc/stat");
    if (!file.is_open()) {
        return stats;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("cpu", 0) == 0) {
            std::istringstream iss(line);
            CpuStatRaw stat;
            iss >> stat.cpuName
                >> stat.user >> stat.nice >> stat.system >> stat.idle
                >> stat.iowait >> stat.irq >> stat.softirq >> stat.steal
                >> stat.guest >> stat.guestNice;
            stats.push_back(stat);
        }
    }
    return stats;
}

std::unordered_map<std::string, uint64_t> ProcReader::readMemInfo(bool mock) {
    std::unordered_map<std::string, uint64_t> mem;

    if (mock || !isProcAvailable()) {
        mem["MemTotal"]     = 16ULL * 1024 * 1024; // 16 GB in KB
        mem["MemFree"]      = 4ULL * 1024 * 1024;
        mem["MemAvailable"] = 9ULL * 1024 * 1024;
        mem["Buffers"]      = 512 * 1024;
        mem["Cached"]       = 4ULL * 1024 * 1024;
        mem["SwapTotal"]    = 4ULL * 1024 * 1024;
        mem["SwapFree"]     = 3ULL * 1024 * 1024;
        mem["Dirty"]        = 128 * 1024;
        return mem;
    }

    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) {
        return mem;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::istringstream iss(line.substr(colon + 1));
            uint64_t val = 0;
            iss >> val;
            mem[key] = val;
        }
    }
    return mem;
}

std::unordered_map<std::string, uint64_t> ProcReader::readVmStat(bool mock) {
    std::unordered_map<std::string, uint64_t> vm;
    if (mock || !isProcAvailable()) {
        vm["pgfault"] = 1205000 + mockTickCounter_ * 45;
        vm["pgmajfault"] = 3200 + mockTickCounter_;
        return vm;
    }

    std::ifstream file("/proc/vmstat");
    if (!file.is_open()) return vm;

    std::string key;
    uint64_t val;
    while (file >> key >> val) {
        vm[key] = val;
    }
    return vm;
}

std::vector<std::string> ProcReader::getProcessPids(bool mock) {
    std::vector<std::string> pids;
    if (mock || !isProcAvailable()) {
        pids = {"1", "2", "110", "450", "890", "1024", "1340", "1500", "2048", "3000"};
        return pids;
    }

#if !defined(_WIN32)
    DIR* dir = opendir("/proc");
    if (!dir) return pids;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR) {
            char* endptr = nullptr;
            long pid = strtol(entry->d_name, &endptr, 10);
            if (*endptr == '\0' && pid > 0) {
                pids.push_back(entry->d_name);
            }
        }
    }
    closedir(dir);
#endif
    return pids;
}

bool ProcReader::readProcessStat(int pid, ProcPidStatRaw& outStat, bool mock) {
    if (mock || !isProcAvailable()) {
        outStat.pid = pid;
        outStat.ppid = (pid == 1) ? 0 : 1;
        outStat.state = 'S';
        outStat.priority = 20;
        outStat.nice = 0;
        outStat.num_threads = 4;
        outStat.utime = 150 + pid * 2;
        outStat.stime = 50 + pid;
        outStat.rss = 1000 + pid * 50;
        outStat.vsize = 20000000 + pid * 100000;
        switch (pid) {
            case 1:    outStat.comm = "systemd"; break;
            case 2:    outStat.comm = "kthreadd"; break;
            case 110:  outStat.comm = "systemd-journal"; break;
            case 450:  outStat.comm = "dbus-daemon"; break;
            case 890:  outStat.comm = "sshd"; break;
            case 1024: outStat.comm = "sysmon"; outStat.state = 'R'; break;
            case 1340: outStat.comm = "bash"; break;
            case 1500: outStat.comm = "python3"; break;
            case 2048: outStat.comm = "clang++"; outStat.state = 'R'; break;
            default:   outStat.comm = "worker_proc"; break;
        }
        return true;
    }

    std::string path = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::getline(file, line);
    if (line.empty()) return false;

    // Process name can contain spaces and parentheses: e.g. "54 (Web Content) S ..."
    // We locate the first '(' and the last ')'
    size_t openParen = line.find('(');
    size_t closeParen = line.rfind(')');
    if (openParen == std::string::npos || closeParen == std::string::npos || closeParen < openParen) {
        return false;
    }

    outStat.pid = std::stoi(line.substr(0, openParen));
    outStat.comm = line.substr(openParen + 1, closeParen - openParen - 1);

    std::string rest = line.substr(closeParen + 2);
    std::istringstream iss(rest);

    iss >> outStat.state >> outStat.ppid >> outStat.pgrp >> outStat.session
        >> outStat.tty_nr >> outStat.tpgid >> outStat.flags >> outStat.minflt
        >> outStat.cminflt >> outStat.majflt >> outStat.cmajflt >> outStat.utime
        >> outStat.stime >> outStat.cutime >> outStat.cstime >> outStat.priority
        >> outStat.nice >> outStat.num_threads;

    // Skip itrealvalue
    uint64_t dummy = 0;
    iss >> dummy >> outStat.starttime >> outStat.vsize >> outStat.rss;

    return true;
}

std::string ProcReader::readProcessCmdline(int pid, bool mock) {
    if (mock || !isProcAvailable()) {
        return "/usr/bin/proc_" + std::to_string(pid);
    }

    std::string path = "/proc/" + std::to_string(pid) + "/cmdline";
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return "";

    std::string cmd;
    char c;
    while (file.get(c)) {
        if (c == '\0') {
            cmd += ' ';
        } else {
            cmd += c;
        }
    }
    if (!cmd.empty() && cmd.back() == ' ') {
        cmd.pop_back();
    }
    return cmd;
}

std::string ProcReader::readProcessUser(int pid, bool mock) {
    if (mock || !isProcAvailable()) {
        return (pid < 500) ? "root" : "ubuntu";
    }

    std::string path = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream file(path);
    if (!file.is_open()) return "unknown";

    std::string line;
    uint32_t uid = 0;
    while (std::getline(file, line)) {
        if (line.rfind("Uid:", 0) == 0) {
            std::istringstream iss(line.substr(4));
            iss >> uid;
            break;
        }
    }

#if !defined(_WIN32)
    struct passwd* pw = getpwuid(uid);
    if (pw) {
        return std::string(pw->pw_name);
    }
#endif
    return std::to_string(uid);
}

std::unordered_map<std::string, std::pair<uint64_t, uint64_t>> ProcReader::readDiskStats(bool mock) {
    std::unordered_map<std::string, std::pair<uint64_t, uint64_t>> res;
    if (mock || !isProcAvailable()) {
        res["sda"] = {15000000 + mockTickCounter_ * 200, 8000000 + mockTickCounter_ * 120};
        return res;
    }

    std::ifstream file("/proc/diskstats");
    if (!file.is_open()) return res;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        int major, minor;
        std::string devName;
        uint64_t readsSuccess, readsMerged, sectorsRead, timeReading;
        uint64_t writesSuccess, writesMerged, sectorsWritten;

        if (iss >> major >> minor >> devName >> readsSuccess >> readsMerged
                >> sectorsRead >> timeReading >> writesSuccess >> writesMerged
                >> sectorsWritten) {
            // Keep block devices like sda, vda, nvme
            if (devName.rfind("sd", 0) == 0 || devName.rfind("nvme", 0) == 0 || devName.rfind("vd", 0) == 0) {
                res[devName] = {sectorsRead, sectorsWritten};
            }
        }
    }
    return res;
}

std::unordered_map<std::string, std::pair<uint64_t, uint64_t>> ProcReader::readNetDev(bool mock) {
    std::unordered_map<std::string, std::pair<uint64_t, uint64_t>> net;
    if (mock || !isProcAvailable()) {
        net["eth0"] = {50000000 + mockTickCounter_ * 1024, 25000000 + mockTickCounter_ * 512};
        net["lo"]   = {1000000 + mockTickCounter_ * 100, 1000000 + mockTickCounter_ * 100};
        return net;
    }

    std::ifstream file("/proc/net/dev");
    if (!file.is_open()) return net;

    std::string line;
    // Skip 2 header lines
    std::getline(file, line);
    std::getline(file, line);

    while (std::getline(file, line)) {
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string iface = line.substr(0, colon);
            // Trim whitespace
            iface.erase(0, iface.find_first_not_of(" \t"));
            iface.erase(iface.find_last_not_of(" \t") + 1);

            std::istringstream iss(line.substr(colon + 1));
            uint64_t rxBytes, rxPackets, rxErrs, rxDrop, rxFifo, rxFrame, rxCompressed, rxMulticast;
            uint64_t txBytes;

            if (iss >> rxBytes >> rxPackets >> rxErrs >> rxDrop >> rxFifo
                    >> rxFrame >> rxCompressed >> rxMulticast >> txBytes) {
                net[iface] = {rxBytes, txBytes};
            }
        }
    }
    return net;
}

void ProcReader::readLoadAvg(double& load1, double& load5, double& load15, bool mock) {
    if (mock || !isProcAvailable()) {
        load1 = 0.65;
        load5 = 0.52;
        load15 = 0.48;
        return;
    }

    std::ifstream file("/proc/loadavg");
    if (file.is_open()) {
        file >> load1 >> load5 >> load15;
    } else {
        load1 = load5 = load15 = 0.0;
    }
}

double ProcReader::readUptime(bool mock) {
    if (mock || !isProcAvailable()) {
        return 3600.0 * 24.5 + mockTickCounter_;
    }

    std::ifstream file("/proc/uptime");
    double uptime = 0.0;
    if (file.is_open()) {
        file >> uptime;
    }
    return uptime;
}

uint64_t ProcReader::readContextSwitches(bool mock) {
    if (mock || !isProcAvailable()) {
        return 45000000 + mockTickCounter_ * 850;
    }

    std::ifstream file("/proc/stat");
    if (!file.is_open()) return 0;

    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("ctxt", 0) == 0) {
            std::istringstream iss(line.substr(4));
            uint64_t ctxt = 0;
            iss >> ctxt;
            return ctxt;
        }
    }
    return 0;
}

uint64_t ProcReader::readInterrupts(bool mock) {
    if (mock || !isProcAvailable()) {
        return 18000000 + mockTickCounter_ * 420;
    }

    std::ifstream file("/proc/stat");
    if (!file.is_open()) return 0;

    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("intr", 0) == 0) {
            std::istringstream iss(line.substr(4));
            uint64_t intr = 0;
            iss >> intr;
            return intr;
        }
    }
    return 0;
}

} // namespace utils
} // namespace sysmon
