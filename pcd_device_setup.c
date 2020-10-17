#include <linux/module.h>
#include <linux/platform_device.h>
#include "platform.h"

struct pcdev_platform_data pcdev_data[2] = {
    [0] = {.size = 512, .permission = READWRITE, .serial_number = "PCDEV111"},
    [1] = {.size = 512, .permission = READWRITE, .serial_number = "PCDEV222"}
};

// Device release call back
void pcdev_release(struct device *dev) {
    pr_info("Device released\n");
}

struct platform_device platform_pcdev_1 = {
    .name = "pseudo_char_device",
    .id = 0,
    .dev = {
        .platform_data = &pcdev_data[0],
        .release = pcdev_release
    }
};

struct platform_device platform_pcdev_2 = {
    .name = "pseudo_char_device",
    .id = 1,
    .dev = {
        .platform_data = &pcdev_data[1],
        .release = pcdev_release
    }

};

static int __init pcdev_platform_init(void)
{
    // register platform device
    platform_device_register(&platform_pcdev_1);
    platform_device_register(&platform_pcdev_2);

    pr_info("PCD device setup module inserted\n");
    return 0;
}

static void __exit pcdev_platform_exit(void)
{
    platform_device_unregister(&platform_pcdev_1);
    platform_device_unregister(&platform_pcdev_2);
    pr_info("PCD device setup module removed\n");
}

module_init(pcdev_platform_init);
module_exit(pcdev_platform_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module to register platform device");