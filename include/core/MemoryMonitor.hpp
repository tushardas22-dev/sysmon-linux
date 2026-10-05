#ifndef MEMORY_MONITOR_HPP
#define MEMORY_MONITOR_HPP

#include <cstdint>
#include <deque>

namespace sysmon {
namespace core {

struct MemorySnapshot {
    uint64_t totalRamBytes = 0;
    uint64_t freeRamBytes = 0;
    uint64_t availableRamBytes = 0;
    uint64_t usedRamBytes = 0;
    uint64_t buffersBytes = 0;
    uint64_t cachedBytes = 0;
    uint64_t dirtyBytes = 0;
    double ramUsagePercent = 0.0;

    uint64_t totalSwapBytes = 0;
    uint64_t freeSwapBytes = 0;
    uint64_t usedSwapBytes = 0;
    double swapUsagePercent = 0.0;

    uint64_t pageFaultsTotal = 0;
    uint64_t pageFaultsDelta = 0;
    uint64_t majorPageFaultsTotal = 0;
    uint64_t majorPageFaultsDelta = 0;

    std::deque<double> historySparkline;
};

class MemoryMonitor {
public:
    MemoryMonitor();

    void update(bool mock = false);
    const MemorySnapshot& getSnapshot() const;

private:
    MemorySnapshot snapshot_;
    uint64_t prevPgFault_ = 0;
    uint64_t prevPgMajFault_ = 0;
    bool isFirstSample_ = true;
};

} // namespace core
} // namespace sysmon

#endif // MEMORY_MONITOR_HPP
