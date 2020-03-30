#include <Arduino.h>
#include <Wire.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "i2c.h"
#include "i2c_lcd.h"
#include "message.h"

#include "ST7036.h"

ST7036 lcd = ST7036 ( LCD_LINES, LCD_LINE_LEN, LCD_ADDR );
#define MAX_BUFFER ((LCD_LINE_LEN * LCD_LINES) + 2)

bool _init_lcd = true;
uint8_t previous_buffer[LCD_LINES][LCD_LINE_LEN];

static void init_LCD()
{
  _FN_START
  if(_init_lcd)
  {
    _init_lcd = false;
    lcd.init();
    lcd.setContrast(10);
  }
  
  _FN_END
}

void lcd_printf(const char * format, ...)
{
  _FN_START

	init_LCD(); 

  char buffer[MAX_BUFFER];
  va_list args;
  va_start (args, format);
  vsnprintf (buffer,MAX_BUFFER, format, args);
  va_end (args);

	/* first we look to see if a different line is sent */
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
      }

      if(previous_buffer[row][col] != (uint8_t)c)
      {
        previous_buffer[row][col] = (uint8_t)c;
        lcd.setCursor(row,col);
        lcd.write(previous_buffer[row][col]);
      }
    }
	}

  _FN_END
}