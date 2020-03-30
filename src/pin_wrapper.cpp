#include <Arduino.h>
#include <Wire.h>
#include <stdio.h>
#include <stdlib.h>

#include "syringe.h"
#include "mcp.h"

void refreshPin()
{
    /* any fn call to get the IO values are here */
    mcp_refresh();
}

void setPin(uint8_t pin, uint8_t val)
{
	if(pin < EXP_A_START)
	{
		pinMode(pin, val);
	}
	else if (pin < EXP_B_START)
	{
		mcp_set(A, pin-EXP_A_START, val);
	}
	else
	{
		mcp_set(B, pin-EXP_B_START, val);
	}
}

uint8_t readPin(uint8_t pin)
{
	uint8_t val = 0x0;
	if(pin < EXP_A_START)
	{
		val = digitalRead(pin);
	}
	else if (pin < EXP_B_START)
	{
		val = mcp_read(A,pin-EXP_A_START);
	}
	else
	{
		val = mcp_read(B,pin-EXP_B_START);
	}
	return val;
}

void writePin(uint8_t pin, uint8_t val)
{
	if(pin < EXP_A_START)
	{
		digitalWrite(pin, val);
	}
	else if (pin < EXP_B_START)
	{
		mcp_write(A, pin-EXP_A_START, val);
	}
	else
	{
		mcp_write(B, pin-EXP_B_START, val);
	}
}
