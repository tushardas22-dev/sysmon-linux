#include "core/CpuMonitor.hpp"
#include <algorithm>

namespace sysmon {
namespace core {

CpuMonitor::CpuMonitor() {
    snapshot_.historySparkline.assign(30, 0.0);
}

void CpuMonitor::update(bool mock) {
    auto currentRaw = utils::ProcReader::readCpuStats(mock);
    if (currentRaw.empty()) {
        return;
    }

    // Read system metrics
    utils::ProcReader::readLoadAvg(snapshot_.load1, snapshot_.load5, snapshot_.load15, mock);
    uint64_t currentCtxt = utils::ProcReader::readContextSwitches(mock);
    uint64_t currentIntr = utils::ProcReader::readInterrupts(mock);

    if (!isFirstSample_) {
        snapshot_.contextSwitchesDelta = (currentCtxt >= prevCtxt_) ? (currentCtxt - prevCtxt_) : 0;
        snapshot_.interruptsDelta = (currentIntr >= prevIntr_) ? (currentIntr - prevIntr_) : 0;
    }
    snapshot_.contextSwitches = currentCtxt;
    snapshot_.interrupts = currentIntr;
    prevCtxt_ = currentCtxt;
    prevIntr_ = currentIntr;

    calculateDeltas(currentRaw);
    prevRawStats_ = currentRaw;
    isFirstSample_ = false;

    // Update history sparkline
    snapshot_.historySparkline.push_back(snapshot_.totalUsagePercent);
    if (snapshot_.historySparkline.size() > 30) {
        snapshot_.historySparkline.pop_front();
    }
}

void CpuMonitor::calculateDeltas(const std::vector<utils::CpuStatRaw>& currentRaw) {
    if (isFirstSample_ || prevRawStats_.size() != currentRaw.size()) {
        snapshot_.coreCount = (currentRaw.size() > 1) ? currentRaw.size() - 1 : 1;
        snapshot_.cores.resize(snapshot_.coreCount);
        for (size_t i = 0; i < snapshot_.coreCount; ++i) {
            snapshot_.cores[i].name = "cpu" + std::to_string(i);
        }
        return;
    }

    // Calculate total CPU usage (entry 0: "cpu")
    const auto& curTotal = currentRaw[0];
    const auto& prevTotal = prevRawStats_[0];

    uint64_t prevIdle = prevTotal.idle + prevTotal.iowait;
    uint64_t curIdle = curTotal.idle + curTotal.iowait;

    uint64_t prevNonIdle = prevTotal.user + prevTotal.nice + prevTotal.system +
                           prevTotal.irq + prevTotal.softirq + prevTotal.steal;
    uint64_t curNonIdle = curTotal.user + curTotal.nice + curTotal.system +
                          curTotal.irq + curTotal.softirq + curTotal.steal;

    uint64_t prevTotalTicks = prevIdle + prevNonIdle;
    uint64_t curTotalTicks = curIdle + curNonIdle;

    uint64_t totalDelta = (curTotalTicks > prevTotalTicks) ? (curTotalTicks - prevTotalTicks) : 0;
    uint64_t idleDelta = (curIdle > prevIdle) ? (curIdle - prevIdle) : 0;

    if (totalDelta > 0) {
        snapshot_.totalUsagePercent = ((double)(totalDelta > idleDelta ? totalDelta - idleDelta : 0) / (double)totalDelta) * 100.0;
        snapshot_.userPercent = ((double)(curTotal.user - prevTotal.user) / (double)totalDelta) * 100.0;
        snapshot_.systemPercent = ((double)(curTotal.system - prevTotal.system) / (double)totalDelta) * 100.0;
        snapshot_.idlePercent = ((double)idleDelta / (double)totalDelta) * 100.0;
        snapshot_.iowaitPercent = ((double)(curTotal.iowait - prevTotal.iowait) / (double)totalDelta) * 100.0;
        snapshot_.irqPercent = ((double)(curTotal.irq + curTotal.softirq - (prevTotal.irq + prevTotal.softirq)) / (double)totalDelta) * 100.0;
        snapshot_.stealPercent = ((double)(curTotal.steal - prevTotal.steal) / (double)totalDelta) * 100.0;
    }

    // Per-core metrics
    snapshot_.coreCount = currentRaw.size() - 1;
    if (snapshot_.cores.size() != snapshot_.coreCount) {
        snapshot_.cores.resize(snapshot_.coreCount);
    }

    for (size_t i = 1; i < currentRaw.size(); ++i) {
        size_t coreIdx = i - 1;
        const auto& curC = currentRaw[i];
        const auto& prevC = prevRawStats_[i];

        uint64_t cPrevIdle = prevC.idle + prevC.iowait;
        uint64_t cCurIdle = curC.idle + curC.iowait;

        uint64_t cPrevNonIdle = prevC.user + prevC.nice + prevC.system +
                                prevC.irq + prevC.softirq + prevC.steal;
        uint64_t cCurNonIdle = curC.user + curC.nice + curC.system +
                               curC.irq + curC.softirq + curC.steal;

        uint64_t cPrevTot = cPrevIdle + cPrevNonIdle;
        uint64_t cCurTot = cCurIdle + cCurNonIdle;

        uint64_t cTotDelta = (cCurTot > cPrevTot) ? (cCurTot - cPrevTot) : 0;
        uint64_t cIdleDelta = (cCurIdle > cPrevIdle) ? (cCurIdle - cPrevIdle) : 0;

        snapshot_.cores[coreIdx].name = curC.cpuName;
        if (cTotDelta > 0) {
            snapshot_.cores[coreIdx].totalUsagePercent = ((double)(cTotDelta > cIdleDelta ? cTotDelta - cIdleDelta : 0) / (double)cTotDelta) * 100.0;
            snapshot_.cores[coreIdx].userPercent = ((double)(curC.user - prevC.user) / (double)cTotDelta) * 100.0;
            snapshot_.cores[coreIdx].systemPercent = ((double)(curC.system - prevC.system) / (double)cTotDelta) * 100.0;
            snapshot_.cores[coreIdx].idlePercent = ((double)cIdleDelta / (double)cTotDelta) * 100.0;
            snapshot_.cores[coreIdx].iowaitPercent = ((double)(curC.iowait - prevC.iowait) / (double)cTotDelta) * 100.0;
        }
    }
}

const CpuSnapshot& CpuMonitor::getSnapshot() const {
    return snapshot_;
}

} // namespace core
} // namespace sysmon
