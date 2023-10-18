obj-m := gpio-sysfs.o

ARCH=arm
KERNELDIR ?= /lib/modules/$(shell uname -r)/build

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNELDIR) M=$(shell pwd) clean
host:
	$(MAKE) -C $(KERNELDIR) M=$(shell pwd) modules
