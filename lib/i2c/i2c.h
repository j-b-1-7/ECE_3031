#ifndef I2C_H
#define I2C_H

#include <Arduino.h>

void i2c_write(uint8_t dev_addr, uint8_t reg_addr, const void* src, uint16_t n_byte);
void i2c_write(uint8_t dev_addr, uint8_t reg_addr, const char* src);
void i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t value);

void i2c_read(uint8_t dev_addr, uint8_t reg_addr, void* dest, uint16_t n_byte);
uint8_t i2c_read(uint8_t dev_addr, uint8_t reg_addr);

#endif