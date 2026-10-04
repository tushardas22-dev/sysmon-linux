#ifndef PROC_READER_HPP
#define PROC_READER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace sysmon {
namespace utils {

struct CpuStatRaw {
    std::string cpuName;
    uint64_t user = 0;
    uint64_t nice = 0;
    uint64_t system = 0;
    uint64_t idle = 0;
    uint64_t iowait = 0;
    uint64_t irq = 0;
    uint64_t softirq = 0;
    uint64_t steal = 0;
    uint64_t guest = 0;
    uint64_t guestNice = 0;
};

struct ProcPidStatRaw {
    int pid = 0;
    std::string comm;
    char state = 'S';
    int ppid = 0;
    int pgrp = 0;
    int session = 0;
    int tty_nr = 0;
    int tpgid = 0;
    unsigned int flags = 0;
    uint64_t minflt = 0;
    uint64_t cminflt = 0;
    uint64_t majflt = 0;
    uint64_t cmajflt = 0;
    uint64_t utime = 0;
    uint64_t stime = 0;
    int64_t cutime = 0;
    int64_t cstime = 0;
    long priority = 0;
    long nice = 0;
    long num_threads = 0;
    uint64_t starttime = 0;
    uint64_t vsize = 0;
    int64_t rss = 0;
};

class ProcReader {
public:
    static bool isProcAvailable();

    static std::string readFileContents(const std::string& path);
    static std::vector<std::string> readFileLines(const std::string& path);

    // Specific proc parsers
    static std::vector<CpuStatRaw> readCpuStats(bool mock = false);
    static std::unordered_map<std::string, uint64_t> readMemInfo(bool mock = false);
    static std::unordered_map<std::string, uint64_t> readVmStat(bool mock = false);
    static std::vector<std::string> getProcessPids(bool mock = false);
    static bool readProcessStat(int pid, ProcPidStatRaw& outStat, bool mock = false);
    static std::string readProcessCmdline(int pid, bool mock = false);
    static std::string readProcessUser(int pid, bool mock = false);
    static std::unordered_map<std::string, std::pair<uint64_t, uint64_t>> readDiskStats(bool mock = false);
    static std::unordered_map<std::string, std::pair<uint64_t, uint64_t>> readNetDev(bool mock = false);
    static void readLoadAvg(double& load1, double& load5, double& load15, bool mock = false);
    static double readUptime(bool mock = false);
    static uint64_t readContextSwitches(bool mock = false);
    static uint64_t readInterrupts(bool mock = false);

private:
    static uint64_t mockTickCounter_;
};

} // namespace utils
} // namespace sysmon

#endif // PROC_READER_HPP
