#ifndef MY_MCP_LIBRARY_H
#define MY_MCP_LIBRARY_H

/*
*   Library intended for use with the MCP23017 IO expander chip. 
*   Library assumes address pins are grounded
*   Written by: Julie Brown
*   Date: March 28 2020
*/

#define MCP_ADD 0x20

enum REG_AB
{
    A = 0x00,
    B = 0x01
};

enum PIN_DIRECTION
{
    OUTPUT_PIN = 0x00,
    INPUT_PIN = 0x01
};

void mcp_set(REG_AB reg, uint8_t pinID, PIN_DIRECTION dir);

void mcp_refresh();
void mcp_write(REG_AB reg, uint8_t pinID, uint8_t state);
uint8_t mcp_read(REG_AB reg, uint8_t pinID);

#endif
