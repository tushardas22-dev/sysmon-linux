#include "ui/AnsiColors.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace sysmon {
namespace ui {

std::string Ansi::colorize(const std::string& text, const char* color) {
    return std::string(color) + text + RESET;
}

std::string Ansi::progressBar(double percentage, int width) {
    if (percentage < 0.0) percentage = 0.0;
    if (percentage > 100.0) percentage = 100.0;

    int filled = static_cast<int>(std::round((percentage / 100.0) * width));
    if (filled > width) filled = width;

    std::string color = getUsageColor(percentage);

    std::string bar = "[";
    bar += color;
    for (int i = 0; i < filled; ++i) {
        bar += "#";
    }
    bar += RESET;
    for (int i = filled; i < width; ++i) {
        bar += ".";
    }
    bar += "]";

    std::ostringstream oss;
    oss << bar << " " << std::fixed << std::setprecision(1) << std::setw(5) << percentage << "%";
    return oss.str();
}

std::string Ansi::formatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int idx = 0;
    double dbytes = static_cast<double>(bytes);

    while (dbytes >= 1024.0 && idx < 5) {
        dbytes /= 1024.0;
        idx++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << dbytes << " " << units[idx];
    return oss.str();
}

std::string Ansi::formatFrequency(uint64_t khz) {
    double mhz = static_cast<double>(khz) / 1000.0;
    std::ostringstream oss;
    if (mhz >= 1000.0) {
        oss << std::fixed << std::setprecision(2) << (mhz / 1000.0) << " GHz";
    } else {
        oss << std::fixed << std::setprecision(0) << mhz << " MHz";
    }
    return oss.str();
}

std::string Ansi::getUsageColor(double percentage) {
    if (percentage < 50.0) {
        return GREEN;
    } else if (percentage < 80.0) {
        return YELLOW;
    } else {
        return RED;
    }
}

} // namespace ui
} // namespace sysmon
