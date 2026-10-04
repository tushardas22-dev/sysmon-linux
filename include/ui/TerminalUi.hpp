#ifndef TERMINAL_UI_HPP
#define TERMINAL_UI_HPP

#include "core/CpuMonitor.hpp"
#include "core/MemoryMonitor.hpp"
#include "core/ProcessMonitor.hpp"
#include "core/DiskMonitor.hpp"
#include "core/NetworkMonitor.hpp"
#include "core/ArchitectureMonitor.hpp"
#include "driver_interface/KernelDriverClient.hpp"
#include "utils/Config.hpp"
#include <string>

namespace sysmon {
namespace ui {

enum class ViewTab {
    DASHBOARD = 1,
    ARCHITECTURE = 2,
    PROCESSES = 3,
    KERNEL_DRIVER = 4,
    HELP = 5
};

class TerminalUi {
public:
    TerminalUi(const utils::Config& config);
    ~TerminalUi();

    void init();
    void render(
        const core::CpuSnapshot& cpu,
        const core::MemorySnapshot& mem,
        const core::ProcessSnapshot& procs,
        const core::DiskSnapshot& disk,
        const core::NetworkSnapshot& net,
        const core::ArchitectureSnapshot& arch,
        const driver::DriverStatus& driverStatus,
        const struct sysmon_kernel_metrics& kMetrics
    );

    void setTab(ViewTab tab);
    ViewTab getTab() const;
    void nextTab();
    void prevTab();

    void setStatusMessage(const std::string& msg);

private:
    void renderHeader(std::string& buffer, const core::ArchitectureSnapshot& arch, const driver::DriverStatus& drv);
    void renderTabBar(std::string& buffer);
    void renderDashboardView(std::string& buffer, const core::CpuSnapshot& cpu, const core::MemorySnapshot& mem, const core::DiskSnapshot& disk, const core::NetworkSnapshot& net, const core::ProcessSnapshot& procs);
    void renderArchitectureView(std::string& buffer, const core::ArchitectureSnapshot& arch, const core::CpuSnapshot& cpu);
    void renderProcessesView(std::string& buffer, const core::ProcessSnapshot& procs);
    void renderKernelDriverView(std::string& buffer, const driver::DriverStatus& drv, const struct sysmon_kernel_metrics& kMetrics, const core::MemorySnapshot& mem);
    void renderHelpView(std::string& buffer);
    void renderFooter(std::string& buffer);

    utils::Config config_;
    ViewTab currentTab_ = ViewTab::DASHBOARD;
    int termRows_ = 24;
    int termCols_ = 80;
    std::string statusMessage_;
    int statusMessageTimer_ = 0;
};

} // namespace ui
} // namespace sysmon

#endif // TERMINAL_UI_HPP
