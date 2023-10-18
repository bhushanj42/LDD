#include "gpio.h"

int lcd_sysfs_probe(struct platform_device *pdev);
int lcd_sysfs_remove(struct platform_device *pdev);

ssize_t lcdxy_show(struct device *dev, struct device_attribute *attr, char *buf){
	struct lcddev_private_data *dev_data = dev_get_drvdata(dev);
	char c[6];
	
	dev_err(dev, "%s", __func__);
	c[0] = '(';
	c[1] = dev_data->row_num + 0x30;
	c[2] = ',';
	c[3] = dev_data->column_num + 0x30;
	c[4] = ')';
	c[5] = 0;	/*Null terminator*/
	
	return sprintf(buf, "%s\n", c);
}

ssize_t lcdxy_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
	struct lcddev_private_data *dev_data = dev_get_drvdata(dev);
	
	dev_err(dev, "XY - %s", buf);
	/*Assuming that the buf will have a tring in the form (r,cc)*/
	/*r - row numer - 0 or 1
	cc - column number - 0 to 15*/
	dev_data->row_num = buf[1] - 0x30;
	dev_data->column_num = ((buf[3] - 0x30) * 10) + buf[4] - 0x30;
	dev_err(dev, "row = %d, column = %d", dev_data->row_num, dev_data->column_num);
	
	lcd_set_cursor(dev_data->row_num, dev_data->column_num, dev);
	return count;
}

ssize_t lcdcmd_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
	/*struct lcddev_private_data *dev_data = dev_get_drvdata(dev);*/
	int ret;
	long value;
	
	dev_err(dev, "%s-%s", __func__, buf);
	ret = kstrtol(buf, 0, &value);
	if(ret){
		dev_err(dev, "Problem in conversion");
		return ret;
	}
	writeCommand((uint8_t)value, dev);
	return count;
}

ssize_t lcdscroll_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
	/*Display shift command is as follows:
	D7 D6 D5 D4 D3  D2  D1 D0
	0  0  0  1  S/C R/L 0  0
	0x1C - Display shift right
	0x14 - cursor shift right
	0x18 - Display shift left
	0x10 - Cursor shift left*/
	int ret;
	long value;
	
	dev_err(dev, "%s-%s", __func__, buf);
	ret = kstrtol(buf, 0, &value);
	if(ret){
		dev_err(dev, "Problem in conversion");
		return ret;
	}
	writeCommand((uint8_t)value, dev);
	return count;
}

ssize_t lcdtext_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
	int i;
	
	dev_err(dev, "%s: count-%lu", __func__, count);
	for(i = 0; i < count; i++){
		dev_err(dev, "->%c", *buf);
		lcd_print_char(*buf, dev);
		buf++;
	}
	return count;
}

static DEVICE_ATTR_WO(lcdcmd);
static DEVICE_ATTR_WO(lcdscroll);
static DEVICE_ATTR_WO(lcdtext);
static DEVICE_ATTR_RW(lcdxy);

static struct attribute *gpio_attrs[] = {
	&dev_attr_lcdcmd.attr,
	&dev_attr_lcdscroll.attr,
	&dev_attr_lcdtext.attr,
	&dev_attr_lcdxy.attr,
	NULL
};

static struct attribute_group gpio_attr_group = {
	.attrs = gpio_attrs
};

static const struct attribute_group *gpio_attr_groups[] = {
	&gpio_attr_group,
	NULL
};

/* Driver private data structure */
struct lcddrv_private_data
{
	struct class *class_lcd;
	struct device *dev;
};

struct lcddrv_private_data lcd_drv_data;

struct of_device_id lcd_device_match[] = 
{
	{.compatible = "org,lcd16x2"},
	{}
};

struct platform_driver lcdsysfs_plat_drv = 
{
	.probe = lcd_sysfs_probe,
	.remove = lcd_sysfs_remove,
	.driver = {
		.name = "lcd-gpio-sysfs",
		.of_match_table = of_match_ptr(lcd_device_match)
	}
};

int lcd_sysfs_probe(struct platform_device *pdev){
	struct device *dev = &pdev->dev;
	struct lcddev_private_data *dev_data;
	int ret;

	dev_err(dev, "Probe started");
	
	dev_data = devm_kzalloc(dev, sizeof(struct lcddev_private_data *), GFP_KERNEL);
	if(!dev_data){
		dev_err(dev, "Can't allocate memory");
		return -ENOMEM;
	}
	dev_data->row_num = 0;
	dev_data->column_num = 0;
	
	dev_err(dev, "getting gpiod");
	
	dev_data->desc[LCD_RS] = gpiod_get(dev, "rs", GPIOD_ASIS);
	if(IS_ERR(dev_data->desc[LCD_RS])){
		ret = PTR_ERR(dev_data->desc[LCD_RS]);
		if(ret == -ENOENT){
			dev_err(dev, "RS not found");
		}
		return ret;
	}
	dev_data->desc[LCD_RW] = gpiod_get(dev, "rw", GPIOD_ASIS);
	if(IS_ERR(dev_data->desc[LCD_RW])){
		ret = PTR_ERR(dev_data->desc[LCD_RW]);
		if(ret == -ENOENT){
			dev_err(dev, "RW not found");
		}
		return ret;
	}
	dev_data->desc[LCD_EN] = gpiod_get(dev, "en", GPIOD_ASIS);
	if(IS_ERR(dev_data->desc[LCD_EN])){
		ret = PTR_ERR(dev_data->desc[LCD_EN]);
		if(ret == -ENOENT){
			dev_err(dev, "EN not found");
		}
		return ret;
	}
	dev_data->desc[LCD_D4] = gpiod_get(dev, "d4", GPIOD_ASIS);
	if(IS_ERR(dev_data->desc[LCD_D4])){
		ret = PTR_ERR(dev_data->desc[LCD_D4]);
		if(ret == -ENOENT){
			dev_err(dev, "D4 not found");
		}
		return ret;
	}
	dev_data->desc[LCD_D5] = gpiod_get(dev, "d5", GPIOD_ASIS);
	if(IS_ERR(dev_data->desc[LCD_D5])){
		ret = PTR_ERR(dev_data->desc[LCD_D5]);
		if(ret == -ENOENT){
			dev_err(dev, "D5 not found");
		}
		return ret;
	}
	dev_data->desc[LCD_D6] = gpiod_get(dev, "d6", GPIOD_ASIS);
	if(IS_ERR(dev_data->desc[LCD_D6])){
		ret = PTR_ERR(dev_data->desc[LCD_D6]);
		if(ret == -ENOENT){
			dev_err(dev, "D6 not found");
		}
		return ret;
	}
	dev_data->desc[LCD_D7] = gpiod_get(dev, "d7", GPIOD_ASIS);
	if(IS_ERR(dev_data->desc[LCD_D7])){
		ret = PTR_ERR(dev_data->desc[LCD_D7]);
		if(ret == -ENOENT){
			dev_err(dev, "D7 not found");
		}
		return ret;
	}
	
	lcd_drv_data.dev = device_create_with_groups(lcd_drv_data.class_lcd, dev, 0, dev_data, gpio_attr_groups, "LCD16x2");
	if(IS_ERR(lcd_drv_data.dev)){
		ret = PTR_ERR(lcd_drv_data.dev);
		if(ret == -ENOENT){
			dev_err(dev, "No GPIO has been assigned to the requested function and/or Index");
		}
		return ret;
	}
	
	dev_err(dev, "Initializing LCD");
	initGPIO(dev);
	initLCD(dev);
	lcd_display_clear(dev);
	lcd_display_return_home(dev);
	lcd_set_cursor(dev_data->row_num, dev_data->column_num, dev);
	
	lcd_print_char('1', dev);
	lcd_print_char('6', dev);
	lcd_print_char('X', dev);
	lcd_print_char('2', dev);
	lcd_print_char(' ', dev);
	lcd_print_char('L', dev);
	lcd_print_char('C', dev);
	lcd_print_char('D', dev);
	lcd_print_char(' ', dev);
	lcd_print_char('D', dev);
	lcd_print_char('R', dev);
	lcd_print_char('I', dev);
	lcd_print_char('V', dev);
	lcd_print_char('E', dev);
	lcd_print_char('R', dev);
	
	dev_err(dev, "Probe Successful!!!");
	
	return 0;
}

int lcd_sysfs_remove(struct platform_device *pdev){
	struct device *dev = &pdev->dev;
	
	dev_err(dev, "Remove started");
	
	lcd_display_clear(dev);
	
	device_unregister(lcd_drv_data.dev);
		
	dev_err(dev, "Remove Successful!!!");
	return 0;
}

int __init lcd_sysfs_init(void){
	pr_err("%s", __func__);
	lcd_drv_data.class_lcd = class_create(THIS_MODULE, "lcd");
	if(IS_ERR(lcd_drv_data.class_lcd)){
		pr_err("%s Error in creating class", __func__);
		return PTR_ERR(lcd_drv_data.class_lcd);
	}
	
	platform_driver_register(&lcdsysfs_plat_drv);

	pr_err("%s: Successful!!!", __func__);
	return 0;
}

void __exit lcd_sysfs_exit(void){
	pr_err("%s", __func__);
	platform_driver_unregister(&lcdsysfs_plat_drv);
	
	class_destroy(lcd_drv_data.class_lcd);
	pr_err("%s: Successful!!!", __func__);
}

module_init(lcd_sysfs_init);
module_exit(lcd_sysfs_exit);

MODULE_AUTHOR("bhushanj42");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A lcd sysfs driver");
