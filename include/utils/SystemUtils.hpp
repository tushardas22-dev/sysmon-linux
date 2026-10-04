#ifndef SYSTEM_UTILS_HPP
#define SYSTEM_UTILS_HPP

#include <string>
#include <cstdint>
#include <atomic>

namespace sysmon {
namespace utils {

class SystemUtils {
public:
    static void getTerminalSize(int& rows, int& cols);
    static void enableRawMode();
    static void disableRawMode();
    static bool isKeyPressed();
    static char readKey();

    static void registerSignalHandlers();
    static bool isInterrupted();
    static void requestExit();

    static bool sendSignalToProcess(int pid, int signalNumber);
    static std::string formatUptime(double seconds);

private:
    static std::atomic<bool> interrupted_;
    static bool rawModeEnabled_;
};

} // namespace utils
} // namespace sysmon

#endif // SYSTEM_UTILS_HPP
