#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/gpio/consumer.h>

int gpio_sysfs_probe(struct platform_device *pdev);
int gpio_sysfs_remove(struct platform_device *pdev);

/* Device private data structure */
struct gpiodev_private_data
{
	char label[20];
	struct gpio_desc *desc;
};

/* Driver private data structure */
struct gpiodrv_private_data
{
	int total_devices;
	struct class *class_gpio;
	struct device **dev;
};

struct gpiodrv_private_data gpio_drv_data;

ssize_t direction_show(struct device *dev, struct device_attribute *attr, char *buf){
	struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);
	int dir;
	char *direction;
	
	dev_err(dev, "%s", __func__);
	dir = gpiod_get_direction(dev_data->desc);
	if(dir < 0){
		return dir;
	}
	/* If dir = 0 then show out, if dir = 1 then show in*/
	direction = (dir == 0) ? "GPIOF_DIR_OUT" : "GPIOF_DIR_IN";
	return sprintf(buf, "%s\n", direction);
}

ssize_t direction_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
	int ret;
	struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);
	
	dev_err(dev, "%s", __func__);
	if(sysfs_streq(buf, "in")){
		ret = gpiod_direction_input(dev_data->desc);
	}
	else if(sysfs_streq(buf, "out")){
		ret = gpiod_direction_output(dev_data->desc, 0);
	}
	else{
		ret = -EINVAL;
	}
	return ret ? : count;
}

ssize_t value_show(struct device *dev, struct device_attribute *attr, char *buf){
	struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);
	int value;
	
	dev_err(dev, "%s", __func__);
	value = gpiod_get_value(dev_data->desc);
	return sprintf(buf, "%d\n", value);
}

ssize_t value_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
	struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);
	int ret;
	long value;
	
	dev_err(dev, "%s", __func__);
	ret = kstrtol(buf, 0, &value);
	if(ret){
		dev_err(dev, "Problem in conversion");
		return ret;
	}
	gpiod_set_value(dev_data->desc, value);
	
	return count;
}

ssize_t label_show(struct device *dev, struct device_attribute *attr, char *buf){
	struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);
	dev_err(dev, "%s", __func__);
	return sprintf(buf, "%s\n", dev_data->label);
}

static DEVICE_ATTR_RW(direction);
static DEVICE_ATTR_RW(value);
static DEVICE_ATTR_RO(label);

static struct attribute *gpio_attrs[] = {
	&dev_attr_direction.attr,
	&dev_attr_value.attr,
	&dev_attr_label.attr,
	NULL
};

static struct attribute_group gpio_attr_group = {
	.attrs = gpio_attrs
};

static const struct attribute_group *gpio_attr_groups[] = {
	&gpio_attr_group,
	NULL
};

struct of_device_id gpio_device_match[] = 
{
	{.compatible = "org,bone-gpio-sysfs"},
	{}
};

struct platform_driver gpiosysfs_plat_drv = 
{
	.probe = gpio_sysfs_probe,
	.remove = gpio_sysfs_remove,
	.driver = {
		.name = "bone-gpio-sysfs",
		.of_match_table = of_match_ptr(gpio_device_match)
	}
};

int gpio_sysfs_probe(struct platform_device *pdev){
	struct device *dev = &pdev->dev;
	struct device_node *parent = dev->of_node;
	struct device_node *child;
	struct gpiodev_private_data *dev_data;
	const char *name;
	int i = 0, ret;

	dev_err(dev, "Probe started");
	
	gpio_drv_data.total_devices = of_get_child_count(parent);
	if(!gpio_drv_data.total_devices){
		dev_err(dev, "No devices found");
		return -EINVAL;
	}
	
	dev_err(dev, "Total device count = %d", gpio_drv_data.total_devices);
	gpio_drv_data.dev = devm_kzalloc(dev, sizeof(struct device *) * gpio_drv_data.total_devices, GFP_KERNEL);

	for_each_available_child_of_node(parent, child){
		dev_data = devm_kzalloc(dev, sizeof(*dev_data), GFP_KERNEL);
		if(!dev_data){
			dev_err(dev, "Can't allocate memory");
			return -ENOMEM;
		}
		
		if(of_property_read_string(child, "label", &name)){
			dev_err(dev, "Missing label information");
			snprintf(dev_data->label, sizeof(dev_data->label), "Unknown gpio%d", i);
		}
		else{
			strcpy(dev_data->label, name);
			dev_err(dev, "GPIO label = %s", dev_data->label);
		}
		
		dev_data->desc = devm_fwnode_get_gpiod_from_child(dev, "bone", &child->fwnode, GPIOD_ASIS, dev_data->label);
		
		if(IS_ERR(dev_data->desc)){
			ret = PTR_ERR(dev_data->desc);
			if(ret == -ENOENT){
				dev_err(dev, "No GPIO has been assigned to the requested function and/or Index");
			}
			return ret;
		}
		
		/* set the gpio direction to output*/
		ret = gpiod_direction_output(dev_data->desc, 0);
		if(ret){
			dev_err(dev, "Problem in setting the direction");
			return ret;
		}
		
		/* Create devices under /sys/class/bone_gpios */
		gpio_drv_data.dev[i] = device_create_with_groups(gpio_drv_data.class_gpio, dev, 0, dev_data, gpio_attr_groups, dev_data->label);
		if(IS_ERR(gpio_drv_data.dev[i])){
			dev_err(dev, "Error in device creation");
			return PTR_ERR(gpio_drv_data.dev[i]);
		}
		
		i++;
	}
	dev_err(dev, "Probe Successful!!!");
	
	return 0;
}

int gpio_sysfs_remove(struct platform_device *pdev){
	struct device *dev = &pdev->dev;
	int i;
	
	dev_err(dev, "Remove started");
	
	for(i = 0; i < gpio_drv_data.total_devices; i++){
		device_unregister(gpio_drv_data.dev[i]);
	}
	dev_err(dev, "Remove Successful!!!");
	return 0;
}

int __init gpio_sysfs_init(void){
	pr_err("%s", __func__);
	gpio_drv_data.class_gpio = class_create(THIS_MODULE, "bone_gpios");
	if(IS_ERR(gpio_drv_data.class_gpio)){
		pr_err("%s Error in creating class", __func__);
		return PTR_ERR(gpio_drv_data.class_gpio);
	}
	
	platform_driver_register(&gpiosysfs_plat_drv);

	pr_err("%s: Successful!!!", __func__);
	return 0;
}

void __exit gpio_sysfs_exit(void){
	pr_err("%s", __func__);
	platform_driver_unregister(&gpiosysfs_plat_drv);
	
	class_destroy(gpio_drv_data.class_gpio);
	pr_err("%s: Successful!!!", __func__);
}

module_init(gpio_sysfs_init);
module_exit(gpio_sysfs_exit);

MODULE_AUTHOR("bhushanj42");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A gpio sysfs driver");
