#ifndef CPU_MONITOR_HPP
#define CPU_MONITOR_HPP

#include "utils/ProcReader.hpp"
#include <vector>
#include <string>
#include <deque>
#include <cstdint>

namespace sysmon {
namespace core {

struct CpuCoreMetrics {
    std::string name;
    double totalUsagePercent = 0.0;
    double userPercent = 0.0;
    double systemPercent = 0.0;
    double idlePercent = 0.0;
    double iowaitPercent = 0.0;
};

struct CpuSnapshot {
    double totalUsagePercent = 0.0;
    double userPercent = 0.0;
    double systemPercent = 0.0;
    double idlePercent = 0.0;
    double iowaitPercent = 0.0;
    double irqPercent = 0.0;
    double stealPercent = 0.0;
    size_t coreCount = 0;
    std::vector<CpuCoreMetrics> cores;
    uint64_t contextSwitches = 0;
    uint64_t contextSwitchesDelta = 0;
    uint64_t interrupts = 0;
    uint64_t interruptsDelta = 0;
    double load1 = 0.0;
    double load5 = 0.0;
    double load15 = 0.0;
    std::deque<double> historySparkline;
};

class CpuMonitor {
public:
    CpuMonitor();

    void update(bool mock = false);
    const CpuSnapshot& getSnapshot() const;

private:
    void calculateDeltas(const std::vector<utils::CpuStatRaw>& currentRaw);

    std::vector<utils::CpuStatRaw> prevRawStats_;
    CpuSnapshot snapshot_;
    uint64_t prevCtxt_ = 0;
    uint64_t prevIntr_ = 0;
    bool isFirstSample_ = true;
};

} // namespace core
} // namespace sysmon

#endif // CPU_MONITOR_HPP
