#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <cstdint>

namespace sysmon {
namespace utils {

enum class ProcessSortField {
    CPU,
    MEMORY,
    PID,
    NAME
};

struct Config {
    uint32_t refreshIntervalMs = 1000;
    size_t maxProcessesDisplayed = 20;
    ProcessSortField sortField = ProcessSortField::CPU;
    bool sortAscending = false;
    bool useKernelDriver = true;
    bool mockMode = false;
    std::string logFilePath = "sysmon.log";
    std::string logLevel = "INFO";
    bool headless = false;

    static Config parseCommandLine(int argc, char* argv[]);
    static void printHelp(const char* progName);
};

} // namespace utils
} // namespace sysmon

#endif // CONFIG_HPP
