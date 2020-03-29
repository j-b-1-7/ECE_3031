/*
*   Library intended for use with the MCP23017 IO expander chip. 
*   Library assumes address pins are grounded
*   Written by: Julie Brown
*   Date: March 28 2020
*/

#include <Wire.h>
#include <stdlib.h>
#include <my_mcp_library.h>

#define DEF_OFFSET 0x0
#define GPIO_OFFSET 0x12

static uint8_t REG[2] = { 0 };
static uint8_t IODIR[2] = { 0 };
static bool is_init = false;

static uint8_t _read(uint8_t *src, uint8_t id)
{
    return (*src & (1 << id));
}

static void _write(uint8_t *src, uint8_t id, uint8_t value)
{
    value &= 0x1; // make sure we only set one bit
    *src &= ~(1 << id);
    *src |= (value << id);
}

static uint8_t _mcp_read(uint8_t addr)
{
    Wire.beginTransmission(MCP_ADD);
    Wire.write(addr);
    Wire.requestFrom(MCP_ADD, 1);
    uint8_t value = Wire.read();
    Wire.endTransmission();
    return value;
}

static void _mcp_write(uint8_t addr, uint8_t value)
{
    Wire.beginTransmission(0x20);
    Wire.write(0x13);
    Wire.write(value);
    Wire.endTransmission();
}

/**
 * expects:
 *  address to write to
 *  the bitmask (I = 1, O = 0)
 *  the write value
 * return:
 *  the read value
 */
static uint8_t _mcp_refresh(uint8_t addr, uint8_t mask, uint8_t write_value)
{
    uint8_t read_value = _mcp_read(addr);
    // join the values to get the real one
    uint8_t writeback = ( write_value & (~mask) ) | ( read_value & mask );
    _mcp_write(addr, writeback);
    return writeback;
}

void mcp_set(REG_AB reg, uint8_t pinID, PIN_DIRECTION dir)
{
    _write(&IODIR[reg], pinID, dir);
}

void mcp_refresh()
{
    if(!is_init)
    {
        // set the direction
        is_init = true;
        _mcp_write(DEF_OFFSET + A, IODIR[A]);
        _mcp_write(DEF_OFFSET + B, IODIR[B]);
    }
    REG[A] = _mcp_refresh(GPIO_OFFSET + A, IODIR[A], REG[A]);
    REG[B] = _mcp_refresh(GPIO_OFFSET + B, IODIR[B], REG[B]);    
}

void mcp_write(REG_AB reg, uint8_t pinID, uint8_t state)
{
    _write(&REG[reg], pinID, state);
}

uint8_t mcp_read(REG_AB reg, uint8_t pinID)
{
    return _read(&REG[reg], pinID);
}
