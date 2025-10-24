#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <lotto/qemu.h>

MODULE_LICENSE("BSD");
MODULE_AUTHOR("DRC");
MODULE_DESCRIPTION("A simple concurrency bug demo");
MODULE_VERSION("0.01");

#define PROCFS_NAME "example"

volatile int next_id = 0;

static int
get_unique_id()
{
    /* Here is the bug. We need to lock here */
    volatile int id = next_id;
    printk(KERN_INFO "id: %d\n", id);
    next_id = id + 1;

    return id;
}

static ssize_t
read(struct file *file, char __user *buffer, size_t count, loff_t *position)
{
    int id = get_unique_id();

    if (count != sizeof(int)) {
        printk(KERN_ERR "Wrong request\n");
        lotto_fail();
    }

    if (*position > 0) {
        return 0;
    }

    if (copy_to_user(buffer, &id, count)) {
        return -EFAULT;
    }

    return count;
}

static int
open(struct inode *inode, struct file *file)
{
    if (next_id > 1) {
        next_id = 0;
    }

    return single_open(file, NULL, NULL);
}

static loff_t
lseek(struct file *file, loff_t off, int flags)
{
    return 0;
}

static const struct proc_ops fops = {
    .proc_open    = open,
    .proc_read    = read,
    .proc_release = single_release,
    .proc_lseek   = lseek,
};

static struct proc_dir_entry *proc_file;

static int __init
demo_init(void)
{
    printk(KERN_INFO "Demo module init\n");

    proc_file = proc_create(PROCFS_NAME, 0666, NULL, &fops);
    if (proc_file == NULL) {
        printk(KERN_ERR "Cannot create mandatory procfs\n");
        lotto_fail();
    }

    printk(KERN_INFO "/proc/%s created\n", PROCFS_NAME);

    return 0;
}

static void __exit
demo_exit(void)
{
    proc_remove(proc_file);
    printk(KERN_INFO "Demo module deinit\n");
}

module_init(demo_init);
module_exit(demo_exit);
