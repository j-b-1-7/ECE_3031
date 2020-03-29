#include <Arduino.h>
#include <Wire.h>
#include <stdio.h>
#include <stdlib.h>

#include "my_I2C_LCD.h"

#define SLAVE 0x3C
#define COMSEND 0x00
#define DATASEND 0x40
#define LINE2 0xC0
#define LINE1 0x00


static void newLine()
{
  Wire.beginTransmission(SLAVE);
  Wire.write(COMSEND);
  Wire.write(LINE2);
  Wire.endTransmission();
}

static void CGRAM()
{
    Wire.beginTransmission(SLAVE);
    Wire.write(COMSEND);
    Wire.write(0x38);
    Wire.write(0x40);
    Wire.endTransmission();
    delay(10);
    Wire.beginTransmission(SLAVE);
    Wire.write(DATASEND);
    Wire.write(0x00);
    Wire.write(0x1E);
    Wire.write(0x18);
    Wire.write(0x14);
    Wire.write(0x12);
    Wire.write(0x01);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.endTransmission(); 
}

void transmit_data(char *out_string)
{
  // This will be used to write a string to the LCD
  Wire.beginTransmission(SLAVE);
  Wire.write(DATASEND);

  int count = 0;
  while(count < 20)
  {
    Wire.write(*out_string);
    out_string += 1;
    count += 1;
  }
  Wire.endTransmission();
  
}

void init_LCD()
{
    Wire.beginTransmission(SLAVE);
    Wire.write(COMSEND);
    Wire.write(0x38);
    delay(10);
    Wire.write(0x39);
    delay(10);
    Wire.write(0x14);
    Wire.write(0x70);
    Wire.write(0x5E);
    Wire.write(0x6D);
    Wire.write(0x0C);
    Wire.write(0x01);
    Wire.write(0x06);
    short test = 55;
    test = Wire.endTransmission();
 
    CGRAM();

    Wire.beginTransmission(SLAVE);
    Wire.write(COMSEND);
    Wire.write(0x39);
    Wire.write(0x01);
    Wire.endTransmission();
    Wire.endTransmission();
    while(test!=0)
    {
      delay(100);
    }
    delay(10);
}