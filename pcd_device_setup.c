#include <linux/module.h>
#include <linux/platform_device.h>
#include "platform.h"

struct pcdev_platform_data pcdev_data[] = {
    [0] = {.size = 512, .permission = READWRITE, .serial_number = "PCDEV111"},
    [1] = {.size = 512, .permission = READWRITE, .serial_number = "PCDEV222"},
    [2] = {.size = 128, .permission = READONLY, .serial_number = "PCDEV333"},
    [3] = {.size = 32, .permission = WRITEONLY, .serial_number = "PCDEV444"}
};

// Device release call back
void pcdev_release(struct device *dev) {
    pr_info("Device released\n");
}

struct platform_device platform_pcdev_1 = {
    .name = "pcdev-A1x",
    .id = 0,
    .dev = {
        .platform_data = &pcdev_data[0],
        .release = pcdev_release
    }
};

struct platform_device platform_pcdev_2 = {
    .name = "pcdev-B1x",
    .id = 1,
    .dev = {
        .platform_data = &pcdev_data[1],
        .release = pcdev_release
    }
};

struct platform_device platform_pcdev_3 = {
    .name = "pcdev-C1x",
    .id = 2,
    .dev = {
        .platform_data = &pcdev_data[2],
        .release = pcdev_release
    }
};

struct platform_device platform_pcdev_4 = {
    .name = "pcdev-D1x",
    .id = 3,
    .dev = {
        .platform_data = &pcdev_data[3],
        .release = pcdev_release
    }
};

struct platform_device *platform_pcdevs[] = {
    &platform_pcdev_1,
    &platform_pcdev_2,
    &platform_pcdev_3,
    &platform_pcdev_4
};

static int __init pcdev_platform_init(void)
{
    // register platform device
    platform_add_devices(platform_pcdevs, ARRAY_SIZE(platform_pcdevs));
    //platform_device_register(&platform_pcdev_1);
    //platform_device_register(&platform_pcdev_2);

    pr_info("PCD device setup module inserted\n");
    return 0;
}

static void __exit pcdev_platform_exit(void)
{
    platform_device_unregister(&platform_pcdev_1);
    platform_device_unregister(&platform_pcdev_2);
    platform_device_unregister(&platform_pcdev_3);
    platform_device_unregister(&platform_pcdev_4);
    pr_info("PCD device setup module removed\n");
}

module_init(pcdev_platform_init);
module_exit(pcdev_platform_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module to register platform device");