# Stage 3 – System Design & Architecture

**Project:** SysMon – System Resource Monitoring Tool & Linux Kernel Driver  
**Version:** 1.0.0  
**Phase:** Stage 3 – Architectural Design & Technical Specification  

---

## 1. Overall System Architecture

SysMon follows a **layered, decoupled producer-consumer architecture**. The system is divided into three primary layers:
1. **Telemetry & Driver Layer (Kernel Space & Low-Level Interfaces):** Interfaces with Linux virtual pseudo-filesystems (`/proc`, `/sys`) and houses the custom Linux character device driver (`/dev/sysmon_driver`).
2. **Core Monitoring & Processing Layer (User Space Engine):** Encapsulates domain logic for calculating deltas, memory conversions, sorting algorithms, and architecture detection. Operates on a dedicated background collector thread.
3. **Presentation & Interaction Layer (TUI & Controller):** Renders flicker-free ANSI dashboard screens, processes non-blocking keyboard events, and dispatches user actions (sorting, process signaling, tab switching).

### High-Level Architecture Diagram

```mermaid
flowchart TB
    subgraph Layer1 ["Layer 1: Telemetry & Driver Layer"]
        Hardware["Physical Hardware / Hyper-V Virtual CPU & RAM"]
        KernelCore["Linux Kernel Subsystems (Scheduler, Memory Manager, VFS)"]
        KDriver["sysmon_driver.ko (/dev/sysmon_driver)"]
        ProcFS["Linux /proc and /sys Virtual Filesystems"]
        
        Hardware --> KernelCore
        KernelCore --> KDriver
        KernelCore --> ProcFS
    end

    subgraph Layer2 ["Layer 2: Core Monitoring Engine (Background Thread)"]
        KDClient["driver::KernelDriverClient (IOCTL Handler)"]
        PReader["utils::ProcReader (Streaming String Parser)"]
        
        KDriver -.->|ioctl| KDClient
        ProcFS -->|read| PReader

        CpuMon["core::CpuMonitor"]
        MemMon["core::MemoryMonitor"]
        ArchMon["core::ArchitectureMonitor"]
        ProcMon["core::ProcessMonitor"]
        DiskMon["core::DiskMonitor"]
        NetMon["core::NetworkMonitor"]

        PReader --> CpuMon & MemMon & ArchMon & ProcMon & DiskMon & NetMon
        KDClient --> CpuMon & MemMon

        DataMutex[("std::mutex Thread-Safe Data Snapshots")]
        CpuMon & MemMon & ArchMon & ProcMon & DiskMon & NetMon --> DataMutex
    end

    subgraph Layer3 ["Layer 3: Orchestration & Presentation Layer (Main Thread)"]
        AppController["SystemMonitorApp (Master Loop)"]
        DataMutex --> AppController

        TerminalUI["ui::TerminalUi (ANSI View Renderer)"]
        KeyInput["utils::SystemUtils (termios Raw Input)"]

        KeyInput --> AppController
        AppController --> TerminalUI
        TerminalUI --> Display["Terminal Console Display"]
    end
```

---

## 2. Major Components and Responsibilities

| Component | Namespace / Class | Key Responsibilities |
| :--- | :--- | :--- |
| **Kernel Driver** | `sysmon_driver.c` | Registers character device `/dev/sysmon_driver`, dynamically obtains major number, traverses `task_struct` using `rcu_read_lock()`, satisfies IOCTL calls with `copy_to_user()`. |
| **Driver Client** | `sysmon::driver::KernelDriverClient` | Manages file descriptor to `/dev/sysmon_driver`, issues IOCTL commands (`SYSMON_IOCTL_GET_METRICS`), maintains driver connection health. |
| **Proc Parser** | `sysmon::utils::ProcReader` | Safely opens and parses `/proc/stat`, `/proc/meminfo`, `/proc/vmstat`, `/proc/diskstats`, `/proc/net/dev`, and PID pseudo-files. Implements mock mode fallback. |
| **CPU Monitor** | `sysmon::core::CpuMonitor` | Computes aggregate and SMP per-core CPU utilization via tick deltas (`delta_work / delta_total * 100`). Tracks context switches and interrupts. |
| **Memory Monitor** | `sysmon::core::MemoryMonitor` | Calculates total, used, free, available, cached RAM, swap space, and page fault frequencies. |
| **Arch Monitor** | `sysmon::core::ArchitectureMonitor` | Interrogates `/sys/devices/system/cpu/` to discover L1d/L1i/L2/L3 cache capacities, associativity, line sizes, and core clock frequencies. |
| **Process Monitor** | `sysmon::core::ProcessMonitor` | Tracks active system processes, PID, PPID, state, thread counts, CPU%, Memory RSS, multi-field sorting, and POSIX signal termination (`kill()`). |
| **Terminal UI** | `sysmon::ui::TerminalUi` | Builds ANSI escape-coded display frames for 5 distinct views (Dashboard, Architecture, Processes, Driver, Help). |
| **App Orchestrator**| `sysmon::SystemMonitorApp` | Coordinates background collection thread (`std::thread`), UI update loop, signal handlers (`SIGINT`, `SIGTERM`), and graceful termination. |

---

## 3. Data Structures Definition

### 3.1 Kernel Shared Structures (`driver/sysmon_driver.h`)
```c
struct sysmon_kernel_metrics {
    uint64_t uptime_seconds;
    uint64_t total_ram_bytes;
    uint64_t free_ram_bytes;
    uint64_t shared_ram_bytes;
    uint64_t buffer_ram_bytes;
    uint64_t total_swap_bytes;
    uint64_t free_swap_bytes;
    uint32_t process_count;
    uint32_t active_cpus;
    uint64_t context_switches;
    uint64_t total_forks;
    uint64_t interrupts_total;
    uint32_t load_avg[3];        /* 1, 5, 15 min load */
    uint64_t sample_timestamp_ns;
};
```

### 3.2 Process Telemetry Structure (`include/core/ProcessMonitor.hpp`)
```cpp
struct ProcessInfo {
    int pid = 0;
    int ppid = 0;
    std::string user;
    std::string name;
    std::string cmdline;
    char state = 'S';
    double cpuPercent = 0.0;
    double memPercent = 0.0;
    uint64_t rssBytes = 0;
    uint64_t vmsBytes = 0;
    long numThreads = 1;
    long priority = 0;
    long nice = 0;
    uint64_t totalTicks = 0;
};
```

### 3.3 Cache Hierarchy Structure (`include/core/ArchitectureMonitor.hpp`)
```cpp
struct CacheLevelInfo {
    int level = 1;
    std::string type;       // "Data", "Instruction", "Unified"
    std::string sizeStr;    // e.g. "32K", "512K", "16M"
    uint64_t sizeBytes = 0;
    int associativity = 0;
    int lineSize = 0;
};
```

---

## 4. UML Diagrams

### 4.1 UML Class Diagram
```mermaid
classDiagram
    class SystemMonitorApp {
        -Config config_
        -CpuMonitor cpuMonitor_
        -MemoryMonitor memMonitor_
        -ProcessMonitor procMonitor_
        -DiskMonitor diskMonitor_
        -NetworkMonitor netMonitor_
        -ArchitectureMonitor archMonitor_
        -KernelDriverClient driverClient_
        -TerminalUi ui_
        -atomic<bool> running_
        -mutex dataMutex_
        -thread collectorThread_
        +run() int
        -dataCollectorLoop() void
        -handleUserInput() void
    }

    class CpuMonitor {
        -vector~CpuStatRaw~ prevRawStats_
        -CpuSnapshot snapshot_
        +update(bool mock) void
        +getSnapshot() CpuSnapshot
        -calculateDeltas() void
    }

    class MemoryMonitor {
        -MemorySnapshot snapshot_
        +update(bool mock) void
        +getSnapshot() MemorySnapshot
    }

    class ProcessMonitor {
        -ProcessSnapshot snapshot_
        -unordered_map prevTicks_
        +update(uint64_t sysTicks, uint64_t ramBytes, bool mock) void
        +sortProcesses(ProcessSortField field, bool asc) void
        +killProcess(int pid, int sig) bool
        +getSnapshot() ProcessSnapshot
    }

    class ArchitectureMonitor {
        -ArchitectureSnapshot snapshot_
        +update(bool mock) void
        +getSnapshot() ArchitectureSnapshot
    }

    class KernelDriverClient {
        -string devPath_
        -int fd_
        -DriverStatus status_
        +initialize() bool
        +queryMetrics(sysmon_kernel_metrics& out, bool mock) bool
        +close() void
    }

    class TerminalUi {
        -Config config_
        -ViewTab currentTab_
        +render(...) void
        +setTab(ViewTab tab) void
    }

    class ProcReader {
        <<utility>>
        +readCpuStats(bool mock) vector~CpuStatRaw~
        +readMemInfo(bool mock) unordered_map
        +readProcessStat(...) bool
    }

    SystemMonitorApp *-- CpuMonitor
    SystemMonitorApp *-- MemoryMonitor
    SystemMonitorApp *-- ProcessMonitor
    SystemMonitorApp *-- ArchitectureMonitor
    SystemMonitorApp *-- KernelDriverClient
    SystemMonitorApp *-- TerminalUi
    CpuMonitor ..> ProcReader
    MemoryMonitor ..> ProcReader
    ProcessMonitor ..> ProcReader
    ArchitectureMonitor ..> ProcReader
```

---

### 4.2 UML Sequence Diagram: Sampling & Telemetry Cycle
```mermaid
sequenceDiagram
    autonumber
    actor User
    participant MainThread as Main Thread (SystemMonitorApp)
    participant Collector as Worker Thread (dataCollectorLoop)
    participant KernelClient as KernelDriverClient
    participant Driver as /dev/sysmon_driver (Kernel)
    participant ProcReader as ProcReader (/proc)
    participant UI as TerminalUi

    MainThread->>Collector: Spawn thread
    loop Periodic Telemetry Sampling (every 1000ms)
        Collector->>KernelClient: queryMetrics(outMetrics)
        alt Driver Online
            KernelClient->>Driver: ioctl(SYSMON_IOCTL_GET_METRICS)
            Driver-->>KernelClient: Return kernel binary struct
            KernelClient-->>Collector: Populated struct sysmon_kernel_metrics
        else Driver Offline / Unprivileged
            KernelClient-->>Collector: Fallback flag enabled
        end

        Collector->>ProcReader: readCpuStats(), readMemInfo(), getProcessPids()
        ProcReader-->>Collector: Raw system tick counters & process states
        Collector->>Collector: Calculate CPU tick deltas & process sort order
        Collector->>MainThread: Lock dataMutex_ & update shared snapshots
    end

    loop UI Render & Input Event Loop (every 80ms)
        MainThread->>User: Check non-blocking keypress (termios)
        alt Key Pressed ('1'-'5', 'c', 'm', 'q')
            User-->>MainThread: Keystroke received
            MainThread->>MainThread: Update active tab / sorting criteria
        end
        MainThread->>UI: render(snapshots...)
        UI-->>User: Output ANSI formatted frame to terminal
    end
```

---

### 4.3 UML State Machine Diagram: Application Lifecycle
```mermaid
stateDiagram-v2
    [*] --> Initializing : Application Launch

    Initializing --> CheckingDriver : Register Signals & Read Config
    CheckingDriver --> Running : Connect /dev/sysmon_driver (Success)
    CheckingDriver --> Running : Driver Unavailable (Procfs Fallback)

    state Running {
        [*] --> DashboardView
        DashboardView --> ArchView : Press '2'
        ArchView --> ProcessView : Press '3'
        ProcessView --> DriverView : Press '4'
        DriverView --> HelpView : Press '5' / 'h'
        HelpView --> DashboardView : Press '1'
        
        state ProcessView {
            [*] --> IdleView
            IdleView --> Sorting : Press 'c' / 'm' / 'p'
            Sorting --> IdleView : Re-sort table
            IdleView --> TerminatingProcess : Press 'k' (Send Signal)
            TerminatingProcess --> IdleView : kill(pid, SIGTERM)
        }
    }

    Running --> ShuttingDown : Press 'q' / Receive SIGINT
    ShuttingDown --> [*] : Restore Termios & Cursor Visibility
```

---

## 5. Development Environment & Git Configuration

### 5.1 Toolchain & Dependencies
- **Operating System:** Ubuntu 22.04 LTS (under WSL2) / Native Linux Kernel 5.15+
- **Compiler:** GNU G++ 11.4+ (C++17 Standard Compliant)
- **Build Utilities:** GNU Make 4.3+, CMake 3.22+
- **Kernel Headers:** `linux-headers-$(uname -r)`
- **Debugging & Profiling:** GDB 12.1, Valgrind 3.18, `strace`

### 5.2 Git Repository & Branching Strategy
The project follows **Git Flow** best practices:
- `main`: Production-ready release branch containing tagged milestones (`v1.0.0`).
- `develop`: Primary integration branch where completed stage features are merged.
- `feature/*`: Specific feature branches:
  - `feature/stage1-intro`
  - `feature/stage2-requirements`
  - `feature/stage3-architecture`
  - `feature/stage4-prototype`
  - `feature/stage5-testing`
  - `feature/stage6-final`
  - `feature/kernel-driver`
  - `feature/ansi-ui`

Each commit message follows conventional commits: `feat:`, `fix:`, `docs:`, `test:`, `refactor:`.
