#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include "platform.h"

#define MAX_DEVICES 10

struct pcdev_private_data
{
    struct pcdev_platform_data pdata;
    char *buffer;
    dev_t dev_num;
    struct cdev cdev;
};

struct pcdrv_private_data
{
    int total_devices;
    dev_t device_num_base;
    struct class *class_pcd;
    struct device *device_pcd;
};

struct pcdrv_private_data pcdrv_data;

int check_permission(int permission, int access_mode) {
    if (permission == READWRITE)
        return 0;

    if (permission == READONLY && (access_mode & FMODE_READ) && !(access_mode & FMODE_WRITE))
        return 0;

    if (permission == WRITEONLY && !(access_mode & FMODE_READ) && (access_mode & FMODE_WRITE))
        return 0;
    
    return -EPERM;
}

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{
    struct pcdev_private_data *dev_data = (struct pcdev_private_data *)filp->private_data;

    pr_info("Lseek: current file position %lld\n", filp->f_pos);
    switch (whence) {
        case SEEK_SET:
            if (offset > dev_data->pdata.size || offset < 0)
                return -EINVAL;
            filp->f_pos = offset;
            break;
        case SEEK_CUR:
            if (filp->f_pos + offset > dev_data->pdata.size || filp->f_pos + offset < 0)
                return -EINVAL;
            filp->f_pos += offset;
            break;
        case SEEK_END:
            if (dev_data->pdata.size + offset > dev_data->pdata.size || dev_data->pdata.size + offset < 0)
                return -EINVAL;
            filp->f_pos = dev_data->pdata.size + offset;
            break;
        default:
            return -EINVAL;
    }
    pr_info("Lseek: new file position %lld\n", filp->f_pos);

    return filp->f_pos;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t size, loff_t *f_pos)
{
    struct pcdev_private_data *dev_data = (struct pcdev_private_data *)filp->private_data;

    pr_info("PCD Requested %i, file position %lld\n", size, *f_pos);

    if (*f_pos + size > dev_data->pdata.size) {
        size = dev_data->pdata.size - *f_pos;
    }

    if (copy_to_user(buff, dev_data->buffer + (*f_pos), size)) {
        return -EFAULT;
    }

    *f_pos += size;
    
    pr_info("PCD Returned %i, file position %lld\n", size, *f_pos);

    return size;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t size, loff_t *f_pos)
{
   struct pcdev_private_data *dev_data = (struct pcdev_private_data *)filp->private_data;

    pr_info("PCD write requested %i; starting position %lld\n", size, *f_pos);

    if (*f_pos + size > dev_data->pdata.size) {
        size = dev_data->pdata.size - *f_pos;
    }

    if (!size) {
        pr_err("No space left on device\n");
        return -ENOMEM;
    }

    if (copy_from_user(dev_data->buffer + (*f_pos), buff, size)) {
        return -EFAULT;
    }

    *f_pos += size;
    
    pr_info("PCD written %i, file position %lld\n", size, *f_pos);

    return size;


}

int pcd_open(struct inode *inode, struct file *filp)
{
    int ret, minor_number;
    struct pcdev_private_data *dev_data;

    // locate which device file called
    minor_number = MINOR(inode->i_rdev);
    pr_info("Minor number: %d\n", minor_number);

    // Use macro to locate which device struct is being called
    dev_data = container_of(inode->i_cdev, struct pcdev_private_data, cdev);

    // Store the device struct pointer in the file's private data
    filp->private_data = dev_data;

    ret = check_permission(dev_data->pdata.permission, filp->f_mode);

    (!ret) ? pr_info("Opened\n") : pr_info("Error opening\n");
    
    return ret;

}

int pcd_release(struct inode *inode, struct file *filp)
{
    pr_info("Closed\n");
    return 0;
}

struct file_operations pcd_fops = {
    .llseek = pcd_lseek,
    .read = pcd_read,
    .write = pcd_write,
    .open = pcd_open,
    .release = pcd_release,
    .owner = THIS_MODULE
};

// called when device removed from the system
int pcd_platform_driver_remove(struct platform_device *pdev) {
    struct pcdev_private_data *dev_data = dev_get_drvdata(&pdev->dev);

    // Remove device
    pr_info("Device %s destroy ...\n", dev_data->pdata.serial_number);
    device_destroy(pcdrv_data.class_pcd, dev_data->dev_num);

    // Remove cdev
    pr_info("cdev del ...\n");
    cdev_del(&dev_data->cdev);

    // Clean up memory
    pr_info("kfree buffer ...\n");
    kfree(dev_data->buffer);

    pr_info("kfree dev data ...\n");
    kfree(dev_data);

    pcdrv_data.total_devices--;

    pr_info("Device is removed\n");
    return 0;
}

// Called when matched platform device found
int pcd_platform_driver_probe(struct platform_device *pdev) {
    int ret = 0;

    struct pcdev_private_data *dev_data;
    struct pcdev_platform_data *tmp_pdata;

    pr_info("A device is detected\n");

    // Dynamically allocate memory for the device private data
    dev_data = devm_kzalloc(&pdev->dev, sizeof(*dev_data), GFP_KERNEL);
    if (!dev_data) {
        pr_info("Cannot allocate memory\n");
        ret = -ENOMEM;
        goto out;
    }
    dev_set_drvdata(&pdev->dev, dev_data);      // Save the device private data pointer in platform device data structure
    pr_info("Allocated %d for dev_data\n", sizeof(*dev_data));

    // Get the platform data
    tmp_pdata = (struct pcdev_platform_data *)dev_get_platdata(&pdev->dev);
    if (!tmp_pdata) {
        pr_info("No platform data available\n");
        ret = -EINVAL;
        goto dev_data_free;
    }
    dev_data->pdata = *tmp_pdata;       // Copy temporary platform data into device
    pr_info("Device serial number %s; Permissions: %d; Size: %d\n", dev_data->pdata.serial_number, dev_data->pdata.permission, dev_data->pdata.size);

    // Dynamically allocate memory for the device buffer using size information from the platform data
    dev_data->buffer = devm_kzalloc(&pdev->dev, dev_data->pdata.size, GFP_KERNEL);
    if (!dev_data) {
        pr_info("Cannot allocate memory\n");
        ret = -ENOMEM;
        goto dev_data_free;
    }
    pr_info("Allocated %d for buffer\n", dev_data->pdata.size);

    // Get the device number
    dev_data->dev_num = pcdrv_data.device_num_base + pdev->id;

    // cdev init and cdev add
    cdev_init(&dev_data->cdev, &pcd_fops);

    dev_data->cdev.owner = THIS_MODULE;
    ret = cdev_add(&dev_data->cdev, dev_data->dev_num, 1);
    if (ret < 0) {
        goto buffer_free;
    }

    // device file creation: populate sysfs with device information
    pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, dev_data->dev_num, NULL, "pcd-%d", pdev->id);
    if (IS_ERR(pcdrv_data.device_pcd)) {
        ret = PTR_ERR(pcdrv_data.device_pcd);
        pr_err("Device creation failed: %i\n", ret);
        goto cdev_del;
    }

    pcdrv_data.total_devices++;
    
    pr_info("Probe was successful\n");
    return 0;

cdev_del:
    cdev_del(&dev_data->cdev);
buffer_free:
    devm_kfree(&pdev->dev, dev_data->buffer);
dev_data_free:
    devm_kfree(&pdev->dev, dev_data);
out:
    pr_info("Device probe failed\n");

    return ret;
}


struct platform_driver pcd_platform_driver = {
    .probe = pcd_platform_driver_probe,
    .remove = pcd_platform_driver_remove,
    .driver = {
        .name = "pseudo_char_device"
    }
};

static int __init pcd_platform_driver_init(void)
{
    int ret;

    // Dynamically allocate a device number for MAX_DEVICES
    ret = alloc_chrdev_region(&pcdrv_data.device_num_base, 0, MAX_DEVICES, "pcdevs");
    if (ret < 0) {
        pr_err("alloc_chrdev_region failed\n");
        return ret;
    }

    // Create device class under /sys/class
    pcdrv_data.class_pcd = class_create(THIS_MODULE, "pcd_class");
    if (IS_ERR(pcdrv_data.class_pcd)) {
        pr_err("Class creation failed\n");
        ret = PTR_ERR(pcdrv_data.class_pcd);
        unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICES);
        return ret;
    }
    
    // Register platform driver
    platform_driver_register(&pcd_platform_driver);
    pr_info("PCD platform driver loaded\n");
    return 0;

}

static void __exit pcd_platform_driver_cleanup(void)
{
    // Unregister platform driver
    platform_driver_unregister(&pcd_platform_driver);

    // Class destroy
    class_destroy(pcdrv_data.class_pcd);

    // Unregister chrdev region
    unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICES);

    pr_info("PCD platform driver unloaded\n");
}

module_init(pcd_platform_driver_init);
module_exit(pcd_platform_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mountjoy");
MODULE_DESCRIPTION("Pseudo character driver");

