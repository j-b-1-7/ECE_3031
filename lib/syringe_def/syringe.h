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
	/*****************
	 * DO NOT USE IF YOU NEED SERIAL */
	COMTX = 0,
	COMRX = 1,
	/*****************/
	NC_2 = 2,
	NC_3 = 3,
	NC_4 = 4,
	NC_5 = 5,
	NC_6 = 6,
	NC_7 = 7,
	NC_8 = 8,
	NC_9 = 9,
	MCU_SRAM_CS = 10,
	MCU_MOSI = 11,
	MCU_MISO = 12,
	MCU_SCK = 13,
    // We start expander here (section A)
	EXP_A_START = 14, 
	B_auto_feed = 14,
	B_Inc = 15,
	B_Dec = 16,
	B_feed = 17,
	B_fill = 18,
	B_Emergency = 19,
	empty_switch = 20,
	full_switch = 21,
    // We start expander here (section B)
	EXP_B_START = 22, 
	LED_auto_feed = 22,
	LED_Inc = 23,
	LED_Dec = 24,
	LED_feed = 25,
	LED_fill = 26,
	Buzzer = 27,
	NC_28 = 28,
	NC_29 = 29,
	NC_30 = 30,
	NB_OF_PIN = NC_30
};

#endif