#include "gpio.h"

void initGPIO(struct device *dev){
	struct lcddev_private_data *dev_data = dev_get_drvdata(dev);
	int value;
	value = 0;
	
	dev_err(dev, "initGPIO started");
	
	gpiod_set_value(dev_data->desc[LCD_RS], LOW_VALUE);
	gpiod_set_value(dev_data->desc[LCD_RW], LOW_VALUE);
	gpiod_set_value(dev_data->desc[LCD_EN], LOW_VALUE);
	gpiod_set_value(dev_data->desc[LCD_D4], LOW_VALUE);
	gpiod_set_value(dev_data->desc[LCD_D5], LOW_VALUE);
	gpiod_set_value(dev_data->desc[LCD_D6], LOW_VALUE);
	gpiod_set_value(dev_data->desc[LCD_D7], LOW_VALUE);
	
	dev_err(dev, "initGPIO finished");
}

void enableLCD(struct device *dev){
	gpio_write_value(LCD_EN, LOW_VALUE, dev);
	udelay(10);
	gpio_write_value(LCD_EN, HIGH_VALUE, dev);
	udelay(10);
	gpio_write_value(LCD_EN, LOW_VALUE, dev);
	udelay(100);
	
/*	dev_err(dev, "enableLCD");*/
}

void writeCommand(uint8_t command, struct device *dev){
	/* RS=0 for LCD command */
	gpio_write_value(LCD_RS, LOW_VALUE, dev);
	
	/*R/nW = 0, for write */
	gpio_write_value(LCD_RW, LOW_VALUE, dev);
	
	write_4_command(command >> 4, dev); /* higher nibble */
	write_4_command(command, dev);     /* lower nibble */
}

void write_4_command(uint8_t data, struct device *dev){
	gpio_write_value(LCD_D4, (data >> 0 ) & 0x1, dev);
	gpio_write_value(LCD_D5, (data >> 1 ) & 0x1, dev);
	gpio_write_value(LCD_D6, (data >> 2 ) & 0x1, dev);
	gpio_write_value(LCD_D7, (data >> 3 ) & 0x1, dev);
	
	enableLCD(dev);
}

void gpio_write_value(uint8_t pin_num, uint8_t pin_val, struct device *dev){
	struct lcddev_private_data *dev_data = dev_get_drvdata(dev);
	
	gpiod_set_value(dev_data->desc[pin_num], pin_val);
}

void initLCD(struct device *dev){
	mdelay(40);
	
	/* RS=0 for LCD command */
	gpio_write_value(LCD_RS, LOW_VALUE, dev);

	/*R/nW = 0, for write */
	gpio_write_value(LCD_RW, LOW_VALUE, dev);
	
	write_4_command(0x03, dev);
	mdelay(5);
	
	write_4_command(0x03, dev);
	udelay(100);
	
	write_4_command(0x03, dev);
	write_4_command(0x02, dev);

    /*4 bit data mode, 2 lines selection , font size 5x8 */
	writeCommand(LCD_CMD_4DL_2N_5X8F, dev);
	
	/* Display ON, Cursor ON */
	writeCommand(LCD_CMD_DON_CURON, dev);
	
	lcd_display_clear(dev);
	
	/*Address auto increment*/
	writeCommand(LCD_CMD_INCADD, dev);
	
	dev_err(dev, "initLCD");
}

/*Clear the display */
void lcd_display_clear(struct device *dev)
{
	writeCommand(LCD_CMD_DIS_CLEAR, dev);
	/*
	 * check page number 24 of datasheet.
	 * display clear command execution wait time is around 2ms
	 */
	mdelay(2); 
}

/*Cursor returns to home position */
void lcd_display_return_home(struct device *dev)
{
	writeCommand(LCD_CMD_DIS_RETURN_HOME, dev);
	/*
	 * check page number 24 of datasheet.
	 * return home command execution wait time is around 2ms
	 */
	mdelay(2);
}

void lcd_set_cursor(uint8_t row, uint8_t column, struct device *dev)
{
	switch (row)
	{
		case 0:
			/* Set cursor to 1st row address and add index*/
			writeCommand(column |= DDRAM_FIRST_LINE_BASE_ADDR, dev);
		break;
		
		case 1:
			/* Set cursor to 2nd row address and add index*/
			writeCommand(column |= DDRAM_SECOND_LINE_BASE_ADDR, dev);
		break;
		
		default:
			dev_err(dev, "lcd_set_cursor"); 
		break;
	}
}

void lcd_print_char(uint8_t data, struct device *dev)
{

	//RS=1, for user data
	gpio_write_value(LCD_RS, HIGH_VALUE, dev);
	
	/*R/nW = 0, for write */
	gpio_write_value(LCD_RW, LOW_VALUE, dev);
	
	write_4_command(data >> 4, dev); /* higher nibble */
	write_4_command(data, dev);      /* lower nibble */
}

