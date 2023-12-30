#include "i2c_slave_drv.h"

int InitADS1115(struct device *dev){
	struct ads1115_device_data *dev_data = dev_get_drvdata(dev);
	uint8_t buffer[3];
	int ret;

	/*Set MSB of LO_THRESH to 0 to enable Conversion ready pin*/
	dev_err(dev, "LO_THRESH");
	buffer[0] = ADS_ADDR_LO_THRESH;
	buffer[1] = 0;
	buffer[2] = 0;
	ret = i2c_master_send(dev_data->i2cClient, &buffer[0], 3);
	if(ret < 3){
		dev_err(dev, "Problem in sending");
		return ret;
	}
	
	/*Set MSB of HI_THRESH to 1 to enable Conversion ready pin*/
	dev_err(dev, "HI_THRESH");
	buffer[0] = ADS_ADDR_HI_THRESH;
	buffer[1] = 0x80;
	buffer[2] = 0;
	ret = i2c_master_send(dev_data->i2cClient, &buffer[0], 3);
	if(ret < 3){
		dev_err(dev, "Problem in sending");
		return ret;
	}
	
	/*Set config register address*/
	buffer[0] = ADS_ADDR_CONFIG_REG;
	/*Set config register value MSB*/
	buffer[1] = (ADS_OS << 7) | (ADS_MUX << 4) | (ADS_PGA << 1) | ADS_MODE;
	/*Set config register value LSB*/
	buffer[2] = (ADS_DR << 5) | (ADS_COMP_MODE << 4) | (ADS_COMP_POL << 3) | (ADS_COMP_LAT << 2) | ADS_COMP_QUEUE;
	
	dev_err(dev, "Configuring ADS");
	ret = i2c_master_send(dev_data->i2cClient, &buffer[0], 3);
	if(ret < 3){
		dev_err(dev, "Problem in sending");
		return ret;
	}
	
	/*Set ADS_CONVERSION_REG_ADDR address*/
	buffer[0] = ADS_ADDR_CONVERSION_REG;
	dev_err(dev, "Set conversion reg Address");
	ret = i2c_master_send(dev_data->i2cClient, &buffer[0], 1);
	if(ret < 1){
		dev_err(dev, "Problem in sending");
		return ret;
	}
	
	dev_err(dev, "Read ADC val");
	mdelay(10);
	/*Read conversion result*/
	ret = i2c_master_recv(dev_data->i2cClient, &buffer[0], 2);
	if(ret < 2){
		dev_err(dev, "Problem in receiving");
		return ret;
	}
	dev_err(dev, "Data = %x %x", buffer[0], buffer[1]);
	return 0;	
}

int ChangeConversionMode(struct device *dev, uint8_t DevOpMode){
	struct ads1115_device_data *dev_data = dev_get_drvdata(dev);
	uint8_t buffer[3];
	int ret;
	
	dev_data->IsContinuousConversion = DevOpMode & 0x01;
	dev_err(dev, "Setting Conversion mode = %d", dev_data->IsContinuousConversion);
	
	/*Set config register address*/
	buffer[0] = ADS_ADDR_CONFIG_REG;
	/*Set config register value MSB*/
	buffer[1] = (ADS_OS << 7) | (ADS_MUX << 4) | (ADS_PGA << 1) | dev_data->IsContinuousConversion;
	/*Set config register value LSB*/
	buffer[2] = (ADS_DR << 5) | (ADS_COMP_MODE << 4) | (ADS_COMP_POL << 3) | (ADS_COMP_LAT << 2) | ADS_COMP_QUEUE;

	ret = i2c_master_send(dev_data->i2cClient, &buffer[0], 3);
	if(ret < 3){
		dev_err(dev, "Problem in sending");
		return ret;
	}
	
	/*Set thte Address register in ADS to point to conversion register*/
	buffer[0] = ADS_ADDR_CONVERSION_REG;
	ret = i2c_master_send(dev_data->i2cClient, &buffer[0], 1);
	if(ret < 1){
		dev_err(dev, "Problem in sending");
		return ret;
	}
	return 0;
}

int triggerConversion(struct ads1115_device_data *dev_data){
	int ret;
	uint8_t buffer[3];
	
	/*Set config register address*/
	buffer[0] = ADS_ADDR_CONFIG_REG;
	/*Set config register value MSB*/
	buffer[1] = (ADS_OS << 7) | (ADS_MUX << 4) | (ADS_PGA << 1) | ADS_SINGLE_SHOT;
	/*Set config register value LSB*/
	buffer[2] = (ADS_DR << 5) | (ADS_COMP_MODE << 4) | (ADS_COMP_POL << 3) | (ADS_COMP_LAT << 2) | ADS_COMP_QUEUE;
	
	ret = i2c_master_send(dev_data->i2cClient, &buffer[0], 3);
	if(ret < 3){
		dev_err(dev_data->dev, "Problem in sending");
		return ret;
	}
	
	/*Set thte Address register in ADS to point to conversion register*/
	buffer[0] = ADS_ADDR_CONVERSION_REG;
	ret = i2c_master_send(dev_data->i2cClient, &buffer[0], 1);
	if(ret < 1){
		dev_err(dev_data->dev, "Problem in sending");
		return ret;
	}
	return 0;
}

uint16_t readADCVal(struct ads1115_device_data *dev_data){
	uint8_t buffer[2];
	int ret;
	
	ret = i2c_master_recv(dev_data->i2cClient, &buffer[0], 2);
	if(ret < 2){
		dev_err(dev_data->dev, "Problem in receiving");
		return ret;
	}
	dev_err(dev_data->dev, "Data = %x %x", buffer[0], buffer[1]);
	
	return ((buffer[0] << 8) | (buffer[1]));
}

