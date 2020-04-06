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
// #define SR_SWITCH_TEST

#define SERIAL_BAUD_RATE 9600

#ifdef DEBUG
#define SERIAL_BUFFER_SIZE 256
#else
#define SERIAL_BUFFER_SIZE 80
#endif

#define MIN_FEED_MINUTE 2
#define MAX_FEED_MINUTE 150 // 2.5 hr. wow such math

// analog pins
#define MOTOR_ENABLE A0

enum pins_e
{
ARDUINO_IO = 0,
/*********************************/
/* DO NOT USE IF YOU NEED SERIAL */
	COMTX =            ARDUINO_IO + 0,
	COMRX =            ARDUINO_IO + 1,
/*********************************/
	B_Inc =            ARDUINO_IO + 2,
	B_Dec =            ARDUINO_IO + 3,
	B_auto_feed =      ARDUINO_IO + 4,
	MOTOR_IN1 =        ARDUINO_IO + 5,
	MOTOR_IN2 =        ARDUINO_IO + 6,
	B_feed =           ARDUINO_IO + 7,
	B_fill =           ARDUINO_IO + 8,
	Buzzer =           ARDUINO_IO + 9,
	MCU_SRAM_CS =      ARDUINO_IO + 10,
	MCU_MOSI =         ARDUINO_IO + 11,
	MCU_MISO =         ARDUINO_IO + 12,
	MCU_SCK =          ARDUINO_IO + 13,
/* We start the IO expander here (section A) */
A_ROW_EXPENDER = ARDUINO_IO + 14, 
	empty_switch_NC =  A_ROW_EXPENDER + 0,
	empty_switch_NO =  A_ROW_EXPENDER + 1,
	BROKEN_A_2 =       A_ROW_EXPENDER + 2,
	NC_A_2 =           A_ROW_EXPENDER + 3,
	B_Emergency_NC =   A_ROW_EXPENDER + 4,
	B_Emergency_NO =   A_ROW_EXPENDER + 5,
	full_switch_NC =   A_ROW_EXPENDER + 6,
	full_switch_NO =   A_ROW_EXPENDER + 7,
/* We start the IO expander here (section B) */
B_ROW_EXPENDER = A_ROW_EXPENDER + 8, 
	LED_auto_feed =    B_ROW_EXPENDER + 0,
	LED_Inc =          B_ROW_EXPENDER + 1,
	LED_Dec =          B_ROW_EXPENDER + 2,
	LED_feed =         B_ROW_EXPENDER + 3,
	LED_fill =         B_ROW_EXPENDER + 4,
	Motor_Nsleep =     B_ROW_EXPENDER + 5,
	NC_B_6 =           B_ROW_EXPENDER + 6,
	NC_B_7 =           B_ROW_EXPENDER + 7,
};

#endif