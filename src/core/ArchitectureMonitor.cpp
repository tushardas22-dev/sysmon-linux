#include "core/ArchitectureMonitor.hpp"
#include "utils/ProcReader.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace sysmon {
namespace core {

ArchitectureMonitor::ArchitectureMonitor() {
    snapshot_.architecture = "x86_64";
    snapshot_.virtualizationType = "Bare Metal / Unknown";
}

void ArchitectureMonitor::update(bool mock) {
    if (!staticInfoInitialized_) {
        detectCpuInfo(mock);
        detectCacheHierarchy(mock);
        detectVirtualization();
        staticInfoInitialized_ = true;
    }

    detectFrequencies(mock);
    detectThermal(mock);
}

void ArchitectureMonitor::detectVirtualization() {
    std::string ver = utils::ProcReader::readFileContents("/proc/version");
    std::transform(ver.begin(), ver.end(), ver.begin(), ::tolower);

    if (ver.find("microsoft") != std::string::npos || ver.find("wsl") != std::string::npos) {
        snapshot_.virtualizationType = "WSL2 (Windows Subsystem for Linux)";
    } else if (ver.find("kvm") != std::string::npos) {
        snapshot_.virtualizationType = "KVM Hypervisor";
    } else if (ver.find("vmware") != std::string::npos) {
        snapshot_.virtualizationType = "VMware Virtual Machine";
    } else {
        snapshot_.virtualizationType = "Native Linux";
    }
}

void ArchitectureMonitor::detectCpuInfo(bool mock) {
    if (mock || !utils::ProcReader::isProcAvailable()) {
        snapshot_.vendorId = "GenuineIntel";
        snapshot_.modelName = "Intel(R) Core(TM) i7-12700H @ 2.70GHz";
        snapshot_.architecture = "x86_64";
        snapshot_.cpuFamily = 6;
        snapshot_.model = 154;
        snapshot_.stepping = 3;
        snapshot_.virtualizationType = "WSL2 (Windows Subsystem for Linux)";
        snapshot_.featureFlags = {"fpu", "vme", "de", "pse", "tsc", "msr", "pae", "mce",
                                  "cx8", "apic", "sep", "mtrr", "pge", "mca", "cmov", "pat",
                                  "sse", "sse2", "ss", "ht", "tm", "pbe", "syscall", "nx",
                                  "lm", "sse3", "ssse3", "fma", "cx16", "sse4_1", "sse4_2",
                                  "avx", "avx2", "aes", "rdrand"};
        return;
    }

    std::ifstream file("/proc/cpuinfo");
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string key = line.substr(0, colon);
        std::string val = line.substr(colon + 1);
        key.erase(key.find_last_not_of(" \t") + 1);
        val.erase(0, val.find_first_not_of(" \t"));

        if (key == "vendor_id" && snapshot_.vendorId.empty()) {
            snapshot_.vendorId = val;
        } else if (key == "model name" && snapshot_.modelName.empty()) {
            snapshot_.modelName = val;
        } else if (key == "cpu family" && snapshot_.cpuFamily == 0) {
            snapshot_.cpuFamily = std::atoi(val.c_str());
        } else if (key == "model" && snapshot_.model == 0) {
            snapshot_.model = std::atoi(val.c_str());
        } else if (key == "stepping" && snapshot_.stepping == 0) {
            snapshot_.stepping = std::atoi(val.c_str());
        } else if (key == "flags" && snapshot_.featureFlags.empty()) {
            std::istringstream iss(val);
            std::string flag;
            while (iss >> flag) {
                snapshot_.featureFlags.push_back(flag);
            }
        }
    }
}

void ArchitectureMonitor::detectCacheHierarchy(bool mock) {
    if (mock || !utils::ProcReader::isProcAvailable()) {
        snapshot_.caches.clear();
        snapshot_.caches.push_back({1, "Data", "48K", 48 * 1024, 12, 64});
        snapshot_.caches.push_back({1, "Instruction", "32K", 32 * 1024, 8, 64});
        snapshot_.caches.push_back({2, "Unified", "1280K", 1280 * 1024, 10, 64});
        snapshot_.caches.push_back({3, "Unified", "24576K", 24576 * 1024, 16, 64});
        return;
    }

    snapshot_.caches.clear();
    std::string basePath = "/sys/devices/system/cpu/cpu0/cache";
    if (!std::filesystem::exists(basePath)) {
        // Fallback for environments where sysfs cache index is masked
        snapshot_.caches.push_back({1, "Data", "32K", 32 * 1024, 8, 64});
        snapshot_.caches.push_back({1, "Instruction", "32K", 32 * 1024, 8, 64});
        snapshot_.caches.push_back({2, "Unified", "512K", 512 * 1024, 8, 64});
        snapshot_.caches.push_back({3, "Unified", "16384K", 16384 * 1024, 16, 64});
        return;
    }

    for (int idx = 0; idx < 10; ++idx) {
        std::string dir = basePath + "/index" + std::to_string(idx);
        if (!std::filesystem::exists(dir)) break;

        CacheLevelInfo info;
        std::string lvlStr = utils::ProcReader::readFileContents(dir + "/level");
        std::string typStr = utils::ProcReader::readFileContents(dir + "/type");
        std::string szStr  = utils::ProcReader::readFileContents(dir + "/size");
        std::string wayStr = utils::ProcReader::readFileContents(dir + "/ways_of_associativity");
        std::string lineSz = utils::ProcReader::readFileContents(dir + "/coherency_line_size");

        if (!lvlStr.empty()) info.level = std::atoi(lvlStr.c_str());
        if (!typStr.empty()) {
            if (!typStr.empty() && typStr.back() == '\n') typStr.pop_back();
            info.type = typStr;
        }
        if (!szStr.empty()) {
            if (!szStr.empty() && szStr.back() == '\n') szStr.pop_back();
            info.sizeStr = szStr;
        }
        if (!wayStr.empty()) info.associativity = std::atoi(wayStr.c_str());
        if (!lineSz.empty()) info.lineSize = std::atoi(lineSz.c_str());

        snapshot_.caches.push_back(info);
    }
}

void ArchitectureMonitor::detectFrequencies(bool mock) {
    if (mock || !utils::ProcReader::isProcAvailable()) {
        snapshot_.frequencies.clear();
        for (int i = 0; i < 4; ++i) {
            CoreFrequencyInfo info;
            info.coreId = i;
            info.curFreqKhz = 2400000 + (i * 120000);
            info.minFreqKhz = 800000;
            info.maxFreqKhz = 4700000;
            snapshot_.frequencies.push_back(info);
        }
        return;
    }

    snapshot_.frequencies.clear();
    // Scan /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq
    for (int i = 0; i < 64; ++i) {
        std::string freqPath = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/scaling_cur_freq";
        if (!std::filesystem::exists(freqPath)) break;

        CoreFrequencyInfo info;
        info.coreId = i;
        std::string cur = utils::ProcReader::readFileContents(freqPath);
        std::string min = utils::ProcReader::readFileContents("/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/scaling_min_freq");
        std::string max = utils::ProcReader::readFileContents("/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/scaling_max_freq");

        if (!cur.empty()) info.curFreqKhz = std::strtoull(cur.c_str(), nullptr, 10);
        if (!min.empty()) info.minFreqKhz = std::strtoull(min.c_str(), nullptr, 10);
        if (!max.empty()) info.maxFreqKhz = std::strtoull(max.c_str(), nullptr, 10);

        snapshot_.frequencies.push_back(info);
    }

    // Fallback if cpufreq is not exposed (e.g. standard WSL2 kernel)
    if (snapshot_.frequencies.empty()) {
        CoreFrequencyInfo info;
        info.coreId = 0;
        info.curFreqKhz = 2600000;
        info.minFreqKhz = 1000000;
        info.maxFreqKhz = 4200000;
        snapshot_.frequencies.push_back(info);
    }
}

void ArchitectureMonitor::detectThermal(bool mock) {
    if (mock || !utils::ProcReader::isProcAvailable()) {
        snapshot_.cpuTemperatureCelsius = 48.5;
        snapshot_.hasTemperatureSensor = true;
        return;
    }

    std::string tempPath = "/sys/class/thermal/thermal_zone0/temp";
    if (std::filesystem::exists(tempPath)) {
        std::string tempStr = utils::ProcReader::readFileContents(tempPath);
        if (!tempStr.empty()) {
            double milliC = std::atof(tempStr.c_str());
            snapshot_.cpuTemperatureCelsius = milliC / 1000.0;
            snapshot_.hasTemperatureSensor = true;
            return;
        }
    }
    snapshot_.hasTemperatureSensor = false;
}

const ArchitectureSnapshot& ArchitectureMonitor::getSnapshot() const {
    return snapshot_;
}

} // namespace core
} // namespace sysmon
