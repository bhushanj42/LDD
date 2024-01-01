#include "i2c_slave_drv.h"

struct ads1115drv_private_data ads_drv_data;

ssize_t readADC_show(struct device *dev, struct device_attribute *attr, char *buf){
	struct ads1115_device_data *dev_data = dev_get_drvdata(dev);
	uint16_t ADCVal;
	
	reinit_completion(&dev_data->conversionComplete);
	if(dev_data->IsContinuousConversion == ADS_SINGLE_SHOT){
		/*Trigger a dummy conversion*/
		triggerConversion(dev_data);	
	}
	/*Now because of this dummy conversion signal a conversion will start inside ADS1115, when this conversion completes,
	an IRQ will be raised, so here we will wait for the IRQ to be raised*/
	wait_for_completion(&dev_data->conversionComplete);
	
	/*Now that we are here it means a conversion was successful and now we can read the value in the conversion register*/
	ADCVal = readADCVal(dev_data);
	
	dev_err(dev_data->dev, "Read was successful %d", dev_data->IsContinuousConversion);
	
	return sprintf(buf, "%x\n", ADCVal);
}

ssize_t changeMode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
	/*0 - Continuous conversion mode
	1 - Single conversion */
	int ret;
	long value;
	
	ret = kstrtol(buf, 0, &value);
	if(ret){
		dev_err(dev, "Problem in conversion");
		return ret;
	}
	
	ret = ChangeConversionMode(dev, (uint8_t)value);
	if(ret){
		dev_err(dev, "Problem in setting conversion mode");
		return ret;
	}
	return count;
}

static DEVICE_ATTR_WO(changeMode);
static DEVICE_ATTR_RO(readADC);

static struct attribute *ads_attrs[] = {
	&dev_attr_changeMode.attr,
	&dev_attr_readADC.attr,
	NULL
};

static struct attribute_group ads_attr_group = {
	.attrs = ads_attrs
};

static const struct attribute_group *ads_attr_groups[] = {
	&ads_attr_group,
	NULL
};

static irqreturn_t conversion_ready_handler(int irq, void *data) {
	struct ads1115_device_data *dev_data = (struct ads1115_device_data *)data;

	complete(&dev_data->conversionComplete);
	return IRQ_HANDLED; 
}

int ADS1115_probe(struct i2c_client *client){
	struct device *dev = &client->dev;
	struct ads1115_device_data *dev_data;
	int ret;
	
	dev_err(dev, "Probe started, I2c Addr %x", client->addr);
	
	dev_data = devm_kzalloc(dev, sizeof(struct ads1115_device_data *), GFP_KERNEL);
	if(!dev_data){
		dev_err(dev, "Not able to allocate memory");
		return -ENOMEM;
	}
	
	mutex_init(&dev_data->bufMutex);
	dev_data->i2cClient = client;
	dev_set_drvdata(dev, dev_data);
	
	dev_data->IsContinuousConversion = ADS_SINGLE_SHOT;
	
	init_completion(&dev_data->conversionComplete);
	
	/* Get GPIO */
	dev_data->gpioDesc = devm_gpiod_get(dev, "conversion", GPIOD_ASIS);
	if(IS_ERR(dev_data->gpioDesc)){
		ret = PTR_ERR(dev_data->gpioDesc);
		if(ret == -ENOENT){
			dev_err(dev, "Problem in getting Converiosn GPIO");
		}
		else{
			dev_err(dev, "Some other GPIO error occurred %d", ret);
		}
		return ret;
	}
	ret = gpiod_direction_input(dev_data->gpioDesc);
	if(ret){
		dev_err(dev,"gpio direction set failed");
		return ret;
	}
	
	/*Make GPIO as interrupt source*/
	ret = gpiod_to_irq(dev_data->gpioDesc);
	if(ret < 0){
		dev_err(dev, "Problem in setting IRQ for GPIO");
		return ret;
	}
	else{
		dev_data->gpio_IRQNum = ret;
		dev_err(dev, "IRQ Number = %d", dev_data->gpio_IRQNum);
	}

	dev_err(dev, "Creating device");
	dev_data->dev = device_create_with_groups(ads_drv_data.class_ads1115, dev, 0, dev_data, ads_attr_groups, "ADS_DRIVER");
	if(IS_ERR(dev_data->dev)){
		ret = PTR_ERR(dev_data->dev);
		if(ret == -ENOENT){
			dev_err(dev, "ADS SLAVE driver creation failed");
		}
		return ret;
	}
	
	ret = InitADS1115(dev);
	if(ret){
		dev_err(dev, "There was a problem in init %d", ret);
		return ret;
	}
	
	dev_err(dev, "IRQ init");
	ret = devm_request_threaded_irq(dev, dev_data->gpio_IRQNum, NULL, conversion_ready_handler, IRQF_TRIGGER_HIGH | IRQF_ONESHOT, 
		"Conversion_IRQ_Callback", dev_data);
	if(ret){
		dev_err(dev, "Problem in setting IRQ callback");
		return ret;
	}
	
	dev_err(dev, "Probe complete");
	return 0;
}

int ADS1115_remove(struct i2c_client *client){
	struct device *dev = &client->dev;
	struct ads1115_device_data *dev_data = dev_get_drvdata(dev);
	
	dev_err(dev, "Remove started");
	
	device_unregister(dev_data->dev);
	
	dev_err(dev, "Remove complete");
	return 0;
}

static struct i2c_device_id i2cDeviceTable[] = {
	{"i2c-device_0", ID_I2C_DEVICE},
	{}
};
MODULE_DEVICE_TABLE(i2c, i2cDeviceTable);

static const struct of_device_id i2c_of_match[] = {
	{.compatible = "org,ADS1115_ADC"},
	{}
};
MODULE_DEVICE_TABLE(of, i2c_of_match);

static struct i2c_driver ADS1115_driver = {
	.probe_new = ADS1115_probe,
	.remove = ADS1115_remove,
	.id_table = i2cDeviceTable,
	.driver = {
		.name = "ADS1115_Slave_I2c_Driver",
		.of_match_table = of_match_ptr(i2c_of_match)
	}
};

int __init ads1115_init(void){
	int ret;
	
	pr_err("%s", __func__);
	
	ads_drv_data.class_ads1115 = class_create(THIS_MODULE, "ADS_Drv_Class");
	if(IS_ERR(ads_drv_data.class_ads1115)){
		pr_err("%s Error in creating class", __func__);
		return PTR_ERR(ads_drv_data.class_ads1115);
	}
	
	ret = i2c_add_driver(&ADS1115_driver);
	if(ret){
		pr_err("I2c driver not added!!!");
		return ret;
	}
	pr_err("%s: Successful!!!", __func__);
	return 0;
}

void __exit ads1115_exit(void){
	pr_err("%s", __func__);
	
	i2c_del_driver(&ADS1115_driver);
	
	class_destroy(ads_drv_data.class_ads1115);
	
	pr_err("%s: Successful!!!", __func__);
}

module_init(ads1115_init);
module_exit(ads1115_exit);

MODULE_AUTHOR("bhushanj42");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A ADS1115 ADC, I2C slave driver");

