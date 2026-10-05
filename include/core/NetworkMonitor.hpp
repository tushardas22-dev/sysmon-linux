#ifndef NETWORK_MONITOR_HPP
#define NETWORK_MONITOR_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace sysmon {
namespace core {

struct NetworkInterfaceMetrics {
    std::string interfaceName;
    uint64_t rxBytesTotal = 0;
    uint64_t txBytesTotal = 0;
    double rxRateBytesSec = 0.0;
    double txRateBytesSec = 0.0;
};

struct NetworkSnapshot {
    std::vector<NetworkInterfaceMetrics> interfaces;
    double totalRxRateBytesSec = 0.0;
    double totalTxRateBytesSec = 0.0;
};

class NetworkMonitor {
public:
    NetworkMonitor();

    void update(double elapsedSeconds, bool mock = false);
    const NetworkSnapshot& getSnapshot() const;

private:
    NetworkSnapshot snapshot_;
    std::unordered_map<std::string, std::pair<uint64_t, uint64_t>> prevBytes_;
    bool isFirstSample_ = true;
};

} // namespace core
} // namespace sysmon

#endif // NETWORK_MONITOR_HPP
