#include "utils/SystemUtils.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <csignal>

#if !defined(_WIN32)
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <signal.h>
#else
#include <windows.h>
#include <conio.h>
#endif

namespace sysmon {
namespace utils {

std::atomic<bool> SystemUtils::interrupted_{false};
bool SystemUtils::rawModeEnabled_{false};

#if !defined(_WIN32)
static struct termios orig_termios;
#endif

static void sigHandler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        SystemUtils::requestExit();
    }
}

void SystemUtils::registerSignalHandlers() {
    std::signal(SIGINT, sigHandler);
    std::signal(SIGTERM, sigHandler);
}

bool SystemUtils::isInterrupted() {
    return interrupted_.load();
}

void SystemUtils::requestExit() {
    interrupted_.store(true);
}

void SystemUtils::getTerminalSize(int& rows, int& cols) {
#if !defined(_WIN32)
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        rows = ws.ws_row;
        cols = ws.ws_col;
        return;
    }
#else
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        return;
    }
#endif
    // Default fallback
    rows = 24;
    cols = 80;
}

void SystemUtils::enableRawMode() {
#if !defined(_WIN32)
    if (rawModeEnabled_) return;
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) return;

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0) {
        rawModeEnabled_ = true;
    }
#endif
}

void SystemUtils::disableRawMode() {
#if !defined(_WIN32)
    if (!rawModeEnabled_) return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    rawModeEnabled_ = false;
#endif
}

bool SystemUtils::isKeyPressed() {
#if !defined(_WIN32)
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval tv = {0, 0};
    int ret = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &tv);
    return (ret > 0 && FD_ISSET(STDIN_FILENO, &readfds));
#else
    return _kbhit() != 0;
#endif
}

char SystemUtils::readKey() {
#if !defined(_WIN32)
    char c = 0;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        return c;
    }
    return 0;
#else
    if (_kbhit()) {
        return static_cast<char>(_getch());
    }
    return 0;
#endif
}

bool SystemUtils::sendSignalToProcess(int pid, int signalNumber) {
#if !defined(_WIN32)
    return (kill(pid, signalNumber) == 0);
#else
    (void)pid;
    (void)signalNumber;
    return false;
#endif
}

std::string SystemUtils::formatUptime(double seconds) {
    uint64_t totalSec = static_cast<uint64_t>(seconds);
    uint64_t days = totalSec / 86400;
    uint64_t hours = (totalSec % 86400) / 3600;
    uint64_t minutes = (totalSec % 3600) / 60;
    uint64_t secs = totalSec % 60;

    std::ostringstream ss;
    if (days > 0) {
        ss << days << "d ";
    }
    ss << std::setfill('0') << std::setw(2) << hours << ":"
       << std::setfill('0') << std::setw(2) << minutes << ":"
       << std::setfill('0') << std::setw(2) << secs;
    return ss.str();
}

} // namespace utils
} // namespace sysmon
