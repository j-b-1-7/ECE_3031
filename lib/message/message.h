#ifndef MESSAGE_H
#define MESSAGE_H

#ifndef SERIAL_BAUD_RATE
#define SERIAL_BAUD_RATE 9600
#endif

#ifndef SERIAL_BUFFER_SIZE
// should be sufficient as a default but can be overriden
#define SERIAL_BUFFER_SIZE 256
#endif

#define DEBUG

extern uint8_t stack_size;
void serial_printf(const char * format, ...);
void debug_printf(const char * format, ...);

#define _FN_START  {debug_printf("\n %d == %s %s::%d\n", stack_size++,  __FILE__, __FUNCTION__, __LINE__);}
#define _FN_END {stack_size--; delay(250);}
#endif