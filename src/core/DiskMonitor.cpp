#include "core/DiskMonitor.hpp"
#include "utils/ProcReader.hpp"

#if !defined(_WIN32)
#include <sys/statvfs.h>
#endif

namespace sysmon {
namespace core {

DiskMonitor::DiskMonitor() = default;

void DiskMonitor::update(double elapsedSeconds, bool mock) {
    if (elapsedSeconds <= 0.0) elapsedSeconds = 1.0;

    auto currentSectors = utils::ProcReader::readDiskStats(mock);
    snapshot_.devices.clear();
    snapshot_.totalReadRateBytesSec = 0.0;
    snapshot_.totalWriteRateBytesSec = 0.0;

    for (const auto& [dev, pair] : currentSectors) {
        DiskDeviceMetrics m;
        m.deviceName = dev;
        // Standard Linux sector size is 512 bytes
        m.readBytesTotal = pair.first * 512ULL;
        m.writeBytesTotal = pair.second * 512ULL;

        if (!isFirstSample_ && prevSectors_.find(dev) != prevSectors_.end()) {
            uint64_t prevR = prevSectors_[dev].first * 512ULL;
            uint64_t prevW = prevSectors_[dev].second * 512ULL;

            uint64_t deltaR = (m.readBytesTotal >= prevR) ? (m.readBytesTotal - prevR) : 0;
            uint64_t deltaW = (m.writeBytesTotal >= prevW) ? (m.writeBytesTotal - prevW) : 0;

            m.readRateBytesSec = static_cast<double>(deltaR) / elapsedSeconds;
            m.writeRateBytesSec = static_cast<double>(deltaW) / elapsedSeconds;

            snapshot_.totalReadRateBytesSec += m.readRateBytesSec;
            snapshot_.totalWriteRateBytesSec += m.writeRateBytesSec;
        }

        snapshot_.devices.push_back(m);
    }

    prevSectors_ = currentSectors;
    isFirstSample_ = false;

    updateFilesystems(mock);
}

void DiskMonitor::updateFilesystems(bool mock) {
    snapshot_.filesystems.clear();

    if (mock || !utils::ProcReader::isProcAvailable()) {
        FilesystemMetrics fs1;
        fs1.mountPoint = "/";
        fs1.totalBytes = 256ULL * 1024 * 1024 * 1024;
        fs1.freeBytes  = 140ULL * 1024 * 1024 * 1024;
        fs1.usedBytes  = fs1.totalBytes - fs1.freeBytes;
        fs1.usedPercent = ((double)fs1.usedBytes / (double)fs1.totalBytes) * 100.0;
        snapshot_.filesystems.push_back(fs1);
        return;
    }

#if !defined(_WIN32)
    std::vector<std::string> mounts = {"/", "/home", "/var"};
    for (const auto& mount : mounts) {
        struct statvfs s;
        if (statvfs(mount.c_str(), &s) == 0) {
            FilesystemMetrics fs;
            fs.mountPoint = mount;
            fs.totalBytes = static_cast<uint64_t>(s.f_blocks) * s.f_frsize;
            fs.freeBytes  = static_cast<uint64_t>(s.f_bavail) * s.f_frsize;
            fs.usedBytes  = (fs.totalBytes >= fs.freeBytes) ? (fs.totalBytes - fs.freeBytes) : 0;
            if (fs.totalBytes > 0) {
                fs.usedPercent = ((double)fs.usedBytes / (double)fs.totalBytes) * 100.0;
                snapshot_.filesystems.push_back(fs);
            }
        }
    }
#endif
}

const DiskSnapshot& DiskMonitor::getSnapshot() const {
    return snapshot_;
}

} // namespace core
} // namespace sysmon
