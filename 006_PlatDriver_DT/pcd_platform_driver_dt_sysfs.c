#include "pcd_platform_driver_dt_sysfs.h"

struct file_operations fops = 
{
	.open = sampleDrv_open,
	.write = sampleDrv_write,
	.read = sampleDrv_read,
	.llseek = sampleDrv_lseek,
	.owner = THIS_MODULE
};

struct custom_dev_config pcdev_config[] = {
	{.config1 = 10, .config2 = 20},
	{.config1 = 60, .config2 = 30},
	{.config1 = 70, .config2 = 40},
	{.config1 = 80, .config2 = 50}
};

struct platform_device_id pcdevs_id[] = {
	{.name = "pseudo_chrdev", .driver_data = 0},
	{ /* sentinel */ } 
};

struct of_device_id pcdev_dt_match[] = {
	{.compatible = "pcdev-A1X", .data = (void *)0},
	{.compatible = "pcdev-B1X", .data = (void *)1},
	{.compatible = "pcdev-C1X", .data = (void *)2},
	{.compatible = "pcdev-D1X", .data = (void *)3},
	{}
};

struct platform_driver struct_platform_driver = {
	.probe = pcd_platform_driver_probe,
	.remove = pcd_platform_driver_remove,
	.id_table = pcdevs_id,
	.driver = {
		.name = "pseudo_chrdev",
		.of_match_table = pcdev_dt_match
	}
};

struct pcdrv_private_data pcdrv_data;


ssize_t show_max_size(struct device *dev, struct device_attribute *attr, char *buf){
	/* Get access to the device private data */
	struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);
	
	return sprintf(buf, "%d\n", dev_data->pdata.size);
}

ssize_t store_max_size(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
	long result;
	int ret;
	struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);
	
	ret = kstrtol(buf, 10, &result);
	if(ret){
		return ret;
	}
	
	dev_data->pdata.size = result;
	
	dev_data->buffer = krealloc(dev_data->buffer, dev_data->pdata.size, GFP_KERNEL);
	
	return count;
}

ssize_t show_serial_num(struct device *dev, struct device_attribute *attr, char *buf){
	/* Get access to the device private data */
	struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);
	
	return sprintf(buf, "%s\n", dev_data->pdata.serial_data);
}
			 
/* Create 2 variables of struct device_attribute */
static DEVICE_ATTR(max_size, S_IRUGO | S_IWUSR, show_max_size, store_max_size);
static DEVICE_ATTR(serial_num, S_IRUGO, show_serial_num, NULL);

int pcd_sysfs_create_files(struct device *pcd_dev){
	int ret;
	
	ret = sysfs_create_file(&pcd_dev->kobj, &dev_attr_max_size.attr);
	if(ret){
		return ret;
	}
	
	return sysfs_create_file(&pcd_dev->kobj, &dev_attr_serial_num.attr);
}

int pcd_platform_driver_probe(struct platform_device *pdev){
	
	struct pcdev_private_data *dev_data;
	struct pcdev_platform_data *pdata;
	int ret;
	
	pr_err("%s:", __func__);
	
	/*1. Get the platform data */
/*	pdata = pdev->dev.platform_data;*/
	pdata = (struct pcdev_platform_data *)dev_get_platdata(&pdev->dev);
	if(!pdata){
		pr_err("No platform data available!!!");
		return -EINVAL;
	}
	
	/*2. Dynamically allocate memory for the device private data */
	dev_data = kzalloc(sizeof(struct pcdev_private_data), GFP_KERNEL);
	if(!dev_data){
		pr_err("Can't allocate memory");
		return -ENOMEM;
	}
	dev_data->pdata.size = pdata->size;
	dev_data->pdata.perm = pdata->perm;
	dev_data->pdata.serial_data = pdata->serial_data;
	
	pr_err("Device size %d\n", dev_data->pdata.size);
	pr_err("Device perm %d\n", dev_data->pdata.perm);
	pr_err("Device Serial num %s\n", dev_data->pdata.serial_data);
	pr_err("Config item 1 = %d", pcdev_config[pdev->id_entry->driver_data].config1);
	pr_err("Config item 2 = %d", pcdev_config[pdev->id_entry->driver_data].config2);
	
	/*3. Dynamically allocate memory for the device buffer using size information from the platform data */
	dev_data->buffer = kzalloc(dev_data->pdata.size, GFP_KERNEL);
	if(!dev_data->buffer){
		pr_err("Can't allocate memory for buffer");
		return -ENOMEM;
	}
	
	pdev->dev.driver_data = dev_data;
	
	/*4. Get the device number  */
	dev_data->dev_num = pcdrv_data.device_num_base + pdev->id;
	
	/*5. Do cdev_init and cedv_add */
	cdev_init(&dev_data->cdev, &fops);
	dev_data->cdev.owner = THIS_MODULE;
	ret = cdev_add(&dev_data->cdev, dev_data->dev_num, 1);
	if(ret){
		pr_err("cdev add failed");
		return ret;
	}
	
	/*6. Create device file for the detected platform data */
	pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, dev_data->dev_num, NULL, "Plat_Dri_pcdev_%d", pdev->id);
	if(IS_ERR(pcdrv_data.device_pcd))
	{
		pr_err("Device creation failed");
		ret = PTR_ERR(pcdrv_data.device_pcd);
		cdev_del(&dev_data->cdev);
		return ret;
	}
	
	pcdrv_data.total_devices++;
	
	ret = pcd_sysfs_create_files(pcdrv_data.device_pcd);
	if(ret){
		dev_err(&pdev->dev, "Problem in creating sysfs entries");
		device_destroy(pcdrv_data.class_pcd, dev_data->dev_num);
		return ret;
	}

	pr_err("%s:A device(driver) is detected", __func__);
	
	return 0;
}

int pcd_platform_driver_remove(struct platform_device *pdev){
	struct pcdev_private_data *dev_data = dev_get_drvdata(&pdev->dev);
	
	pr_err("%s:", __func__);
	
	/*1. Remove a device that was created with a device_create() */
	device_destroy(pcdrv_data.class_pcd, dev_data->dev_num);
	
	/*2. Remove a cdev entry from the system */
	cdev_del(&dev_data->cdev);
	
	/*3. Free the memory held by the device */
	kfree(dev_data->buffer);
	kfree(dev_data);

	pcdrv_data.total_devices--;
	
	pr_err("%s:A device(driver) is removed", __func__);
	return 0;
}

static int __init platform_driver_init(void) {
	/* Register platform driver*/
	
	int ret;
	
	pr_err("%s:", __func__);
	
	/*1. Dynamically allocate device numbers*/
	ret = alloc_chrdev_region(&pcdrv_data.device_num_base, 0, 1, "Platform_Driver_device");
	if (ret){
		pr_err("Problem in allocating device numbers");
		return ret;
	}
	
	/*2. Create device class under /sys/class */
	pcdrv_data.class_pcd = class_create(THIS_MODULE, "pcd_class");
	if(IS_ERR(pcdrv_data.class_pcd))
	{
		pr_err("Class creation failed");
		ret = PTR_ERR(pcdrv_data.class_pcd);
		unregister_chrdev_region(pcdrv_data.device_num_base, 1);
		return ret;
	}
	
	/*3. Register a platform driver */
	ret = platform_driver_register(&struct_platform_driver);
	if(ret){
		pr_err("Couldn't register platform driver");
		class_destroy(pcdrv_data.class_pcd);
		unregister_chrdev_region(pcdrv_data.device_num_base, 1);
	}
	pr_err("%s:Platform Driver initialised", __func__);

	return 0;
}

static void __exit platform_driver_exit(void) {
	pr_err("%s:", __func__);
	/*1. Unregister the platform driver */
	platform_driver_unregister(&struct_platform_driver);
	
	/*2. Class destroy */
	class_destroy(pcdrv_data.class_pcd);
	
	/*3. Unregister device numbers */
	unregister_chrdev_region(pcdrv_data.device_num_base, 1);
	
	pr_err("%s:Platform Driver removed", __func__);
}

void pcdev_release(struct device *dev){
	pr_err("%s:", __func__);
	pr_err("%s:Platform Driver released", __func__);
}

module_init(platform_driver_init);
module_exit(platform_driver_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module that registers platform Driver");
