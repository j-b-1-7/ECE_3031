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

uint8_t REG[2] = { 0 };
uint8_t IODIR[2] = { 0 };
bool is_init = false;

uint8_t _read(uint8_t *src, uint8_t id)
{
    uint8_t val = *src & (1 << id);
}

void _write(uint8_t *src, uint8_t id, uint8_t value)
{
    value &= 0x1; // make sure we only set one bit
    *src &= ~(1 << id);
    *src |= (value << id);
}

uint8_t _mcp_read(uint8_t addr)
{
    Wire.beginTransmission(MCP_ADD);
    Wire.write(addr);
    Wire.requestFrom(MCP_ADD, 1);
    uint8_t value = Wire.read();
    Wire.endTransmission();
    return value;
}

void _mcp_write(uint8_t addr, uint8_t value)
{
    Wire.beginTransmission(MCP_ADD);
    Wire.write(addr);
    Wire.write(value);
    Wire.endTransmission();
}

/**
 * expects:
 *  
 */
uint8_t _mcp_refresh(uint8_t addr, uint8_t mask, uint8_t write_value)
{
    uint8_t read_value = _mcp_read(addr);
    uint8_t o_value = write_value &= ~(mask);
    uint8_t i_value = read_value &= (mask);
    // join the values to get the real one
    uint8_t writeback = o_value | i_value;
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
