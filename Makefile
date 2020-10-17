obj-m := pcd_device_setup.o pcd_platform_driver.o
ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-
KDIR=/home/dwebb/workspace/ldd/source/linux_bbb_4.14/
HOST_KDIR=/lib/modules/$(shell uname -r)/build/

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) clean

help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) help

host:
	make -C $(HOST_KDIR) M=$(PWD) host
