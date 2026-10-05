#include "core/MemoryMonitor.hpp"
#include "utils/ProcReader.hpp"
#include <algorithm>

namespace sysmon {
namespace core {

MemoryMonitor::MemoryMonitor() {
    snapshot_.historySparkline.assign(30, 0.0);
}

void MemoryMonitor::update(bool mock) {
    auto memInfo = utils::ProcReader::readMemInfo(mock);
    auto vmStat = utils::ProcReader::readVmStat(mock);

    if (memInfo.find("MemTotal") != memInfo.end()) {
        snapshot_.totalRamBytes = memInfo["MemTotal"] * 1024ULL;
        snapshot_.freeRamBytes = memInfo["MemFree"] * 1024ULL;

        if (memInfo.find("MemAvailable") != memInfo.end()) {
            snapshot_.availableRamBytes = memInfo["MemAvailable"] * 1024ULL;
            snapshot_.usedRamBytes = (snapshot_.totalRamBytes > snapshot_.availableRamBytes) ?
                                     (snapshot_.totalRamBytes - snapshot_.availableRamBytes) : 0;
        } else {
            uint64_t buffers = memInfo["Buffers"] * 1024ULL;
            uint64_t cached = memInfo["Cached"] * 1024ULL;
            snapshot_.usedRamBytes = snapshot_.totalRamBytes - (snapshot_.freeRamBytes + buffers + cached);
            snapshot_.availableRamBytes = snapshot_.freeRamBytes + buffers + cached;
        }

        snapshot_.buffersBytes = memInfo["Buffers"] * 1024ULL;
        snapshot_.cachedBytes = memInfo["Cached"] * 1024ULL;
        snapshot_.dirtyBytes = memInfo["Dirty"] * 1024ULL;

        if (snapshot_.totalRamBytes > 0) {
            snapshot_.ramUsagePercent = ((double)snapshot_.usedRamBytes / (double)snapshot_.totalRamBytes) * 100.0;
        }
    }

    if (memInfo.find("SwapTotal") != memInfo.end()) {
        snapshot_.totalSwapBytes = memInfo["SwapTotal"] * 1024ULL;
        snapshot_.freeSwapBytes = memInfo["SwapFree"] * 1024ULL;
        snapshot_.usedSwapBytes = (snapshot_.totalSwapBytes >= snapshot_.freeSwapBytes) ?
                                  (snapshot_.totalSwapBytes - snapshot_.freeSwapBytes) : 0;

        if (snapshot_.totalSwapBytes > 0) {
            snapshot_.swapUsagePercent = ((double)snapshot_.usedSwapBytes / (double)snapshot_.totalSwapBytes) * 100.0;
        } else {
            snapshot_.swapUsagePercent = 0.0;
        }
    }

    // Page fault metrics from /proc/vmstat
    if (vmStat.find("pgfault") != vmStat.end()) {
        uint64_t curFault = vmStat["pgfault"];
        uint64_t curMajFault = vmStat["pgmajfault"];

        if (!isFirstSample_) {
            snapshot_.pageFaultsDelta = (curFault >= prevPgFault_) ? (curFault - prevPgFault_) : 0;
            snapshot_.majorPageFaultsDelta = (curMajFault >= prevPgMajFault_) ? (curMajFault - prevPgMajFault_) : 0;
        }
        snapshot_.pageFaultsTotal = curFault;
        snapshot_.majorPageFaultsTotal = curMajFault;
        prevPgFault_ = curFault;
        prevPgMajFault_ = curMajFault;
        isFirstSample_ = false;
    }

    snapshot_.historySparkline.push_back(snapshot_.ramUsagePercent);
    if (snapshot_.historySparkline.size() > 30) {
        snapshot_.historySparkline.pop_front();
    }
}

const MemorySnapshot& MemoryMonitor::getSnapshot() const {
    return snapshot_;
}

} // namespace core
} // namespace sysmon
