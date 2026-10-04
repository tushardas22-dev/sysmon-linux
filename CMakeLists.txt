cmake_minimum_required(VERSION 3.14)
project(sysmon LANGUAGES C CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings and optimization flags
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -O2 -pthread)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/driver
)

# Core library source files
set(SYSMON_SOURCES
    src/ui/AnsiColors.cpp
    src/utils/Config.cpp
    src/utils/Logger.cpp
    src/utils/ProcReader.cpp
    src/utils/SystemUtils.cpp
    src/core/CpuMonitor.cpp
    src/core/MemoryMonitor.cpp
    src/core/ArchitectureMonitor.cpp
    src/core/DiskMonitor.cpp
    src/core/NetworkMonitor.cpp
    src/core/ProcessMonitor.cpp
    src/driver_interface/KernelDriverClient.cpp
    src/ui/TerminalUi.cpp
    src/SystemMonitorApp.cpp
)

# Main Executable
add_executable(sysmon src/main.cpp ${SYSMON_SOURCES})

find_package(Threads REQUIRED)
target_link_libraries(sysmon PRIVATE Threads::Threads)

# Unit Tests Executable
add_executable(sysmon_tests tests/test_runner.cpp ${SYSMON_SOURCES})
target_link_libraries(sysmon_tests PRIVATE Threads::Threads)

# Enable CTest
enable_testing()
add_test(NAME AllTests COMMAND sysmon_tests)
