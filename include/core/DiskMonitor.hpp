#ifndef DISK_MONITOR_HPP
#define DISK_MONITOR_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace sysmon {
namespace core {

struct DiskDeviceMetrics {
    std::string deviceName;
    uint64_t readBytesTotal = 0;
    uint64_t writeBytesTotal = 0;
    double readRateBytesSec = 0.0;
    double writeRateBytesSec = 0.0;
};

struct FilesystemMetrics {
    std::string mountPoint;
    uint64_t totalBytes = 0;
    uint64_t freeBytes = 0;
    uint64_t usedBytes = 0;
    double usedPercent = 0.0;
};

struct DiskSnapshot {
    std::vector<DiskDeviceMetrics> devices;
    std::vector<FilesystemMetrics> filesystems;
    double totalReadRateBytesSec = 0.0;
    double totalWriteRateBytesSec = 0.0;
};

class DiskMonitor {
public:
    DiskMonitor();

    void update(double elapsedSeconds, bool mock = false);
    const DiskSnapshot& getSnapshot() const;

private:
    void updateFilesystems(bool mock);

    DiskSnapshot snapshot_;
    std::unordered_map<std::string, std::pair<uint64_t, uint64_t>> prevSectors_;
    bool isFirstSample_ = true;
};

} // namespace core
} // namespace sysmon

#endif // DISK_MONITOR_HPP
