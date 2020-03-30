/*
*   Library intended for use with the MCP23017 IO expander chip. 
*   Library assumes address pins are grounded
*   Written by: Julie Brown
*   Date: March 28 2020
*/

#include <Arduino.h>
#include <Wire.h>
#include <stdlib.h>

#include "message.h"
#include "mcp.h"
#include "i2c.h"

#define DEF_OFFSET 0x0
#define GPIO_OFFSET 0x12

static uint8_t REG[2] = { 0 };
static uint8_t IODIR[2] = { 0 };
static bool is_init = false;

static uint8_t _read(uint8_t *src, uint8_t id)
{
    return (*src >> id) & 0x1;
}

static void _write(uint8_t *src, uint8_t id, uint8_t value)
{
    *src &= ~(0x1 << id);
    *src |= (value << id);
}

static uint8_t _mcp_read(uint8_t addr)
{
    uint8_t dest = 0x0;
    i2c_read(MCP_I2C_ADDR, addr, &dest, 1);
    return dest;
}

static void _mcp_write(uint8_t addr, uint8_t value)
{
    i2c_write(MCP_I2C_ADDR, addr, &value, 1);
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

void mcp_set(REG_AB reg, uint8_t pinID, uint8_t dir)
{
    // we need to flip the direction since mcp expects I = 1, O = 0
    // but arduino uses INPUT = 0x0 and OUTPUT = 0x1
    uint8_t reverse_direction = (~dir);
    _write(&IODIR[reg], pinID, reverse_direction);
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
    uint8_t val = _read(&REG[reg], pinID);
    return val;
}
