#ifndef I2C_LCD_H
#define I2C_LCD_H

/* as per data sheet */
#define REAL_LCD_ADDR 0x78 
/* it seems to work only this way */
#define LCD_I2C_ADDR (REAL_LCD_ADDR >> 1)

void lcd_printf(const char * format, ...);

#endif