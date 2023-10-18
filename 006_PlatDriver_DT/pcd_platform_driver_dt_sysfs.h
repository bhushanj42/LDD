#ifndef PCD_PLATFORM_DRIVER_DT_SYS_H
#define PCD_PLATFORM_DRIVER_DT_SYS_H

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>

#include "platform_data.h"

loff_t sampleDrv_lseek (struct file *filep, loff_t offset, int whence);
ssize_t sampleDrv_read (struct file *filep, char __user *buff, size_t count, loff_t *f_pos);
ssize_t sampleDrv_write (struct file *filep, const char __user *buff, size_t count, loff_t *f_pos);
int sampleDrv_open (struct inode *inode, struct file *filep);
int sampleDrv_release (struct inode *inode, struct file *filep);

int pcd_platform_driver_probe(struct platform_device *pdev);
int pcd_platform_driver_remove(struct platform_device *pdev);

struct custom_dev_config {
	int config1;
	int config2;
};

/* Device private data structure */
struct pcdev_private_data {
	struct pcdev_platform_data pdata;
	char *buffer;
	dev_t dev_num;
	struct cdev cdev;
};

/* Driver private data structure */
struct pcdrv_private_data {
	int total_devices;
	dev_t device_num_base;
	struct class *class_pcd;
	struct device *device_pcd;
};

#endif

