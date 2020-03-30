#ifndef I2C_LCD_H
#define I2C_LCD_H

/* as per data sheet */
#define LCD_ADDR 0x78 
// lcd is 2X20
#define LCD_LINE_LEN 20
#define LCD_LINES 2

void lcd_printf(const char * format, ...);

#endif