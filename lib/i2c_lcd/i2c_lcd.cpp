#include <Arduino.h>
#include <Wire.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "i2c.h"
#include "i2c_lcd.h"
#include "message.h"

#define COMSEND 0x00
#define DATASEND 0x40

// lcd is 2X20
#define LCD_LINE_LEN 20
#define LCD_LINES 2
#define MAX_BUFFER ((LCD_LINE_LEN * LCD_LINES) + 2)

uint8_t previous_buffer[LCD_LINES][LCD_LINE_LEN];

static void init_LCD()
{
  _FN_START

  uint8_t packet_1[] = { 0x38, 0x14, 0x70, 0x5E, 0x6D, 0x0C, 0x01, 0x06 };
  i2c_write(LCD_I2C_ADDR, COMSEND, packet_1, 8);
  delay(10);
  
  uint8_t packet_2[] = { 0x38, 0x40 };
  i2c_write(LCD_I2C_ADDR, COMSEND, packet_2, 2);
  delay(10);

  uint8_t packet_3[] = { 0x00, 0x1E, 0x18, 0x14, 0x12, 0x01, 0x00, 0x00 };
  i2c_write(LCD_I2C_ADDR, COMSEND, packet_3, 8);
  delay(10);

  uint8_t packet_4[] = { 0x39, 0x01 };
  i2c_write(LCD_I2C_ADDR, COMSEND, packet_4, 2);
  delay(10);
  
  _FN_END
}

void lcd_printf(const char * format, ...)
{
  _FN_START

  char buffer[MAX_BUFFER];
  va_list args;
  va_start (args, format);
  vsnprintf (buffer,MAX_BUFFER, format, args);
  va_end (args);

	/* first we look to see if a different line is sent */
	bool is_diff = false;
  int i = 0;
  for(int row = 0; row < LCD_LINES; row += 1)
  {
    bool EOL = false;
    for(int col = 0; col < LCD_LINE_LEN; col += 1)
    {
      char c = ' ';
      if ( ! EOL )
      {
        if (buffer[i] != '\0')
        {
          if (buffer[i] == '\n')
          {
            EOL = true;
          }
          else
          {
            c = buffer[i];
          }
          i += 1;
        }

        if(previous_buffer[row][col] != (uint8_t)c)
        {
          is_diff = true;
        }
      }
      previous_buffer[row][col] = (uint8_t)c;
    }
	}

	// this could go away if we change only one char
	if(is_diff)
	{
		init_LCD(); 
    i2c_write(LCD_I2C_ADDR, DATASEND, previous_buffer[0], 20);
    /* new line */
    uint8_t packet[] = { 0xC0 };
    i2c_write(LCD_I2C_ADDR, COMSEND, packet, 1);
    i2c_write(LCD_I2C_ADDR, DATASEND, previous_buffer[1] ,20);
	}
  _FN_END
}