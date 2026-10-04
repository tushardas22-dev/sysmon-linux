#ifndef KERNEL_DRIVER_CLIENT_HPP
#define KERNEL_DRIVER_CLIENT_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

#include "../../driver/sysmon_driver.h"

namespace sysmon {
namespace driver {

struct DriverStatus {
    bool connected = false;
    std::string devicePath = "/dev/sysmon_driver";
    std::string message;
    uint64_t successfulIoctls = 0;
    uint64_t failedIoctls = 0;
};

class KernelDriverClient {
public:
    KernelDriverClient(const std::string& devPath = "/dev/sysmon_driver");
    ~KernelDriverClient();

    bool initialize();
    void close();
    bool isConnected() const;

    bool queryMetrics(struct sysmon_kernel_metrics& outMetrics, bool mockFallback = true);
    bool queryProcessTable(struct sysmon_proc_table& outTable, bool mockFallback = true);
    bool resetDriverStats();

    const DriverStatus& getStatus() const;

private:
    std::string devPath_;
    int fd_ = -1;
    DriverStatus status_;
};

} // namespace driver
} // namespace sysmon

#endif // KERNEL_DRIVER_CLIENT_HPP
