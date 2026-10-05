#ifndef ARCHITECTURE_MONITOR_HPP
#define ARCHITECTURE_MONITOR_HPP

#include <string>
#include <vector>
#include <cstdint>

namespace sysmon {
namespace core {

struct CacheLevelInfo {
    int level = 1;
    std::string type;       // "Data", "Instruction", "Unified"
    std::string sizeStr;    // e.g. "32K", "512K", "16M"
    uint64_t sizeBytes = 0;
    int associativity = 0;
    int lineSize = 0;
};

struct CoreFrequencyInfo {
    int coreId = 0;
    uint64_t curFreqKhz = 0;
    uint64_t minFreqKhz = 0;
    uint64_t maxFreqKhz = 0;
};

struct ArchitectureSnapshot {
    std::string vendorId;
    std::string modelName;
    std::string architecture;
    int cpuFamily = 0;
    int model = 0;
    int stepping = 0;
    std::string virtualizationType;
    std::vector<std::string> featureFlags;

    std::vector<CacheLevelInfo> caches;
    std::vector<CoreFrequencyInfo> frequencies;
    double cpuTemperatureCelsius = 0.0;
    bool hasTemperatureSensor = false;
};

class ArchitectureMonitor {
public:
    ArchitectureMonitor();

    void update(bool mock = false);
    const ArchitectureSnapshot& getSnapshot() const;

private:
    void detectCpuInfo(bool mock);
    void detectCacheHierarchy(bool mock);
    void detectFrequencies(bool mock);
    void detectThermal(bool mock);
    void detectVirtualization();

    ArchitectureSnapshot snapshot_;
    bool staticInfoInitialized_ = false;
};

} // namespace core
} // namespace sysmon

#endif // ARCHITECTURE_MONITOR_HPP
