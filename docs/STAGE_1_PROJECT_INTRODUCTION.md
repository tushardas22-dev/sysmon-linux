# Stage 1 – Project Introduction & Problem Definition

**Project Title:** SysMon – System Resource Monitoring Tool & Linux Kernel Driver  
**Target Environment:** Ubuntu Linux / Windows Subsystem for Linux 2 (WSL2)  
**Curriculum Track:** 20-Day Training: Linux, Device Drivers, System Programming & C++  
**Author:** Individual Capstone Submission  

---

## 1. Project Idea & Objectives

In modern operating systems, performance monitoring and resource profiling are essential for diagnosing system bottlenecks, detecting memory leaks, analyzing multithreaded scheduling behavior, and ensuring system stability.

The objective of this project is to architect and develop **SysMon**, a comprehensive, high-performance system resource monitoring suite implemented in **Modern C++ (C++17)** on **Ubuntu (WSL2)**, augmented with a custom **Linux Character Device Driver**.

### Primary Objectives
1. **Develop a Low-Overhead Telemetry Engine:** Build high-throughput system collectors utilizing direct Linux kernel interfaces and virtual filesystems (`/proc` and `/sys`).
2. **Implement a Custom Linux Character Device Driver (`/dev/sysmon_driver`):** Bridge the user space and kernel space boundary by exposing kernel data structures (`struct sysinfo`, `task_struct`, load averages) through an IOCTL interface.
3. **Analyze Computer Architecture & Hardware Hierarchy:** Expose deep CPU architectural properties including Symmetrical Multi-Processing (SMP) per-core loads, cache memory hierarchy (L1d, L1i, L2, L3 caches, cache line size, associativity), CPU clock frequency scaling, context switches, interrupt rates, and page faults.
4. **Deliver a Terminal User Interface (TUI):** Create a responsive, non-blocking ANSI dashboard providing real-time multi-core gauges, memory and swap status, I/O rates, and an interactive process management interface capable of process signaling (`SIGTERM`, `SIGKILL`).
5. **Demonstrate 20-Day Training Concepts:** Combine Linux system programming, POSIX APIs, Modern C++ object-oriented principles, multithreading, concurrency synchronization, computer architecture metrics, and device driver engineering into a cohesive, production-ready system.

---

## 2. Problem Statement

Standard monitoring tools such as `top`, `vmstat`, and basic system utilities often exhibit one or more of the following limitations:
- **Excessive User-Kernel Overhead:** User-space tools that rely exclusively on scanning `/proc` must open, parse, convert strings, and close dozens or hundreds of pseudo-files on every sampling tick, introducing measurable CPU overhead and context switching.
- **Surface-Level Metrics:** Most basic monitoring scripts present aggregate CPU percentages and free RAM, completely obscuring hardware architecture details such as L1/L2/L3 cache sizes, cache line sharing, frequency governors, page fault categories (minor vs major faults), and hardware interrupt frequencies.
- **Lack of Kernel-Level Extensibility:** Standard tools run entirely in user space and do not demonstrate or utilize custom kernel drivers to extract internal kernel structures (`task_struct`, monotonic kernel timers, raw page counts) directly.
- **Limited Portability Across Hybrid Environments (WSL2 vs Native Linux):** Virtualized development platforms like WSL2 have unique kernel characteristics, hypervisor abstractions, and masked sysfs paths that cause standard monitoring scripts to fail or misreport metrics.

**SysMon solves this problem** by providing a hybrid architecture: it features a dedicated Linux Character Driver for privileged zero-copy binary data exchange, while maintaining a fallback parser for standard `/proc` environments, all exposed through an interactive, multi-view terminal interface.

---

## 3. Project Scope

### In-Scope
1. **Core Monitoring Modules:**
   - **CPU Module:** Overall CPU utilization %, User%, System%, Idle%, IOWait%, Steal%, and per-core SMP breakdown (Core 0 to Core N).
   - **Memory Module:** Total, Used, Free, Available, Buffers, Cached RAM, Dirty pages, Swap Total/Used/Free, and page fault rates.
   - **Architecture & Hardware Module:** Processor model, vendor, microarchitecture family, stepping, instruction flags (SSE, AVX, AES), L1d/L1i/L2/L3 cache hierarchy (size, associativity, line size), and core frequencies.
   - **Process Module:** Real-time scanning of active processes, PID, PPID, User, State (Running, Sleeping, Zombie), CPU%, Memory (RSS/VMS), Threads, Priority, Niceness, with sorting and signal dispatching.
   - **Disk & Network I/O Module:** Sector read/write rates (KB/s, IOPS) and network interface transfer rates (RX/TX KB/s).
2. **Linux Device Driver:**
   - Linux character device `/dev/sysmon_driver` implemented as a loadable kernel module (`.ko`).
   - Custom IOCTL commands (`SYSMON_IOCTL_GET_METRICS`, `SYSMON_IOCTL_GET_PROCS`, `SYSMON_IOCTL_RESET_STATS`).
   - RCU-protected traversal of kernel `task_struct` list.
3. **C++ Application Architecture:**
   - Multithreaded architecture: background telemetry worker thread decoupled from the terminal UI thread.
   - Concurrency synchronization using `std::mutex`, `std::condition_variable`, and `std::atomic<bool>`.
   - Resource acquisition via RAII for file descriptors, termios raw modes, and kernel device files.
4. **Automated Build & Test Infrastructure:**
   - Build scripts using GNU Make and CMake (C++17 standard, `-Wall -Wextra -O2 -pthread`).
   - Custom test runner verifying CPU deltas, sorting algorithms, memory units, and driver fallback modes.

### Out-of-Scope
- Graphic desktop GUI (Qt or GTK) — terminal ANSI interface chosen specifically for lightweight execution on headless servers and WSL2 environments.
- Network daemon / remote HTTP monitoring server (REST/WebSocket).

---

## 4. Expected Outcome and Applications

### Expected Outcomes
- A compiled, high-performance binary (`sysmon`) with minimal CPU footprint (< 1.5% CPU overhead during live monitoring).
- A loadable Linux kernel module (`sysmon_driver.ko`) demonstrating character device registration, file operations, and IOCTL communication.
- A comprehensive unit test suite (`sysmon_tests`) with 100% pass rate.
- Complete documentation covering all 6 stages of software development lifecycle, architecture diagrams, and viva/interview defense material.

### Real-World Applications
1. **Embedded Linux & Edge Systems:** Low-overhead resource profiling where GUI tools are unavailable.
2. **High-Performance Computing (HPC) & Server Infrastructure:** Rapid diagnosis of thread contention, context switch spikes, memory exhaustion, and I/O bottlenecks.
3. **WSL2 Developer Workstations:** Seamless inspection of Windows-Linux hybrid memory allocation and Hyper-V virtual CPU sharing.
4. **Educational & Interview Demonstration:** Serving as a benchmark portfolio project demonstrating mastery over Linux system internals, kernel modules, modern C++, and computer systems architecture.
