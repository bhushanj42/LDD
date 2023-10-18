obj-m := pcd_sysfs.o
pcd_sysfs-objs += pcd_platform_driver_dt_sysfs.o pcd_syscalls.o

ARCH=arm
KERNELDIR ?= /lib/modules/$(shell uname -r)/build

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNELDIR) M=$(shell pwd) clean
host:
	$(MAKE) -C $(KERNELDIR) M=$(shell pwd) modules
