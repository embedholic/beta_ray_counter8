/*
 * i2c_clcd.h
 *
 *  Created on: May 7, 2024
 *      Author: isjeon
 */

#ifndef I2C_CLCD_H_
#define I2C_CLCD_H_

void i2c_lcd_init(void);
void i2c_lcd_command(uint8_t command);
void i2c_lcd_command_8(uint8_t command);
void i2c_lcd_data(uint8_t data);
void i2c_lcd_goto_XY(uint8_t row, uint8_t col);
void i2c_lcd_string(uint8_t row, uint8_t col, char string[]);

#endif /* I2C_CLCD_H_ */
