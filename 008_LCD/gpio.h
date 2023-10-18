#ifndef GPIO_H
#define GPIO_H

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
#include <linux/delay.h>

#define NUM_OF_GPIOS		7

#define LOW_VALUE		0
#define HIGH_VALUE		1

/*LCD commands */
#define LCD_CMD_4DL_2N_5X8F  		0x28
#define LCD_CMD_DON_CURON    		0x0E
#define LCD_CMD_INCADD       		0x06
#define LCD_CMD_DIS_CLEAR    		0X01
#define LCD_CMD_DIS_RETURN_HOME  	0x02

/* Sets DDRAM address. DDRAM data is sent and received after this setting. */
#define LCD_CMD_SET_DDRAM_ADDRESS  			0x80
#define DDRAM_SECOND_LINE_BASE_ADDR         	(LCD_CMD_SET_DDRAM_ADDRESS | 0x40 )
#define DDRAM_FIRST_LINE_BASE_ADDR          	LCD_CMD_SET_DDRAM_ADDRESS

enum GPIO_Num{
	LCD_RS,
	LCD_RW,
	LCD_EN,
	LCD_D4,
	LCD_D5,
	LCD_D6,
	LCD_D7
};

/* Device private data */
struct lcddev_private_data{
	struct gpio_desc *desc[NUM_OF_GPIOS];
	uint8_t row_num;
	uint8_t column_num;
};

void initGPIO(struct device *dev);
void initLCD(struct device *dev);
void writeCommand(uint8_t command, struct device *dev);
void write_4_command(uint8_t data, struct device *dev);
void enableLCD(struct device *dev);
void gpio_write_value(uint8_t pin_num, uint8_t pin_val, struct device *dev);
void lcd_display_clear(struct device *dev);
void lcd_display_return_home(struct device *dev);
void lcd_set_cursor(uint8_t row, uint8_t column, struct device *dev);
void lcd_print_char(uint8_t data, struct device *dev);

#endif

