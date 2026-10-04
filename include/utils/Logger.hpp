#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fstream>
#include <mutex>
#include <memory>

namespace sysmon {
namespace utils {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERR
};

class Logger {
public:
    static Logger& getInstance();

    void init(const std::string& filePath, LogLevel minLevel = LogLevel::INFO, bool consoleEcho = false);
    void log(LogLevel level, const std::string& message);
    void debug(const std::string& message) { log(LogLevel::DEBUG, message); }
    void info(const std::string& message)  { log(LogLevel::INFO, message); }
    void warn(const std::string& message)  { log(LogLevel::WARNING, message); }
    void error(const std::string& message) { log(LogLevel::ERR, message); }

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string levelToString(LogLevel level) const;
    std::string currentTimestamp() const;

    std::ofstream logFile_;
    LogLevel minLevel_ = LogLevel::INFO;
    bool consoleEcho_ = false;
    std::mutex mutex_;
    bool initialized_ = false;
};

} // namespace utils
} // namespace sysmon

#endif // LOGGER_HPP
