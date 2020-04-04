#ifndef SYRINGE_H
#define SYRINGE_H

// #define DEBUG
// #define SLOW_DEBUG
// #define PRINT_STACK_TRACE

#define MAIN
// #define MCP_LED_TEST
// #define MCP_SWITCH_TEST
// #define LCD_TEST
// #define LCD_W_BUTTON_TEST

#define SERIAL_BAUD_RATE 9600

#ifdef DEBUG
#define SERIAL_BUFFER_SIZE 256
#else
#define SERIAL_BUFFER_SIZE 80
#endif

#define MIN_FEED_MINUTE 1
#define MAX_FEED_MINUTE 150 // 2.5 hr. wow such math

enum pins_e
{
ARDUINO_IO = 0,
/*********************************/
/* DO NOT USE IF YOU NEED SERIAL */
	COMTX =            ARDUINO_IO + 0,
	COMRX =            ARDUINO_IO + 1,
/*********************************/
	MOTOR_IO =         ARDUINO_IO + 2,
	MOTOR_PWM =        ARDUINO_IO + 3,
	NC_4 =             ARDUINO_IO + 4,
	NC_5 =             ARDUINO_IO + 5,
	NC_6 =             ARDUINO_IO + 6,
	NC_7 =             ARDUINO_IO + 7,
	NC_8 =             ARDUINO_IO + 8,
	Buzzer =           ARDUINO_IO + 9,
	MCU_SRAM_CS =      ARDUINO_IO + 10,
	MCU_MOSI =         ARDUINO_IO + 11,
	MCU_MISO =         ARDUINO_IO + 12,
	MCU_SCK =          ARDUINO_IO + 13,
/* We start the IO expander here (section A) */
A_ROW_EXPENDER = 14, 
	B_auto_feed =      A_ROW_EXPENDER + 0,
	B_Inc =            A_ROW_EXPENDER + 1,
	B_Dec =            A_ROW_EXPENDER + 2,
	B_feed =           A_ROW_EXPENDER + 3,
	B_fill =           A_ROW_EXPENDER + 4,
	empty_switch_NC =  A_ROW_EXPENDER + 5,
	full_switch_NC =   A_ROW_EXPENDER + 6,
	B_Emergency_NO =   A_ROW_EXPENDER + 7,
/* We start the IO expander here (section B) */
B_ROW_EXPENDER = 22, 
	LED_auto_feed =    B_ROW_EXPENDER + 0,
	LED_Inc =          B_ROW_EXPENDER + 1,
	LED_Dec =          B_ROW_EXPENDER + 2,
	LED_feed =         B_ROW_EXPENDER + 3,
	LED_fill =         B_ROW_EXPENDER + 4,
	empty_switch_NO =  B_ROW_EXPENDER + 5,
	full_switch_NO =   B_ROW_EXPENDER + 6,
	B_Emergency_NC =   B_ROW_EXPENDER + 7
};

#endif