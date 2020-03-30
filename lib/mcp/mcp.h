#ifndef MCP_H
#define MCP_H

/*
*   Library intended for use with the MCP23017 IO expander chip. 
*   Library assumes address pins are grounded
*   Written by: Julie Brown
*   Date: March 28 2020
*/

/* as per data sheet */
#define MCP_I2C_ADDR 0x20 

enum REG_AB
{
    A = 0x00,
    B = 0x01
};

void mcp_set(REG_AB reg, uint8_t pinID, uint8_t dir);

void mcp_refresh();
void mcp_write(REG_AB reg, uint8_t pinID, uint8_t state);
uint8_t mcp_read(REG_AB reg, uint8_t pinID);

#endif
