# SysMon: Technical Interview & Viva Defense Preparation Guide

**Project:** SysMon – System Resource Monitoring Tool & Linux Kernel Driver  
**Course Track:** Linux, Device Drivers, System Programming & Modern C++ (20-Day Training)  
**Target Role:** Linux Systems Engineer / Embedded C++ Developer / Kernel Engineer  

This comprehensive guide prepares you to excel in the technical interview and project viva round. It contains **30+ detailed questions and answers** directly addressing the code, architecture, design trade-offs, and concepts demonstrated in SysMon.

---

## Section 1: Linux System Programming & Virtual Filesystems

### Q1: What is the `/proc` filesystem in Linux, and how does SysMon parse it?
**Answer:**  
`/proc` is a virtual (pseudo) filesystem created on-the-fly by the Linux kernel in RAM (VFS). It does not reside on disk storage. Instead, when user space reads a file like `/proc/stat` or `/proc/meminfo`, the kernel executes internal handler functions that gather live kernel metrics and format them as text.  
In SysMon, `ProcReader` opens these virtual files via standard streams/file descriptors, extracts key metrics (e.g. CPU tick deltas, memory bytes, process status), and converts them into structured C++ data objects.

### Q2: How does SysMon calculate CPU utilization percentage from `/proc/stat`?
**Answer:**  
CPU utilization cannot be determined from a single reading because `/proc/stat` reports cumulative CPU jiffies (time units) since system boot. SysMon uses a two-pass delta calculation across two sampling intervals ($t_1$ and $t_2$):
1. **Idle Time:** $\Delta \text{Idle} = (\text{idle}_2 + \text{iowait}_2) - (\text{idle}_1 + \text{iowait}_1)$
2. **Active Time:** $\Delta \text{Active} = (\text{user}_2 + \text{nice}_2 + \text{system}_2 + \text{irq}_2 + \text{softirq}_2 + \text{steal}_2) - (\text{user}_1 + \dots)$
3. **Total Time:** $\Delta \text{Total} = \Delta \text{Idle} + \Delta \text{Active}$
4. **Utilization %:** $\text{CPU \%} = \frac{\Delta \text{Active}}{\Delta \text{Total}} \times 100$  
This is computed both for aggregate CPU (`cpu`) and for every individual core (`cpu0`, `cpu1`, etc.) for SMP architectures.

### Q3: What edge case occurs when parsing `/proc/[pid]/stat`, and how did you resolve it?
**Answer:**  
In `/proc/[pid]/stat`, the second field is the executable name enclosed in parentheses `(comm)`. However, a process or thread name may contain arbitrary characters, including spaces, tabs, or additional parentheses (e.g., `(Web Content)` or `(sh (child))`). Naive tokenization with `cin >>` or `sscanf` splits on intermediate spaces and shifts all subsequent integer fields.  
SysMon resolves this by searching for the **first `(`** and the **last `)`** using `std::string::rfind(')')`. The string between is safely extracted as the process name, and the substring after the final `)` is parsed for state, PPID, utime, stime, priority, and RSS memory.

### Q4: How does SysMon resolve a numeric UID from `/proc/[pid]/status` to an actual Linux username?
**Answer:**  
In `ProcReader::readProcessUser()`, SysMon reads `/proc/[pid]/status` to locate the `Uid:` line. It then calls the POSIX function `getpwuid(uid)`. `getpwuid` queries the system user database (e.g. `/etc/passwd` or NSS/LDAP) and returns a pointer to `struct passwd`, from which `pw->pw_name` is retrieved.

### Q5: What is the difference between Resident Set Size (RSS) and Virtual Memory Size (VMS)?
**Answer:**  
- **Virtual Memory Size (VMS / vsize):** The total address space allocated to the process, including mapped shared libraries, allocated heap that has not yet been written to, and memory-mapped files.
- **Resident Set Size (RSS):** The portion of memory actually held in physical RAM for that process. In `/proc/[pid]/stat`, field 24 reports RSS in page units. SysMon multiplies this by the system page size (`sysconf(_SC_PAGE_SIZE)`, typically 4096 bytes) to determine actual RAM consumption.

### Q6: How does SysMon implement process termination?
**Answer:**  
SysMon uses the POSIX `kill(pid, signalNumber)` system call via `SystemUtils::sendSignalToProcess()`. By default, it issues `SIGTERM` (signal 15), which allows the target process to perform graceful cleanup and exit handlers. If the process does not terminate, `SIGKILL` (signal 9) can be dispatched, which the kernel handles directly and cannot be caught or ignored.

---

## Section 2: Linux Device Drivers & Kernel Internals

### Q7: Why did you develop a custom Linux Character Device Driver (`sysmon_driver`) instead of solely reading `/proc`?
**Answer:**  
While `/proc` is convenient, it incurs significant overhead:
1. Every access involves a file system lookup, allocating VFS inodes, formatting binary kernel metrics into human-readable ASCII text, and copying text strings across user-kernel boundaries.
2. User space must then parse and convert ASCII strings back into binary integers.
3. Reading 100+ processes requires opening and closing hundreds of file descriptors every second.  
By developing `sysmon_driver.c`, we demonstrate kernel-level programming. The driver creates `/dev/sysmon_driver`. Through a single `ioctl()` call, the driver copies a pre-populated binary C structure directly from kernel memory to user memory with zero string formatting and zero parsing overhead.

### Q8: How is the character device registered and exposed in `/dev`?
**Answer:**  
The driver performs dynamic registration during `module_init`:
1. `alloc_chrdev_region(&dev_num, 0, 1, SYSMON_DEVICE_NAME)` dynamically allocates an unused major number.
2. `cdev_init(&sysmon_cdev, &sysmon_fops)` binds the file operations (`open`, `release`, `unlocked_ioctl`).
3. `cdev_add(&sysmon_cdev, dev_num, 1)` registers the character device with the kernel VFS.
4. `class_create(SYSMON_CLASS_NAME)` creates a sysfs device class under `/sys/class/sysmon`.
5. `device_create(sysmon_class, NULL, dev_num, NULL, SYSMON_DEVICE_NAME)` triggers `udev` to automatically create the device node at `/dev/sysmon_driver` with the assigned major/minor numbers.

### Q9: How does the driver iterate over active processes in kernel space without causing race conditions or deadlocks?
**Answer:**  
Inside `populate_process_table()`, the driver utilizes the macro `for_each_process(task)` enclosed within **RCU (Read-Copy Update) read locks**:
```c
rcu_read_lock();
for_each_process(task) {
    // Read task->pid, task->comm, task->utime, task->mm
}
rcu_read_unlock();
```
RCU allows read operations to proceed concurrently with task creation and termination without taking heavy read/write locks, preventing scheduling stalls or priority inversions.

### Q10: How does data transfer safely between kernel space and user space in your driver?
**Answer:**  
The driver uses `copy_to_user(to_user_ptr, from_kernel_ptr, size)`. Direct memory assignment (`*user_ptr = val`) is strictly prohibited in kernel space because:
1. User-space virtual memory addresses may be paged out or invalid, which would trigger a kernel page fault panic (oops).
2. It violates memory protection domains (SMAP - Supervisor Mode Access Prevention).  
`copy_to_user()` verifies that the destination memory range belongs to the calling process and handles page faults gracefully, returning `-EFAULT` if the pointer is invalid.

### Q11: What is `ioctl()`, and why is it preferred over `read()` for telemetry?
**Answer:**  
`read()` operates on byte streams without structured semantics. In contrast, `ioctl` (Input/Output Control) allows user space to send structured requests with specific command codes. Using `_IOR(SYSMON_MAGIC, 1, struct sysmon_kernel_metrics)`, the command encodes the transfer direction (read), magic identifier, sequence number, and exact payload size, providing type-safe binary structure exchange.

### Q12: How does the driver handle multi-threaded concurrent user-space requests?
**Answer:**  
Inside `sysmon_ioctl()`, critical sections and metrics counters are guarded using a kernel mutex:
```c
mutex_lock(&sysmon_lock);
// Execute ioctl command and populate metrics
mutex_unlock(&sysmon_lock);
```
This ensures multiple user-space threads accessing `/dev/sysmon_driver` concurrently do not corrupt internal buffers or race on invocation counters.

---

## Section 3: Modern C++ (C++17) & Concurrency Architecture

### Q13: What concurrency model does SysMon use, and why?
**Answer:**  
SysMon uses a **producer-consumer multithreaded architecture**:
- **Producer (Collector Worker Thread):** Executes `SystemMonitorApp::dataCollectorLoop()` on a background `std::thread`. It sleeps for the configured interval (e.g. 1000ms), queries kernel/proc telemetry, computes deltas, and updates shared snapshot objects.
- **Consumer (Main UI Thread):** Executes the user interaction and rendering loop. It runs at a higher refresh rate (~12-15 FPS / 80ms) to ensure keyboard input is immediately responsive and non-blocking.
- **Synchronization:** The snapshot objects are protected by a `std::mutex` (`dataMutex_`). During UI rendering, the main thread acquires the lock briefly to copy or inspect the latest snapshot, preventing data races.

### Q14: How is RAII (Resource Acquisition Is Initialization) applied in SysMon?
**Answer:**  
RAII is used throughout SysMon to prevent resource leaks:
1. **File Descriptors:** `KernelDriverClient` opens the driver handle in `initialize()` and automatically calls `close()` in its destructor `~KernelDriverClient()`.
2. **Terminal State:** `TerminalUi` enables raw mode and hides the cursor on initialization; its destructor `~TerminalUi()` guarantees restoring the original termios settings and unhiding the cursor even if an exception occurs.
3. **Mutex Locks:** All mutex locking uses `std::lock_guard<std::mutex>`, ensuring automatic unlock upon scope exit.
4. **Threads:** In `~SystemMonitorApp()`, the destructor signals `running_.store(false)` and calls `collectorThread_.join()`, preventing dangling threads.

### Q15: Why are `std::atomic<bool>` variables used for flags like `running_` and `interrupted_`?
**Answer:**  
Using a plain `bool` across threads without synchronization causes undefined behavior due to compiler optimizations (e.g. caching the variable in a CPU register) and memory reordering. `std::atomic<bool>` guarantees sequential consistency and memory barrier semantics across CPU cores without the heavy overhead of a full operating system mutex.

### Q16: How are STL algorithms and lambdas used in the process manager?
**Answer:**  
In `ProcessMonitor::sortProcesses()`, `std::sort` is invoked with a custom C++ lambda comparator:
```cpp
auto comp = [field, ascending](const ProcessInfo& a, const ProcessInfo& b) {
    if (field == ProcessSortField::CPU) {
        return ascending ? (a.cpuPercent < b.cpuPercent) : (a.cpuPercent > b.cpuPercent);
    } // ...
};
std::sort(snapshot_.processes.begin(), snapshot_.processes.end(), comp);
```
This allows dynamic switching between sorting by CPU, memory, PID, or name in $O(N \log N)$ time.

### Q17: What was the result of your Valgrind memory leak profiling?
**Answer:**  
Profiling with `valgrind --tool=memcheck --leak-check=full` showed:
`0 bytes in 0 blocks in use at exit; 0 errors from 0 contexts`. All dynamic memory allocations were cleanly paired with deallocations through smart resource management.

---

## Section 4: Computer Architecture & Hardware Interaction

### Q18: How does SysMon discover and report the CPU cache hierarchy?
**Answer:**  
SysMon inspects the sysfs directory tree under `/sys/devices/system/cpu/cpu0/cache/index[0..3]/`:
- `index0`: Level 1 Data Cache (L1d)
- `index1`: Level 1 Instruction Cache (L1i)
- `index2`: Level 2 Unified Cache (L2)
- `index3`: Level 3 Shared Last-Level Cache (L3)  
SysMon parses the pseudofiles `level`, `type`, `size` (e.g. `32K`, `512K`, `16M`), `ways_of_associativity` (e.g. 8-way, 12-way), and `coherency_line_size` (typically 64 bytes).

### Q19: What is the significance of cache line size and associativity in computer architecture?
**Answer:**  
- **Cache Line Size (typically 64 bytes):** The minimum unit of data transferred between main memory (DRAM) and CPU caches. Knowledge of cache line size is essential for systems programmers to avoid **false sharing** in multithreaded code, where independent variables modified by different cores reside on the same cache line.
- **Associativity ($N$-way Set Associative):** Governs how memory addresses map into cache sets. Higher associativity reduces conflict misses at the expense of slightly more complex lookup logic.

### Q20: What are Context Switches and Hardware Interrupts, and why does SysMon track their rates?
**Answer:**  
- **Context Switch:** The process where the CPU scheduler saves the architectural register state of the currently executing task and restores the register state of another task. Excessive context switching indicates thread thrashing, lock contention, or excessive I/O interrupts.
- **Interrupts:** Hardware-level signals sent from peripheral controllers (NIC, disk controller, timer) to the CPU, causing an interrupt handler to run.  
SysMon monitors `ctxt` and `intr` in `/proc/stat` and computes the delta per second to alert engineers to scheduling bottlenecks.

### Q21: What is the difference between Minor and Major Page Faults?
**Answer:**  
- **Minor Page Fault (`minflt`):** Occurs when a virtual memory address is accessed that does not currently have a physical page mapping in the page table, but the page is already in physical RAM (e.g. shared library page or newly allocated zero-fill page). Handled rapidly by the kernel MMU.
- **Major Page Fault (`majflt`):** Occurs when the requested page is not in RAM and must be read from disk storage or swap space. Major faults cause significant latency (milliseconds vs nanoseconds).  
SysMon parses `/proc/vmstat` to display instantaneous page fault rates.

### Q22: How does SysMon detect whether it is executing inside WSL2 or native Linux?
**Answer:**  
SysMon reads `/proc/version`. Under WSL2, the kernel version string contains the substring `"microsoft"` or `"WSL2"` (e.g., `Linux version 5.15.153.1-microsoft-standard-WSL2`). If detected, SysMon identifies the platform as WSL2 and adjusts hypervisor expectations accordingly.

---

## Section 5: Terminal User Interface & System Interaction

### Q23: How does SysMon implement non-blocking keyboard input on a Linux terminal?
**Answer:**  
By default, Linux terminal standard input operates in **canonical mode**, buffering characters until Enter is pressed and echoing characters back to the screen.  
SysMon modifies terminal flags using the POSIX `termios` API:
```c
struct termios raw = orig_termios;
raw.c_lflag &= ~(ECHO | ICANON | ISIG);
raw.c_cc[VMIN] = 0;   // Non-blocking read
raw.c_cc[VTIME] = 0;  // 0 timeout
tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
```
It then checks for key readiness using `select()` or `poll()`. If a key is waiting, `read()` consumes the character instantly without blocking or waiting for Enter.

### Q24: How does SysMon prevent terminal screen flickering during live refresh?
**Answer:**  
Naive terminal applications clear the screen with `\033[2J` and print line-by-line, causing visible flicker. SysMon avoids this by:
1. Pre-building the entire terminal display frame in memory inside a reserved `std::string buffer`.
2. Repositioning the cursor to the top-left coordinate (`\033[H`) rather than wiping the screen.
3. Flushing the complete frame to `std::cout` in a single system write operation.

### Q25: How does the application handle window resize events?
**Answer:**  
SysMon queries the current terminal geometry on every render pass using the `ioctl` system call with command `TIOCGWINSZ`:
```c
struct winsize ws;
ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
// ws.ws_row and ws.ws_col contain live dimensions
```
This dynamically adapts table column widths, command truncation lengths, and progress bar spans whenever the user resizes their terminal window.

---

## Section 6: Architecture Decisions & Project Evaluation

### Q26: What happens if SysMon is run on a system where the kernel driver is not loaded?
**Answer:**  
SysMon was designed with high fault tolerance. When `KernelDriverClient::initialize()` fails to open `/dev/sysmon_driver` (due to missing module or non-root permissions), it logs a warning and marks the driver status as `OFFLINE / FALLBACK`. The application then smoothly routes all telemetry queries to the user-space `ProcReader` without any crash or loss of user interface functionality.

### Q27: If you had more time, what future improvements would you introduce?
**Answer:**  
1. **eBPF Tracing:** Utilize BPF CO-RE (Compile Once – Run Everywhere) programs to monitor file I/O latency and process lifecycle events without requiring out-of-tree kernel modules.
2. **cgroups v2 Support:** Group process metrics by systemd slice and Docker container ID to monitor containerized microservices.
3. **GPU Telemetry:** Integrate NVIDIA NVML / ROCm libraries for GPU core and VRAM tracking.
4. **Network Socket Telemetry:** Parse `/proc/net/tcp` to list active network connections per process.

### Q28: How would you summarize the core takeaway from this project in one sentence?
**Answer:**  
SysMon demonstrates the power of bridging low-level Linux kernel module development with modern, thread-safe C++ systems programming to produce an efficient, low-overhead resource profiling platform.
