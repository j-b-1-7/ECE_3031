#include <Arduino.h>
#include <Wire.h>

#include "message.h"

static bool _i2c_initialized = false;

static void i2c_init()
{
    if(!_i2c_initialized)
    {
        _FN_START
        _i2c_initialized = true;
        Wire.begin();
        _FN_END
    }
}

void i2c_write(uint8_t dev_addr, uint8_t reg_addr, void* src, uint16_t n_byte)
{
    _FN_START

    i2c_init();

    uint8_t *buf = (uint8_t*)src;    
    Wire.beginTransmission(dev_addr);
    Wire.write(reg_addr);
    for (uint16_t i = 0; i < n_byte; i += 1)
    {
        Wire.write(buf[i]);
        delay(2);
        debug_printf("0x%02X ", buf[i]);
    }
    Wire.endTransmission();

    _FN_END
}

void i2c_read(uint8_t dev_addr, uint8_t reg_addr, void* dest, uint16_t n_byte)
{
    _FN_START

    i2c_init();
    
    uint8_t *buf = (uint8_t*)dest;
    Wire.beginTransmission(dev_addr);
    Wire.write(reg_addr);
    Wire.endTransmission();

    Wire.requestFrom(reg_addr, n_byte);
    for (uint16_t i = 0; i < n_byte &&  Wire.available(); i += 1)
    {
        buf[i] = (uint8_t)Wire.read();
        delay(2);
        debug_printf("0x%02X ", buf[i]);
    }
    
    _FN_END
}




