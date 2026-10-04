#include "SystemMonitorApp.hpp"
#include "utils/Config.hpp"
#include "utils/Logger.hpp"
#include <iostream>
#include <exception>

int main(int argc, char* argv[]) {
    try {
        sysmon::utils::Config config = sysmon::utils::Config::parseCommandLine(argc, argv);
        sysmon::utils::Logger::getInstance().init(config.logFilePath, sysmon::utils::LogLevel::INFO, false);

        sysmon::SystemMonitorApp app(config);
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown Fatal Error occurred.\n";
        return 1;
    }
}
