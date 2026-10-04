# Linux Character Device Driver - `sysmon_driver`

## Overview
`sysmon_driver` is a custom Linux Character Device Driver developed as part of the 20-day training project demonstrating **Linux Device Drivers, System Programming, and Hardware-Software interaction**.

The driver creates a character device at `/dev/sysmon_driver` and provides user space with direct, zero-overhead kernel telemetry via `ioctl()` system calls.

---

## Technical Features
1. **Dynamic Character Device Registration**: Uses `alloc_chrdev_region` to obtain a major number dynamically.
2. **Device Node Creation**: Registers a sysfs class (`class_create`) and device (`device_create`), allowing `udev` to automatically generate `/dev/sysmon_driver`.
3. **Safe Process Traversal**: Uses `rcu_read_lock()` and `for_each_process(task)` to inspect kernel `struct task_struct` safely without deadlocks.
4. **Hardware & Subsystem Telemetry**:
   - `si_meminfo()` & `si_swapinfo()`: Physical and virtual memory statistics.
   - `get_avenrun()`: System load averages (1, 5, 15 minutes).
   - High-resolution monotonic kernel timestamps via `ktime_get_real_ns()`.
5. **Concurrency Protection**: Protected with `mutex_lock(&sysmon_lock)` to ensure multi-threaded user-space requests are serialized safely.

---

## Building and Loading

### Prerequisites (Ubuntu / Linux)
```bash
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)
```

### Note on WSL2
Standard default WSL2 kernels ship as monolithic kernels without loadable module support enabled by default. On native Ubuntu, standard VM (VirtualBox/VMware), or a custom WSL2 kernel with loadable module support (`CONFIG_MODULES=y`), the driver builds and loads seamlessly:

```bash
# In the driver directory:
make

# Insert the module into the Linux kernel:
sudo insmod sysmon_driver.ko

# Verify device node creation and kernel log messages:
ls -l /dev/sysmon_driver
dmesg | grep sysmon_driver

# Grant read/write permissions for user-space app:
sudo chmod 666 /dev/sysmon_driver

# Unload the module when done:
sudo rmmod sysmon_driver
```

If the kernel driver is not loaded, the C++ user-space application automatically detects this and gracefully falls back to reading `/proc` directly!
