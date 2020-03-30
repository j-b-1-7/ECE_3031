#include <Arduino.h>
#include <Wire.h>

#include "message.h"

#define HEX_STR "0x%02X"
constexpr char _unable_to[] = "unable to";
constexpr char _write[] = "write to";
constexpr char _read[] = "read from";

static bool _i2c_initialized = false;

static uint8_t _end_transmission(uint8_t dev_addr)
{
    uint8_t exit_code = Wire.endTransmission(true);
    switch(exit_code)
    {
        case 0:
            break;
        case 1:
            serial_printf("\n!Buffer Overflow dev" HEX_STR ,dev_addr);
            break;
        case 2:
            serial_printf("\n!NACK dev" HEX_STR, dev_addr);
            break;
        case 3:
            serial_printf("\n!NACK of dev" HEX_STR, dev_addr);
            break;
        default:
            serial_printf("\n!UNK ERR dev" HEX_STR, dev_addr);
            break;
    }
    return exit_code;
}

static uint8_t _begin_transmission(uint8_t dev_addr, uint8_t reg_addr)
{
    if(!_i2c_initialized)
    {
        _FN_START
        _i2c_initialized = true;
        Wire.begin();
        delay(100);
        _FN_END
    }

    Wire.beginTransmission(dev_addr);
    if( Wire.write(reg_addr) != 1 )
    {
        return _end_transmission(dev_addr);
    }
    else
    {
        return 0;
    }
}

void i2c_write(uint8_t dev_addr, uint8_t reg_addr, const void* src, uint16_t n_byte)
{
    _FN_START

    if ( 0 == _begin_transmission(dev_addr, reg_addr) )
    {
        uint8_t *buf = (uint8_t*)src;    
        if( n_byte != (uint16_t)Wire.write(buf, n_byte))
        {
            serial_printf("%s %s dev" HEX_STR, _unable_to, _write, dev_addr);
        }
        _end_transmission(dev_addr);
    }
    _FN_END
}

void i2c_write(uint8_t dev_addr, uint8_t reg_addr, const char* src)
{
    _FN_START

    if ( 0 == _begin_transmission(dev_addr, reg_addr) )
    {
        if( strlen(src) != (uint16_t)Wire.write(src))
        {
            serial_printf("%s %s dev" HEX_STR, _unable_to, _write, dev_addr);
        }
        _end_transmission(dev_addr);
    }
    _FN_END
}

void i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t value)
{
    _FN_START

    if ( 0 == _begin_transmission(dev_addr, reg_addr) )
    {
        if( 1 != Wire.write(value))
        {
            serial_printf("%s %s dev" HEX_STR, _unable_to, _write, dev_addr);
        }
        _end_transmission(dev_addr);
    }
    _FN_END
}

void i2c_read(uint8_t dev_addr, uint8_t reg_addr, void* dest, uint16_t n_byte)
{
    _FN_START
    
    uint8_t *buf = (uint8_t*)dest;
    if ( 0 == _begin_transmission(dev_addr, reg_addr) )
    {
        _end_transmission(dev_addr);
        Wire.requestFrom(dev_addr, n_byte);

        if(n_byte == (uint16_t)Wire.available())
        {
            for (uint16_t i = 0; i < n_byte; i += 1)
            {
                buf[i] = (uint8_t)Wire.read();
            }
        }
        else
        {
            serial_printf("%s %s dev" HEX_STR, _unable_to, _read, dev_addr);
        }

    }

    _FN_END
}

uint8_t i2c_read(uint8_t dev_addr, uint8_t reg_addr)
{
    _FN_START
    
    uint8_t value = 0x0;
    if ( 0 == _begin_transmission(dev_addr, reg_addr) )
    {
        _end_transmission(dev_addr);
        Wire.requestFrom(dev_addr,0x1U);
        if(1 == Wire.available())
        {
            value = (uint8_t)Wire.read();   
        }
        else
        {
            serial_printf("%s %s dev" HEX_STR, _unable_to, _read, dev_addr);
        }     

    }

    _FN_END
    return value;
}




