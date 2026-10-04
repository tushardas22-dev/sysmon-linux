#ifndef SYSMON_DRIVER_H
#define SYSMON_DRIVER_H

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/ioctl.h>
#elif defined(_WIN32)
#include <stdint.h>
#ifndef _IOR
#define _IOR(type,nr,size) ((unsigned long)(type | (nr << 8)))
#define _IO(type,nr)       ((unsigned long)(type | (nr << 8)))
#endif
#else
#include <stdint.h>
#include <sys/ioctl.h>
#endif

#define SYSMON_DEVICE_NAME "sysmon_driver"
#define SYSMON_CLASS_NAME  "sysmon"
#define SYSMON_MAGIC 'S'

#define MAX_PROC_ENTRIES 64
#define COMM_LEN 16

/**
 * Structure passed between Kernel and User-space containing
 * system-wide telemetry collected directly in kernel space.
 */
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
    uint32_t load_avg[3];        /* 1, 5, 15 min load scaled by 65536 */
    uint64_t sample_timestamp_ns;
};

/**
 * Structure containing snapshot of a process task directly
 * traversed from kernel task_struct.
 */
struct sysmon_proc_entry {
    int32_t pid;
    int32_t tgid;
    int32_t ppid;
    uint32_t uid;
    int32_t state;
    int32_t priority;
    int32_t static_prio;
    uint64_t utime_ticks;
    uint64_t stime_ticks;
    uint64_t rss_pages;
    uint64_t total_vm_pages;
    char comm[COMM_LEN];
};

/**
 * Payload for process table query ioctl
 */
struct sysmon_proc_table {
    uint32_t count;
    struct sysmon_proc_entry entries[MAX_PROC_ENTRIES];
};

/* IOCTL Command Definitions */
#define SYSMON_IOCTL_GET_METRICS _IOR(SYSMON_MAGIC, 1, struct sysmon_kernel_metrics)
#define SYSMON_IOCTL_GET_PROCS   _IOR(SYSMON_MAGIC, 2, struct sysmon_proc_table)
#define SYSMON_IOCTL_RESET_STATS _IO(SYSMON_MAGIC, 3)

#endif /* SYSMON_DRIVER_H */
