#include "pcd_platform_driver_dt_sysfs.h"

loff_t sampleDrv_lseek (struct file *filep, loff_t offset, int whence)
{
	pr_err("LLSeek\n");
	return 0;
}

ssize_t sampleDrv_read (struct file *filep, char __user *buff, size_t count, loff_t *f_pos)
{
	pr_err("Read for %zu bytes\n", count);
	pr_err("Current file pos = %lld\n", *f_pos);
	return count;
}

ssize_t sampleDrv_write (struct file *filep, const char __user *buff, size_t count, loff_t *f_pos)
{	
	pr_err("Wrote for %zu bytes\n", count);
	pr_err("Current file pos = %lld\n", *f_pos);
	return count;
}

int sampleDrv_open (struct inode *inode, struct file *filep)
{
	pr_err("Open\n");
	return 0;
}

int sampleDrv_release (struct inode *inode, struct file *filep)
{
	pr_err("Release\n");
	return 0;
}
