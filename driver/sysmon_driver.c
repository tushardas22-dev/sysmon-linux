/**
 * sysmon_driver.c - Linux Character Device Driver for System Telemetry
 *
 * Exposes /dev/sysmon_driver to provide process task tables
 * and memory metrics to user space via IOCTL.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/sched/signal.h>
#include <linux/sched/loadavg.h>
#include <linux/sched/stat.h>
#include <linux/timekeeping.h>
#include <linux/mutex.h>
#include <linux/version.h>

#include "sysmon_driver.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Developer");
MODULE_DESCRIPTION("SysMon Linux Character Device Driver");
MODULE_VERSION("1.0");

static dev_t dev_num;
static struct cdev sysmon_cdev;
static struct class *sysmon_class = NULL;
static struct device *sysmon_device = NULL;

static DEFINE_MUTEX(sysmon_lock);
static unsigned long ioctl_invocation_count = 0;

/* File Operations: Open */
static int sysmon_open(struct inode *inodep, struct file *filep)
{
    pr_info("sysmon_driver: Device opened by PID %d (%s)\n", current->pid, current->comm);
    return 0;
}

/* File Operations: Release / Close */
static int sysmon_release(struct inode *inodep, struct file *filep)
{
    pr_info("sysmon_driver: Device closed by PID %d\n", current->pid);
    return 0;
}

/* Helper to gather kernel statistics */
static void populate_kernel_metrics(struct sysmon_kernel_metrics *metrics)
{
    struct sysinfo si;
    unsigned long avenrun[3];

    memset(metrics, 0, sizeof(*metrics));

    /* Populate standard sysinfo */
    si_meminfo(&si);
    si_swapinfo(&si);

    metrics->uptime_seconds = (uint64_t)ktime_get_seconds();
    metrics->total_ram_bytes = (uint64_t)si.totalram * si.mem_unit;
    metrics->free_ram_bytes = (uint64_t)si.freeram * si.mem_unit;
    metrics->shared_ram_bytes = (uint64_t)si.sharedram * si.mem_unit;
    metrics->buffer_ram_bytes = (uint64_t)si.bufferram * si.mem_unit;
    metrics->total_swap_bytes = (uint64_t)si.totalswap * si.mem_unit;
    metrics->free_swap_bytes = (uint64_t)si.freeswap * si.mem_unit;
    metrics->process_count = (uint32_t)si.procs;
    metrics->active_cpus = (uint32_t)num_online_cpus();

    /* Load averages */
    get_avenrun(avenrun, FIXED_1/200, 0);
    metrics->load_avg[0] = (uint32_t)avenrun[0];
    metrics->load_avg[1] = (uint32_t)avenrun[1];
    metrics->load_avg[2] = (uint32_t)avenrun[2];

    /* Sample timestamp in nanoseconds */
    metrics->sample_timestamp_ns = (uint64_t)ktime_get_real_ns();
}

/* Helper to gather top process snapshots safely using RCU */
static void populate_process_table(struct sysmon_proc_table *table)
{
    struct task_struct *task;
    uint32_t count = 0;

    memset(table, 0, sizeof(*table));

    rcu_read_lock();
    for_each_process(task) {
        if (count >= MAX_PROC_ENTRIES) {
            break;
        }

        table->entries[count].pid = (int32_t)task->pid;
        table->entries[count].tgid = (int32_t)task->tgid;
        table->entries[count].ppid = task->real_parent ? (int32_t)task->real_parent->pid : 0;
        table->entries[count].uid = from_kuid(&init_user_ns, task_uid(task));
        table->entries[count].state = (int32_t)task->__state;
        table->entries[count].priority = (int32_t)task->prio;
        table->entries[count].static_prio = (int32_t)task->static_prio;
        table->entries[count].utime_ticks = (uint64_t)task->utime;
        table->entries[count].stime_ticks = (uint64_t)task->stime;

        if (task->mm) {
            table->entries[count].rss_pages = get_mm_rss(task->mm);
            table->entries[count].total_vm_pages = task->mm->total_vm;
        } else {
            table->entries[count].rss_pages = 0;
            table->entries[count].total_vm_pages = 0;
        }

        get_task_comm(table->entries[count].comm, task);
        count++;
    }
    rcu_read_unlock();

    table->count = count;
}

/* File Operations: Unlocked IOCTL */
static long sysmon_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    int ret = 0;

    if (_IOC_TYPE(cmd) != SYSMON_MAGIC) {
        pr_warn("sysmon_driver: Invalid IOCTL magic received\n");
        return -ENOTTY;
    }

    mutex_lock(&sysmon_lock);
    ioctl_invocation_count++;

    switch (cmd) {
    case SYSMON_IOCTL_GET_METRICS: {
        struct sysmon_kernel_metrics metrics;
        populate_kernel_metrics(&metrics);
        if (copy_to_user((void __user *)arg, &metrics, sizeof(metrics))) {
            ret = -EFAULT;
        }
        break;
    }
    case SYSMON_IOCTL_GET_PROCS: {
        struct sysmon_proc_table *table;
        table = kmalloc(sizeof(*table), GFP_KERNEL);
        if (!table) {
            ret = -ENOMEM;
            break;
        }
        populate_process_table(table);
        if (copy_to_user((void __user *)arg, table, sizeof(*table))) {
            ret = -EFAULT;
        }
        kfree(table);
        break;
    }
    case SYSMON_IOCTL_RESET_STATS: {
        ioctl_invocation_count = 0;
        pr_info("sysmon_driver: Statistics reset\n");
        ret = 0;
        break;
    }
    default:
        ret = -EINVAL;
        break;
    }

    mutex_unlock(&sysmon_lock);
    return ret;
}

/* File operations structure definition */
static const struct file_operations sysmon_fops = {
    .owner          = THIS_MODULE,
    .open           = sysmon_open,
    .release        = sysmon_release,
    .unlocked_ioctl = sysmon_ioctl,
};

/* Module Initialization */
static int __init sysmon_init(void)
{
    int ret;

    pr_info("sysmon_driver: Initializing System Resource Monitoring Kernel Driver\n");

    /* Dynamically allocate character device numbers */
    ret = alloc_chrdev_region(&dev_num, 0, 1, SYSMON_DEVICE_NAME);
    if (ret < 0) {
        pr_err("sysmon_driver: Failed to allocate major number (error %d)\n", ret);
        return ret;
    }

    /* Initialize and add cdev structure */
    cdev_init(&sysmon_cdev, &sysmon_fops);
    sysmon_cdev.owner = THIS_MODULE;

    ret = cdev_add(&sysmon_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("sysmon_driver: Failed to add cdev (error %d)\n", ret);
        goto unregister_chrdev;
    }

    /* Create sysfs class */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    sysmon_class = class_create(SYSMON_CLASS_NAME);
#else
    sysmon_class = class_create(THIS_MODULE, SYSMON_CLASS_NAME);
#endif
    if (IS_ERR(sysmon_class)) {
        ret = PTR_ERR(sysmon_class);
        pr_err("sysmon_driver: Failed to create device class (error %d)\n", ret);
        goto delete_cdev;
    }

    /* Create device node in /dev */
    sysmon_device = device_create(sysmon_class, NULL, dev_num, NULL, SYSMON_DEVICE_NAME);
    if (IS_ERR(sysmon_device)) {
        ret = PTR_ERR(sysmon_device);
        pr_err("sysmon_driver: Failed to create device node (error %d)\n", ret);
        goto destroy_class;
    }

    pr_info("sysmon_driver: Registered successfully with Major %d, Minor %d\n",
            MAJOR(dev_num), MINOR(dev_num));
    pr_info("sysmon_driver: Device node created at /dev/%s\n", SYSMON_DEVICE_NAME);
    return 0;

destroy_class:
    class_destroy(sysmon_class);
delete_cdev:
    cdev_del(&sysmon_cdev);
unregister_chrdev:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

/* Module Cleanup */
static void __exit sysmon_exit(void)
{
    pr_info("sysmon_driver: Cleaning up System Resource Monitoring Kernel Driver\n");
    device_destroy(sysmon_class, dev_num);
    class_destroy(sysmon_class);
    cdev_del(&sysmon_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("sysmon_driver: Module unloaded successfully\n");
}

module_init(sysmon_init);
module_exit(sysmon_exit);
