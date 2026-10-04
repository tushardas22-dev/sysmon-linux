#ifndef ANSI_COLORS_HPP
#define ANSI_COLORS_HPP

#include <string>
#include <cstdint>

namespace sysmon {
namespace ui {

class Ansi {
public:
    // Text formatting
    static constexpr const char* RESET       = "\033[0m";
    static constexpr const char* BOLD        = "\033[1m";
    static constexpr const char* DIM         = "\033[2m";
    static constexpr const char* UNDERLINE   = "\033[4m";
    static constexpr const char* BLINK       = "\033[5m";
    static constexpr const char* REVERSE     = "\033[7m";
    static constexpr const char* HIDE_CURSOR = "\033[?25l";
    static constexpr const char* SHOW_CURSOR = "\033[?25h";
    static constexpr const char* CLEAR_SCREEN= "\033[2J\033[H";
    static constexpr const char* CLEAR_LINE  = "\033[2K\r";

    // Standard foreground colors
    static constexpr const char* BLACK       = "\033[30m";
    static constexpr const char* RED         = "\033[31m";
    static constexpr const char* GREEN       = "\033[32m";
    static constexpr const char* YELLOW      = "\033[33m";
    static constexpr const char* BLUE        = "\033[34m";
    static constexpr const char* MAGENTA     = "\033[35m";
    static constexpr const char* CYAN        = "\033[36m";
    static constexpr const char* WHITE       = "\033[37m";

    // Bright foreground colors
    static constexpr const char* BRIGHT_BLACK   = "\033[90m";
    static constexpr const char* BRIGHT_RED     = "\033[91m";
    static constexpr const char* BRIGHT_GREEN   = "\033[92m";
    static constexpr const char* BRIGHT_YELLOW  = "\033[93m";
    static constexpr const char* BRIGHT_BLUE    = "\033[94m";
    static constexpr const char* BRIGHT_MAGENTA = "\033[95m";
    static constexpr const char* BRIGHT_CYAN    = "\033[96m";
    static constexpr const char* BRIGHT_WHITE   = "\033[97m";

    // Background colors
    static constexpr const char* BG_BLACK    = "\033[40m";
    static constexpr const char* BG_RED      = "\033[41m";
    static constexpr const char* BG_GREEN    = "\033[42m";
    static constexpr const char* BG_BLUE     = "\033[44m";
    static constexpr const char* BG_CYAN     = "\033[46m";
    static constexpr const char* BG_WHITE    = "\033[47m";

    // Helpers
    static std::string colorize(const std::string& text, const char* color);
    static std::string progressBar(double percentage, int width = 20);
    static std::string formatBytes(uint64_t bytes);
    static std::string formatFrequency(uint64_t khz);
    static std::string getUsageColor(double percentage);
};

} // namespace ui
} // namespace sysmon

#endif // ANSI_COLORS_HPP
