#include "utils/Logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace sysmon {
namespace utils {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

void Logger::init(const std::string& filePath, LogLevel minLevel, bool consoleEcho) {
    std::lock_guard<std::mutex> lock(mutex_);
    minLevel_ = minLevel;
    consoleEcho_ = consoleEcho;

    if (logFile_.is_open()) {
        logFile_.close();
    }

    logFile_.open(filePath, std::ios::out | std::ios::app);
    if (!logFile_.is_open()) {
        std::cerr << "[Logger] Failed to open log file: " << filePath << std::endl;
    }
    initialized_ = true;
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < minLevel_) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    std::string timestamp = currentTimestamp();
    std::string lvlStr = levelToString(level);

    std::string formatted = "[" + timestamp + "] [" + lvlStr + "] " + message;

    if (logFile_.is_open()) {
        logFile_ << formatted << std::endl;
        logFile_.flush();
    }

    if (consoleEcho_) {
        std::cout << formatted << std::endl;
    }
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERR:     return "ERROR";
        default:                return "UNKNOWN";
    }
}

std::string Logger::currentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) % 1000;

    std::stringstream ss;
    struct tm tm_buf;
#if defined(_WIN32)
    localtime_s(&tm_buf, &in_time_t);
#else
    localtime_r(&in_time_t, &tm_buf);
#endif
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

} // namespace utils
} // namespace sysmon
