CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 -pthread -Iinclude -Idriver

SRC_FILES := src/ui/AnsiColors.cpp \
             src/utils/Config.cpp \
             src/utils/Logger.cpp \
             src/utils/ProcReader.cpp \
             src/utils/SystemUtils.cpp \
             src/core/CpuMonitor.cpp \
             src/core/MemoryMonitor.cpp \
             src/core/ArchitectureMonitor.cpp \
             src/core/DiskMonitor.cpp \
             src/core/NetworkMonitor.cpp \
             src/core/ProcessMonitor.cpp \
             src/driver_interface/KernelDriverClient.cpp \
             src/ui/TerminalUi.cpp \
             src/SystemMonitorApp.cpp

MAIN_SRC := src/main.cpp
TEST_SRC := tests/test_runner.cpp

BIN_DIR := bin
TARGET := $(BIN_DIR)/sysmon
TEST_TARGET := $(BIN_DIR)/sysmon_tests

.PHONY: all clean run test driver help

all: $(TARGET) $(TEST_TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(TARGET): $(SRC_FILES) $(MAIN_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(MAIN_SRC) $(SRC_FILES) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

$(TEST_TARGET): $(SRC_FILES) $(TEST_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_SRC) $(SRC_FILES) -o $(TEST_TARGET)
	@echo "Tests build complete: $(TEST_TARGET)"

test: $(TEST_TARGET)
	./$(TEST_TARGET)

run: $(TARGET)
	./$(TARGET)

mock: $(TARGET)
	./$(TARGET) --mock

driver:
	$(MAKE) -C driver

clean:
	rm -rf $(BIN_DIR) *.log
	$(MAKE) -C driver clean 2>/dev/null || true

help:
	@echo "Available make targets:"
	@echo "  make        : Build sysmon and test suite"
	@echo "  make run    : Run interactive SysMon"
	@echo "  make mock   : Run SysMon in mock mode"
	@echo "  make test   : Build and execute test suite"
	@echo "  make driver : Build the Linux kernel character driver"
	@echo "  make clean  : Clean binaries, logs, and kernel objects"
