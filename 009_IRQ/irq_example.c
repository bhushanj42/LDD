#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/device.h>
#include <linux/platform_device.h>

/* This example is taken from "1. Creating an input device driver" from website
https://docs.kernel.org/input/input-programming.html
*/

int gpio_irq_probe(struct platform_device *pdev);
int gpio_irq_remove(struct platform_device *pdev);

struct of_device_id gpio_device_match[] = 
{
	{.compatible = "org,lcd16x2"},
	{}
};

struct platform_driver gpioirq_plat_drv = 
{
	.probe = gpio_irq_probe,
	.remove = gpio_irq_remove,
	.driver = {
		.name = "gpio-irq",
		.of_match_table = of_match_ptr(gpio_device_match)
	}
};

struct gpiodev_prv_data{
	char label[20];
	struct gpio_desc *desc;
	int irq_number;
};

struct gpiodrv_prv_data{
	struct class *class_gpio;
	struct device *device_gpio;
};

struct gpiodrv_prv_data gpioIrq_data;

static int __init hello_init(void){
	pr_err("%s", __func__);
	
	gpioIrq_data.class_gpio = class_create(THIS_MODULE, "gpio_Irq_class");
	if(IS_ERR(gpioIrq_data.class_gpio)){
		pr_err("%s Error in creating class", __func__);
		return PTR_ERR(gpioIrq_data.class_gpio);
	}
	
	platform_driver_register(&gpioirq_plat_drv);
	
	pr_err("%s successful", __func__);
	return 0;
}

static void __exit hello_exit(void){
	pr_err("%s", __func__);
	platform_driver_unregister(&gpioirq_plat_drv);
	
	class_destroy(gpioIrq_data.class_gpio);
	pr_err("%s successful", __func__);
}

static irqreturn_t gpio_irq_handler(int irq, void *data) {
	struct device *dev = (struct device *)data;
	dev_err(dev, "Interrupt triggered - %d", irq);
	return IRQ_HANDLED; 
}

int gpio_irq_probe(struct platform_device *pdev){
	struct device *dev = &pdev->dev;
	struct gpiodev_prv_data *dev_data;
	int ret;
	
	dev_err(dev, "Probe started");
	
	dev_data = devm_kzalloc(dev, sizeof(struct gpiodev_prv_data *), GFP_KERNEL);
	if(!dev_data){
		dev_err(dev, "Can't allocate memory");
		return -ENOMEM;
	}
	dev_data->label[0] = 'S';
	dev_data->label[1] = 'A';
	dev_data->label[2] = 'M';
	dev_data->label[3] = 'P';
	dev_data->label[4] = 'L';
	dev_data->label[5] = 'E';
	dev_data->label[6] = ' ';
	dev_data->label[7] = 'I';
	dev_data->label[8] = 'R';
	dev_data->label[9] = 'Q';
	dev_data->label[10] = 0;
	
	dev_set_drvdata(dev, dev_data);
	
	dev_err(dev, "getting gpiod");
	dev_data->desc = devm_gpiod_get(dev, "rs", GPIOD_ASIS);
	if(IS_ERR(dev_data->desc)){
		ret = PTR_ERR(dev_data->desc);
		if(ret == -ENOENT){
			dev_err(dev, "RS not found");
		}
		return ret;
	}
	ret = gpiod_direction_input(dev_data->desc);
	if(ret){
		dev_err(dev,"gpio direction set failed");
		return ret;
	}
	
	dev_data->irq_number = gpiod_to_irq(dev_data->desc);
	dev_err(dev, "IRQ Number = %d", dev_data->irq_number);
	
	ret = request_irq(dev_data->irq_number, gpio_irq_handler, IRQF_TRIGGER_LOW, "my_gpio_irq", dev);
	if(ret){
		dev_err(dev, "Error!Can not request interrupt nr.: %d", dev_data->irq_number);
		return ret;
	}
	
	gpioIrq_data.device_gpio = device_create(gpioIrq_data.class_gpio, dev, 0, "NULL", dev_data->label);
	if(IS_ERR(gpioIrq_data.device_gpio)){
		dev_err(dev, "Device creation failed");
		ret = PTR_ERR(gpioIrq_data.device_gpio);
		return ret;
	}

	dev_err(dev, "Probe Completed");
	
	return 0;
}

int gpio_irq_remove(struct platform_device *pdev){
	struct device *dev = &pdev->dev;
	
	dev_err(dev, "Remove started");
	device_unregister(gpioIrq_data.device_gpio);
	dev_err(dev, "Remove Completed");
	
	return 0;
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("bhushanj42");
MODULE_DESCRIPTION("A simple gpio interrupt");
