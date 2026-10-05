#include "core/ProcessMonitor.hpp"
#include "utils/ProcReader.hpp"
#include "utils/SystemUtils.hpp"
#include <algorithm>
#include <cmath>

#if !defined(_WIN32)
#include <unistd.h>
#endif

namespace sysmon {
namespace core {

ProcessMonitor::ProcessMonitor() = default;

void ProcessMonitor::update(uint64_t totalSysDeltaTicks, uint64_t totalRamBytes, bool mock) {
    auto pids = utils::ProcReader::getProcessPids(mock);

    snapshot_.processes.clear();
    snapshot_.totalProcesses = 0;
    snapshot_.runningProcesses = 0;
    snapshot_.sleepingProcesses = 0;
    snapshot_.zombieProcesses = 0;

    long pageSize = 4096;
#if !defined(_WIN32)
    pageSize = sysconf(_SC_PAGE_SIZE);
    if (pageSize <= 0) pageSize = 4096;
#endif

    std::unordered_map<int, ProcTickHistory> currentTicks;

    for (const auto& pidStr : pids) {
        int pid = std::atoi(pidStr.c_str());
        if (pid <= 0) continue;

        utils::ProcPidStatRaw rawStat;
        if (!utils::ProcReader::readProcessStat(pid, rawStat, mock)) {
            continue;
        }

        ProcessInfo p;
        p.pid = rawStat.pid;
        p.ppid = rawStat.ppid;
        p.name = rawStat.comm;
        p.state = rawStat.state;
        p.priority = rawStat.priority;
        p.nice = rawStat.nice;
        p.numThreads = rawStat.num_threads;
        p.rssBytes = static_cast<uint64_t>(rawStat.rss) * pageSize;
        p.vmsBytes = rawStat.vsize;
        p.cmdline = utils::ProcReader::readProcessCmdline(pid, mock);
        p.user = utils::ProcReader::readProcessUser(pid, mock);
        p.totalTicks = rawStat.utime + rawStat.stime;

        currentTicks[pid] = {rawStat.utime, rawStat.stime};

        // State counts
        if (p.state == 'R') snapshot_.runningProcesses++;
        else if (p.state == 'Z') snapshot_.zombieProcesses++;
        else snapshot_.sleepingProcesses++;

        // Calculate CPU %
        if (!isFirstSample_ && prevTicks_.find(pid) != prevTicks_.end()) {
            const auto& prev = prevTicks_[pid];
            uint64_t curTot = rawStat.utime + rawStat.stime;
            uint64_t prevTot = prev.utime + prev.stime;
            uint64_t deltaProc = (curTot >= prevTot) ? (curTot - prevTot) : 0;

            if (totalSysDeltaTicks > 0) {
                p.cpuPercent = (static_cast<double>(deltaProc) / static_cast<double>(totalSysDeltaTicks)) * 100.0;
            } else {
                p.cpuPercent = 0.0;
            }
        } else {
            p.cpuPercent = 0.0;
        }

        // Calculate Memory %
        if (totalRamBytes > 0) {
            p.memPercent = (static_cast<double>(p.rssBytes) / static_cast<double>(totalRamBytes)) * 100.0;
        }

        snapshot_.processes.push_back(p);
    }

    snapshot_.totalProcesses = snapshot_.processes.size();
    prevTicks_ = currentTicks;
    isFirstSample_ = false;
}

void ProcessMonitor::sortProcesses(utils::ProcessSortField field, bool ascending) {
    auto comp = [field, ascending](const ProcessInfo& a, const ProcessInfo& b) {
        if (field == utils::ProcessSortField::CPU) {
            return ascending ? (a.cpuPercent < b.cpuPercent) : (a.cpuPercent > b.cpuPercent);
        } else if (field == utils::ProcessSortField::MEMORY) {
            return ascending ? (a.rssBytes < b.rssBytes) : (a.rssBytes > b.rssBytes);
        } else if (field == utils::ProcessSortField::PID) {
            return ascending ? (a.pid < b.pid) : (a.pid > b.pid);
        } else {
            return ascending ? (a.name < b.name) : (a.name > b.name);
        }
    };

    std::sort(snapshot_.processes.begin(), snapshot_.processes.end(), comp);
}

std::vector<ProcessInfo> ProcessMonitor::filterByName(const std::string& query) const {
    std::vector<ProcessInfo> filtered;
    for (const auto& p : snapshot_.processes) {
        if (p.name.find(query) != std::string::npos ||
            p.cmdline.find(query) != std::string::npos ||
            std::to_string(p.pid).find(query) != std::string::npos) {
            filtered.push_back(p);
        }
    }
    return filtered;
}

bool ProcessMonitor::killProcess(int pid, int signal) {
    return utils::SystemUtils::sendSignalToProcess(pid, signal);
}

const ProcessSnapshot& ProcessMonitor::getSnapshot() const {
    return snapshot_;
}

} // namespace core
} // namespace sysmon
