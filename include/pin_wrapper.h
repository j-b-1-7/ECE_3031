#ifndef PIN_WRAPPER_H
#define PIN_WRAPPER_H

#include <Arduino.h>

void refreshPin();

void setPin(uint8_t pin, uint8_t val);
uint8_t readPin(uint8_t pin);
void writePin(uint8_t pin, uint8_t val);

#endif