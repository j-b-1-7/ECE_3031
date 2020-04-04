#include <Arduino.h>
#include <Wire.h>
#include <stdio.h>
#include <stdlib.h>

#include "pin_wrapper.h"
#include "syringe.h"
#include "mcp.h"

static uint16_t time_since(uint16_t from, uint16_t to)
{
	if(from > to)
	{
		// we overflowed so we count from (previous to max value) + current 
		return (UINT16_MAX - from) + to;
	}
	else
	{
		return to - from;
	}
}

void refreshPin()
{
#ifdef PIN_REFRESH_RATE
	// assign globally
	static uint16_t last_time = 0;
	uint16_t current_millis = millis();
	if (last_time != current_millis 
	&& time_since(last_time, current_millis) > PIN_REFRESH_RATE)
	{
		last_time = current_millis;
#else
	{
#endif
		mcp_refresh();
	}
}

void setPin(uint8_t pin, uint8_t val)
{
	if(pin < A_ROW_EXPENDER)
	{
		pinMode(pin, val);
	}
	else if (pin < B_ROW_EXPENDER)
	{
		mcp_set(A, pin-A_ROW_EXPENDER, val);
	}
	else
	{
		mcp_set(B, pin-B_ROW_EXPENDER, val);
	}
}

uint8_t readPin(uint8_t pin)
{
	uint8_t val = 0x0;
	if(pin < A_ROW_EXPENDER)
	{
		val = digitalRead(pin);
	}
	else if (pin < B_ROW_EXPENDER)
	{
		val = mcp_read(A,pin-A_ROW_EXPENDER);
	}
	else
	{
		val = mcp_read(B,pin-B_ROW_EXPENDER);
	}
	return val;
}

void writePin(uint8_t pin, uint8_t val)
{
	if(pin < A_ROW_EXPENDER)
	{
		digitalWrite(pin, val);
	}
	else if (pin < B_ROW_EXPENDER)
	{
		mcp_write(A, pin-A_ROW_EXPENDER, val);
	}
	else
	{
		mcp_write(B, pin-B_ROW_EXPENDER, val);
	}
}
