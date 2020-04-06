#include <Arduino.h>
#include <stdio.h>

#include "message.h"

uint8_t stack_size = 1;
static bool _serial_initialized = false;
static char _serial_printer_buffer[SERIAL_BUFFER_SIZE] = { 0 };

void serial_display()
{
    if(!_serial_initialized)
    {
        Serial.begin(SERIAL_BAUD_RATE);
        _serial_initialized = true;
    }
    
    char *str = _serial_printer_buffer;
    char *cur = str;
    for (uint16_t i =0; i < SERIAL_BUFFER_SIZE-1 && cur[i] != '\0'; i += 1)
    {
        if(cur[i] == '\n')
        {
            cur[i] = '\0';
            Serial.println(str);
            str = &cur[i+1];
        }
    }

    Serial.print(str);
}

void serial_printf(const char * format, ...)
{
    va_list args;
    va_start (args, format);
    vsnprintf (_serial_printer_buffer,SERIAL_BUFFER_SIZE, format, args);
    va_end (args);
    serial_display();
}

void debug_printf(const char * format, ...)
{
#ifdef DEBUG
    va_list args;
    va_start (args, format);
    vsnprintf (_serial_printer_buffer,SERIAL_BUFFER_SIZE, format, args);
    va_end (args);
    serial_display();
#endif
}