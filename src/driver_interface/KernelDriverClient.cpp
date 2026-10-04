#include "driver_interface/KernelDriverClient.hpp"
#include "utils/Logger.hpp"
#include <cstring>
#include <chrono>

#if !defined(_WIN32)
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#endif

namespace sysmon {
namespace driver {

KernelDriverClient::KernelDriverClient(const std::string& devPath)
    : devPath_(devPath) {
    status_.devicePath = devPath_;
    status_.connected = false;
    status_.message = "Driver client initialized, not yet connected.";
}

KernelDriverClient::~KernelDriverClient() {
    close();
}

bool KernelDriverClient::initialize() {
#if !defined(_WIN32)
    fd_ = ::open(devPath_.c_str(), O_RDWR);
    if (fd_ < 0) {
        // Try read-only
        fd_ = ::open(devPath_.c_str(), O_RDONLY);
    }

    if (fd_ >= 0) {
        status_.connected = true;
        status_.message = "Connected to kernel device node " + devPath_;
        utils::Logger::getInstance().info("Successfully connected to " + devPath_);
        return true;
    } else {
        status_.connected = false;
        status_.message = "Failed to open " + devPath_ + " (errno: " + std::to_string(errno) + " - " + strerror(errno) + "). Falling back to user-space /proc.";
        utils::Logger::getInstance().warn(status_.message);
        return false;
    }
#else
    status_.connected = false;
    status_.message = "Non-Linux environment detected. Kernel module IOCTL disabled.";
    return false;
#endif
}

void KernelDriverClient::close() {
#if !defined(_WIN32)
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
#endif
    status_.connected = false;
}

bool KernelDriverClient::isConnected() const {
    return status_.connected;
}

bool KernelDriverClient::queryMetrics(struct sysmon_kernel_metrics& outMetrics, bool mockFallback) {
#if !defined(_WIN32)
    if (fd_ >= 0) {
        if (::ioctl(fd_, SYSMON_IOCTL_GET_METRICS, &outMetrics) == 0) {
            status_.successfulIoctls++;
            return true;
        }
        status_.failedIoctls++;
    }
#endif

    if (mockFallback) {
        // Provide mock metrics matching kernel driver struct
        std::memset(&outMetrics, 0, sizeof(outMetrics));
        outMetrics.uptime_seconds = 86400;
        outMetrics.total_ram_bytes = 16ULL * 1024 * 1024 * 1024;
        outMetrics.free_ram_bytes = 4ULL * 1024 * 1024 * 1024;
        outMetrics.shared_ram_bytes = 256 * 1024 * 1024;
        outMetrics.buffer_ram_bytes = 512 * 1024 * 1024;
        outMetrics.total_swap_bytes = 4ULL * 1024 * 1024 * 1024;
        outMetrics.free_swap_bytes = 3ULL * 1024 * 1024 * 1024;
        outMetrics.process_count = 142;
        outMetrics.active_cpus = 8;
        outMetrics.context_switches = 5420000;
        outMetrics.total_forks = 89400;
        outMetrics.interrupts_total = 1200000;
        outMetrics.load_avg[0] = static_cast<uint32_t>(0.75 * 65536);
        outMetrics.load_avg[1] = static_cast<uint32_t>(0.55 * 65536);
        outMetrics.load_avg[2] = static_cast<uint32_t>(0.45 * 65536);
        outMetrics.sample_timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return true;
    }

    return false;
}

bool KernelDriverClient::queryProcessTable(struct sysmon_proc_table& outTable, bool mockFallback) {
#if !defined(_WIN32)
    if (fd_ >= 0) {
        if (::ioctl(fd_, SYSMON_IOCTL_GET_PROCS, &outTable) == 0) {
            status_.successfulIoctls++;
            return true;
        }
        status_.failedIoctls++;
    }
#endif

    if (mockFallback) {
        std::memset(&outTable, 0, sizeof(outTable));
        outTable.count = 5;

        outTable.entries[0].pid = 1;
        outTable.entries[0].ppid = 0;
        outTable.entries[0].uid = 0;
        outTable.entries[0].state = 0;
        outTable.entries[0].priority = 20;
        outTable.entries[0].rss_pages = 2500;
        std::strncpy(outTable.entries[0].comm, "systemd", COMM_LEN - 1);

        outTable.entries[1].pid = 2;
        outTable.entries[1].ppid = 0;
        outTable.entries[1].uid = 0;
        outTable.entries[1].state = 1;
        outTable.entries[1].priority = 0;
        outTable.entries[1].rss_pages = 0;
        std::strncpy(outTable.entries[1].comm, "kthreadd", COMM_LEN - 1);

        outTable.entries[2].pid = 412;
        outTable.entries[2].ppid = 1;
        outTable.entries[2].uid = 1000;
        outTable.entries[2].state = 0;
        outTable.entries[2].priority = 20;
        outTable.entries[2].rss_pages = 8200;
        std::strncpy(outTable.entries[2].comm, "sysmon", COMM_LEN - 1);

        outTable.entries[3].pid = 620;
        outTable.entries[3].ppid = 1;
        outTable.entries[3].uid = 0;
        outTable.entries[3].state = 1;
        outTable.entries[3].priority = 20;
        outTable.entries[3].rss_pages = 1800;
        std::strncpy(outTable.entries[3].comm, "sshd", COMM_LEN - 1);

        outTable.entries[4].pid = 1050;
        outTable.entries[4].ppid = 1;
        outTable.entries[4].uid = 1000;
        outTable.entries[4].state = 1;
        outTable.entries[4].priority = 20;
        outTable.entries[4].rss_pages = 4500;
        std::strncpy(outTable.entries[4].comm, "bash", COMM_LEN - 1);

        return true;
    }

    return false;
}

bool KernelDriverClient::resetDriverStats() {
#if !defined(_WIN32)
    if (fd_ >= 0) {
        return (::ioctl(fd_, SYSMON_IOCTL_RESET_STATS) == 0);
    }
#endif
    return false;
}

const DriverStatus& KernelDriverClient::getStatus() const {
    return status_;
}

} // namespace driver
} // namespace sysmon
