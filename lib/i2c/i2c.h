#include <Arduino.h>

void i2c_write(uint8_t dev_addr, uint8_t reg_addr, void* src, uint16_t n_byte);
void i2c_read(uint8_t dev_addr, uint8_t reg_addr, void *dest, uint16_t n_byte);
