#ifndef MESSAGE_H
#define MESSAGE_H

#include "syringe.h"

#ifndef SERIAL_BAUD_RATE
#define SERIAL_BAUD_RATE 9600
#endif

#ifndef SERIAL_BUFFER_SIZE
// should be sufficient as a default but can be overriden
#define SERIAL_BUFFER_SIZE 256
#endif

#define BITS_PATTERN "%c%c%c%c%c%c%c%c"
#define TO_BITS(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

extern uint8_t stack_size;
void serial_printf(const char * format, ...);
void debug_printf(const char * format, ...);

#ifdef DEBUG
#ifdef PRINT_STACK_TRACE
#define _FN_START  {debug_printf("\n %d == %s %s::%d", stack_size++,  __FILE__, __FUNCTION__, __LINE__);}
#else
#define _FN_START
#endif
#else
#define _FN_START
#endif



#ifdef SLOW_DEBUG
#define _FN_END {stack_size--; delay(100);}
#else
#define _FN_END {stack_size--;}
#endif

#endif