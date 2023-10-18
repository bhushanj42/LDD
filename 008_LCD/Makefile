obj-m := lcd_sysfs.o
lcd_sysfs-objs += lcd-sysfs.o gpio.o
ARCH=arm
KERNELDIR ?= /lib/modules/$(shell uname -r)/build

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNELDIR) M=$(shell pwd) clean
host:
	$(MAKE) -C $(KERNELDIR) M=$(shell pwd) modules
