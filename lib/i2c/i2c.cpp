#include <Arduino.h>
#include <Wire.h>

#include "message.h"

static bool _i2c_initialized = false;

static uint8_t _end_transmission(uint8_t dev_addr)
{
    uint8_t exit_code = Wire.endTransmission(true);
    switch(exit_code)
    {
        case 0:
            break;
        case 1:
            serial_printf("\nERROR: data too long to fit in transmit buffer on dev::0x%02X", dev_addr);
            break;
        case 2:
            serial_printf("\nERROR: received NACK on transmit of address on dev::0x%02X", dev_addr);
            break;
        case 3:
            serial_printf("\nERROR: received NACK on transmit of data on dev::0x%02X", dev_addr);
            break;
        default:
            serial_printf("\nERROR: other error on dev::0x%02X", dev_addr);
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
            debug_printf("Unable to write: ");
            for (uint16_t i = 0; i < n_byte; i += 1)
            {
                debug_printf("0x%02X, ", buf[i]);
            }
            debug_printf("\n");
        }
        _end_transmission(dev_addr);
    }
    _FN_END
}

void i2c_write(uint8_t dev_addr, uint8_t reg_addr, const char* src)
{
    i2c_write(dev_addr, reg_addr, src, strlen(src));
}

void i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t value)
{
    _FN_START

    if ( 0 == _begin_transmission(dev_addr, reg_addr) )
    {
        if( 1 != Wire.write(value))
        {
            debug_printf("Unable to write 0x%02X\n", value);
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
            debug_printf("Unable to read from device[0x%02X]\n", dev_addr);
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
            debug_printf("Unable to read from device[0x%02X]\n", dev_addr);
        }     

    }

    _FN_END
    return value;
}




