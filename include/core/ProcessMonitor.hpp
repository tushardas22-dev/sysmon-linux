#ifndef PROCESS_MONITOR_HPP
#define PROCESS_MONITOR_HPP

#include "utils/Config.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace sysmon {
namespace core {

struct ProcessInfo {
    int pid = 0;
    int ppid = 0;
    std::string user;
    std::string name;
    std::string cmdline;
    char state = 'S';
    double cpuPercent = 0.0;
    double memPercent = 0.0;
    uint64_t rssBytes = 0;
    uint64_t vmsBytes = 0;
    long numThreads = 1;
    long priority = 0;
    long nice = 0;
    uint64_t totalTicks = 0;
};

struct ProcessSnapshot {
    std::vector<ProcessInfo> processes;
    size_t totalProcesses = 0;
    size_t runningProcesses = 0;
    size_t sleepingProcesses = 0;
    size_t zombieProcesses = 0;
};

class ProcessMonitor {
public:
    ProcessMonitor();

    void update(uint64_t totalSysDeltaTicks, uint64_t totalRamBytes, bool mock = false);
    void sortProcesses(utils::ProcessSortField field, bool ascending = false);
    std::vector<ProcessInfo> filterByName(const std::string& query) const;
    const ProcessSnapshot& getSnapshot() const;
    bool killProcess(int pid, int signal = 15); // Default SIGTERM

private:
    struct ProcTickHistory {
        uint64_t utime = 0;
        uint64_t stime = 0;
    };

    ProcessSnapshot snapshot_;
    std::unordered_map<int, ProcTickHistory> prevTicks_;
    bool isFirstSample_ = true;
};

} // namespace core
} // namespace sysmon

#endif // PROCESS_MONITOR_HPP
