/*
*   Library intended for use with the MCP23017 IO expander chip. 
*   Library assumes address pins are grounded
*   Written by: Julie Brown
*   Date: March 28 2020
*/

#include <Wire.h>
#include <stdlib.h>
#include <my_mcp_library.h>


#define GPIO_OFFSET 0x12

uint8_t IN_REG[2] = { 0 };
uint8_t OUT_REG[2] = { 0 };

uint8_t IODIR[2] = { 0 };

void buffer_IO_direction(REG_AB reg, uint8_t pinID, PIN_DIRECTION dir)
{

    IODIR[reg] &= ~(1 << pinID);
    IODIR[reg] |= (dir << pinID);
    
}

void write_IO_direction()
{

    Wire.beginTransmission(MCP_ADD);

    Wire.write(A);
    Wire.write(IODIR[A]);

    Wire.endTransmission();

    Wire.beginTransmission(MCP_ADD);

    Wire.write(B);
    Wire.write(IODIR[B]);

    Wire.endTransmission();

}



void mcp_buffer_pin(REG_AB reg, uint8_t pinID, uint8_t state)
{
    OUT_REG[reg] &= ~(1 << pinID);
    OUT_REG[reg] |= (state << pinID);
}

void mcp_write()
{

    Wire.beginTransmission(MCP_ADD);
    
    Wire.write(A+GPIO_OFFSET);

    if(Wire.write(OUT_REG[A]) == 0)
    {
        //ERROR
    }

    Wire.endTransmission();

    Wire.beginTransmission(MCP_ADD);
    
    Wire.write(B+GPIO_OFFSET);
    Wire.write(OUT_REG[B]);

    Wire.endTransmission();

}

uint8_t mcp_read(REG_AB reg, uint8_t pinID)
{
    return (IN_REG[reg] & (1 << pinID));       
}

void mcp_update_pins()
{

    Wire.beginTransmission(MCP_ADD);

    Wire.write(A+GPIO_OFFSET);
    Wire.requestFrom(MCP_ADD, 1);

    IN_REG[A] = Wire.read();

    Wire.endTransmission();

    Wire.beginTransmission(MCP_ADD);

    Wire.write(A+GPIO_OFFSET);
    Wire.write(OUT_REG[A]);

    Wire.endTransmission();

    Wire.beginTransmission(MCP_ADD);

    Wire.write(B+GPIO_OFFSET);
    Wire.requestFrom(MCP_ADD, 1);

    IN_REG[B] = Wire.read();
    
    Wire.endTransmission();

    Wire.beginTransmission(MCP_ADD);

    Wire.write(B+GPIO_OFFSET);
    Wire.write(OUT_REG[B]);

    Wire.endTransmission();
    
}

uint8_t get_reg_val(REG_AB reg)
{

    return IN_REG[reg];

}
