#include "core/NetworkMonitor.hpp"
#include "utils/ProcReader.hpp"

namespace sysmon {
namespace core {

NetworkMonitor::NetworkMonitor() = default;

void NetworkMonitor::update(double elapsedSeconds, bool mock) {
    if (elapsedSeconds <= 0.0) elapsedSeconds = 1.0;

    auto currentNet = utils::ProcReader::readNetDev(mock);
    snapshot_.interfaces.clear();
    snapshot_.totalRxRateBytesSec = 0.0;
    snapshot_.totalTxRateBytesSec = 0.0;

    for (const auto& [iface, pair] : currentNet) {
        NetworkInterfaceMetrics m;
        m.interfaceName = iface;
        m.rxBytesTotal = pair.first;
        m.txBytesTotal = pair.second;

        if (!isFirstSample_ && prevBytes_.find(iface) != prevBytes_.end()) {
            uint64_t prevRx = prevBytes_[iface].first;
            uint64_t prevTx = prevBytes_[iface].second;

            uint64_t deltaRx = (m.rxBytesTotal >= prevRx) ? (m.rxBytesTotal - prevRx) : 0;
            uint64_t deltaTx = (m.txBytesTotal >= prevTx) ? (m.txBytesTotal - prevTx) : 0;

            m.rxRateBytesSec = static_cast<double>(deltaRx) / elapsedSeconds;
            m.txRateBytesSec = static_cast<double>(deltaTx) / elapsedSeconds;

            snapshot_.totalRxRateBytesSec += m.rxRateBytesSec;
            snapshot_.totalTxRateBytesSec += m.txRateBytesSec;
        }

        snapshot_.interfaces.push_back(m);
    }

    prevBytes_ = currentNet;
    isFirstSample_ = false;
}

const NetworkSnapshot& NetworkMonitor::getSnapshot() const {
    return snapshot_;
}

} // namespace core
} // namespace sysmon
