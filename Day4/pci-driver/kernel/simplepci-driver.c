// kernel/simplepci_msg.c

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/mutex.h>

#define DRIVER_NAME "simplepci"
#define CLASS_NAME "simplepci_class"
#define DEVICE_NAME "simplepci"

#define VENDOR_ID 0x1234
#define DEVICE_ID 0x11e8
#define MSG_BUF_SIZE 256

static int major;
static struct class *simplepci_class = NULL;
static struct device *simplepci_device = NULL;
static void __iomem *hw_addr;
static struct pci_dev *pdev_global;

static char msg_buf[MSG_BUF_SIZE] = {0};
static size_t msg_len = 0;
static DEFINE_MUTEX(msg_mutex);

static int simplepci_open(struct inode *inode, struct file *file) {
    pr_info("simplepci: device opened\n");
    return 0;
}

static int simplepci_release(struct inode *inode, struct file *file) {
    pr_info("simplepci: device closed\n");
    return 0;
}

static ssize_t simplepci_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    ssize_t to_copy;

    if (mutex_lock_interruptible(&msg_mutex))
        return -ERESTARTSYS;

    if (*offset >= msg_len) {
        mutex_unlock(&msg_mutex);
        return 0; // EOF
    }

    to_copy = min(len, msg_len - *offset);

    if (copy_to_user(buf, msg_buf + *offset, to_copy)) {
        mutex_unlock(&msg_mutex);
        return -EFAULT;
    }

    *offset += to_copy;
    mutex_unlock(&msg_mutex);
    return to_copy;
}

static ssize_t simplepci_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    ssize_t to_copy;

    if (len == 0 || len >= MSG_BUF_SIZE)
        return -EINVAL;

    if (mutex_lock_interruptible(&msg_mutex))
        return -ERESTARTSYS;

    memset(msg_buf, 0, MSG_BUF_SIZE);
    to_copy = min(len, MSG_BUF_SIZE - 1);

    if (copy_from_user(msg_buf, buf, to_copy)) {
        mutex_unlock(&msg_mutex);
        return -EFAULT;
    }

    msg_len = to_copy;
    mutex_unlock(&msg_mutex);
    pr_info("simplepci: Message written: %s\n", msg_buf);
    return to_copy;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = simplepci_open,
    .release = simplepci_release,
    .read = simplepci_read,
    .write = simplepci_write,
};

static struct pci_device_id pci_ids[] = {
    { PCI_DEVICE(VENDOR_ID, DEVICE_ID), },
    { 0 }
};
MODULE_DEVICE_TABLE(pci, pci_ids);

static int simplepci_probe(struct pci_dev *pdev, const struct pci_device_id *id) {
    int err;

    pr_info(DRIVER_NAME ": Found device\n");
    pdev_global = pdev;

    err = pci_enable_device(pdev);
    if (err) return err;

    err = pci_request_region(pdev, 0, DRIVER_NAME);
    if (err) return err;

    hw_addr = pci_iomap(pdev, 0, pci_resource_len(pdev, 0));
    if (!hw_addr) {
        pci_release_region(pdev, 0);
        return -ENOMEM;
    }

    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) return major;

    simplepci_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(simplepci_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(simplepci_class);
    }

    simplepci_device = device_create(simplepci_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(simplepci_device)) {
        class_destroy(simplepci_class);
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(simplepci_device);
    }

    pr_info(DRIVER_NAME ": /dev/%s created\n", DEVICE_NAME);
    return 0;
}

static void simplepci_remove(struct pci_dev *pdev) {
    if (hw_addr)
        pci_iounmap(pdev, hw_addr);

    pci_release_region(pdev, 0);
    device_destroy(simplepci_class, MKDEV(major, 0));
    class_destroy(simplepci_class);
    unregister_chrdev(major, DEVICE_NAME);

    pr_info(DRIVER_NAME ": Device removed\n");
}

static struct pci_driver simplepci_driver = {
    .name = DRIVER_NAME,
    .id_table = pci_ids,
    .probe = simplepci_probe,
    .remove = simplepci_remove,
};

module_pci_driver(simplepci_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeganathan Swaminathan");
MODULE_DESCRIPTION("Simple PCI driver with string message read/write support");
