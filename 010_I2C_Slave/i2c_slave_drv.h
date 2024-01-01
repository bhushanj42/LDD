#ifndef I2C_SLAVE_DRV_H
#define I2C_SLAVE_DRV_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/completion.h>

#define ID_I2C_DEVICE			0

#define ADS1115_ADDR 			0b1001000
#define ADS_ADDR_CONVERSION_REG	0
#define ADS_ADDR_CONFIG_REG 		1
#define ADS_ADDR_LO_THRESH		2
#define ADS_ADDR_HI_THRESH		3
#define ADS_OS 			1  /*Start a single conversion*/
#define ADS_MUX 			4 /*AINP = AIN0 & AINN = GND*/
//#define ADS_PGA 			0 /*FSR = +/-6.144 V*/
#define ADS_PGA 			1 /*FSR = +/-4.096 V*/
#define ADS_SINGLE_SHOT		1
#define ADS_CONTINUOUS_CONVERSION	0
#define ADS_MODE 			ADS_SINGLE_SHOT /*Single shot mode*/
#define ADS_DR 			0   /*8 Samples per sec*/
#define ADS_COMP_MODE 			0
#define ADS_COMP_POL 			0	/*1 - Active high when conversion done in single shot mode*/
#define ADS_COMP_LAT 			0
/*0 - Assert after 1 conversion
1 - Assert after 2 conversion
2 - Assert after 4 conversion
3 - Disable comparator and set ALERT/RDY pin to high-impedance (default)*/
#define ADS_COMP_QUEUE 		2

#define CONVERSION_READY		1
#define CONVERSION_NOT_READY		0

/* Driver private data structure */
struct ads1115drv_private_data{
	struct class *class_ads1115;
};

struct ads1115_device_data{
	struct i2c_client *i2cClient;
	struct device *dev;
	struct gpio_desc *gpioDesc;
	unsigned int gpio_IRQNum;
	struct mutex bufMutex;
	uint8_t IsContinuousConversion;
	struct completion conversionComplete;
};

int InitADS1115(struct device *dev);
int ChangeConversionMode(struct device *dev, uint8_t DevOpMode);
int triggerConversion(struct ads1115_device_data *dev_data);
uint16_t readADCVal(struct ads1115_device_data *dev_data);

#endif
