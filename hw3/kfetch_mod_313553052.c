#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/utsname.h>
#include <linux/cpu.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/sysinfo.h>
#include <linux/mutex.h>
#include "kfetch.h"

#define DEVICE_NAME "kfetch"
#define BUF_SIZE 1024

static int major;
static int kfetch_mask = KFETCH_FULL_INFO; 
static struct class *cls;
static DEFINE_MUTEX(kfetch_mutex);
static char *ascii_art[] = {

    "        .-.        ",
    "       (.. |       ",
    "       <>  |       ",
    "      / --- \\       ",
    "     ( |   | )      ",
    "   |\\\\_)__(_//|    ",
    "  <__)------(__>  ",
};

static int split_lines(char *input, const char **lines, int max_lines) {
    int count = 0;
    char *start = input;

    while (*start && count < max_lines) {
        lines[count++] = start;
        start = strchr(start, '\n'); 
        if (start) {
            *start = '\0'; 
            start++;
        }
    }
    return count;
}

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *);

/* File operations */
static const struct file_operations kfetch_ops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
};


static void get_kernel_release(char *buf, size_t buf_size) {
    strncat(buf, "Kernel:    ", buf_size);
    strncat(buf, utsname()->release, buf_size);
    strncat(buf, "\n", buf_size);
}

static void get_cpu_model(char *buf, size_t buf_size) {
    unsigned int cpu = 0;
    struct cpuinfo_x86 *c;
    for_each_online_cpu(cpu) {
        c = &cpu_data(cpu);
        strncat(buf, "CPU:       ", buf_size);
        strncat(buf, c->x86_model_id, buf_size);
        strncat(buf, "\n", buf_size);
        break;
    }
}

static void get_cpu_cores(char *buf, size_t buf_size) {
    char temp[64];
    snprintf(temp, sizeof(temp), "CPUs:      %d / %d\n", num_online_cpus(), nr_cpu_ids);
    strncat(buf, temp, buf_size);
}

static void get_memory_info(char *buf, size_t buf_size) {
    struct sysinfo sys_info;
    char temp[64];
    si_meminfo(&sys_info);
    snprintf(temp, sizeof(temp), "Memory:    %lu MB / %lu MB\n",
             (sys_info.freeram << (PAGE_SHIFT - 10)) / 1024,
             (sys_info.totalram << (PAGE_SHIFT - 10)) / 1024);
    strncat(buf, temp, buf_size);
}

static void get_uptime(char *buf, size_t buf_size) {
    char temp[64];
    s64 uptime = ktime_divns(ktime_get_coarse_boottime(), NSEC_PER_SEC) / 60;
    snprintf(temp, sizeof(temp), "Uptime:    %lld mins\n", uptime);
    strncat(buf, temp, buf_size);
}

static void get_num_processes(char *buf, size_t buf_size) {
    struct task_struct *task;
    size_t count = 0;
    char temp[64];
    for_each_process(task) {
        count++;
    }
    snprintf(temp, sizeof(temp), "Processes: %zu\n", count);
    strncat(buf, temp, buf_size);
}

static void fetch_info(char *buf, size_t buf_size) {
    if (kfetch_mask & KFETCH_RELEASE) {
        get_kernel_release(buf, buf_size);
    }
    if (kfetch_mask & KFETCH_CPU_MODEL) {
        get_cpu_model(buf, buf_size);
    }
    if (kfetch_mask & KFETCH_NUM_CPUS) {
        get_cpu_cores(buf, buf_size);
    }
    if (kfetch_mask & KFETCH_MEM) {
        get_memory_info(buf, buf_size);
    }
    if (kfetch_mask & KFETCH_UPTIME) {
        get_uptime(buf, buf_size);
    }
    if (kfetch_mask & KFETCH_NUM_PROCS) {
        get_num_processes(buf, buf_size);
    }
}

/* Device open */
static int device_open(struct inode *inode, struct file *file) {
    if(!mutex_trylock(&kfetch_mutex)){
        return -EBUSY;
    }
    try_module_get(THIS_MODULE);
    return 0;
}

/* Device release */
static int device_release(struct inode *inode, struct file *file) {
    mutex_unlock(&kfetch_mutex);
    module_put(THIS_MODULE);
    return 0;
}


static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset) {

    static char info_buf[BUF_SIZE];
    static char formatted_buf[BUF_SIZE];
    char hostname_buf[64];
    const char *info_lines[7];
    int i, line_count;

    memset(info_buf, 0, sizeof(info_buf));
    memset(formatted_buf, 0, sizeof(formatted_buf));
    memset(hostname_buf, 0, sizeof(hostname_buf));


    snprintf(hostname_buf, sizeof(hostname_buf), "%s", utsname()->nodename);


    fetch_info(info_buf, sizeof(info_buf));


    line_count = split_lines(info_buf, info_lines, 7);


    for (i = 0; i < 7; i++) {
        if (i == 0) {
            snprintf(formatted_buf + strlen(formatted_buf), sizeof(formatted_buf) - strlen(formatted_buf),
                     "%-20s  %s\n", ascii_art[i], hostname_buf);
        } else if (i == 1) {
            snprintf(formatted_buf + strlen(formatted_buf), sizeof(formatted_buf) - strlen(formatted_buf),
                     "%-20s  %s\n", ascii_art[i], "-----------------------------");
        } else if (i - 2 < line_count) {
            snprintf(formatted_buf + strlen(formatted_buf), sizeof(formatted_buf) - strlen(formatted_buf),
                     "%-20s  %s\n", ascii_art[i], info_lines[i - 2]);
        } else {
            snprintf(formatted_buf + strlen(formatted_buf), sizeof(formatted_buf) - strlen(formatted_buf),
                     "%-20s\n", ascii_art[i]);
        }
    }

    if (copy_to_user(buffer, formatted_buf, strlen(formatted_buf))) {
        return -EFAULT;
    }

    return strlen(formatted_buf);
}



static ssize_t device_write(struct file *filp, const char __user *buffer, size_t length, loff_t *off) {
    int new_mask;
    if (copy_from_user(&new_mask, buffer, sizeof(new_mask))) {
        return -EFAULT;
    }
    kfetch_mask = new_mask; /* Update mask */
    return sizeof(new_mask);
}


static int __init kfetch_init(void) {
    major = register_chrdev(0, DEVICE_NAME, &kfetch_ops);
    if (major < 0) {
        pr_alert("Failed to register char device\n");
        return major;
    }
    
    cls = class_create(DEVICE_NAME);

    if (IS_ERR(cls)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(cls);
    }
    device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    pr_info("kfetch module loaded with device /dev/%s\n", DEVICE_NAME);
    return 0;
}


static void __exit kfetch_exit(void) {
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, DEVICE_NAME);
    pr_info("kfetch module unloaded\n");
}

module_init(kfetch_init);
module_exit(kfetch_exit);

MODULE_LICENSE("GPL");
