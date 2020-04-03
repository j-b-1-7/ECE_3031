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
	empty_switch_NC = 2,
	empty_switch_NO = 3,
	full_switch_NC = 4,
	full_switch_NO = 5,
	B_Emergency_NC = 6,
	B_Emergency_NO = 7,
	NC_8 = 8,
	Buzzer = 9,
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
	NC_19 = 19,
	NC_20 = 20,
	NC_21 = 21,
    // We start expander here (section B)
	EXP_B_START = 22, 
	LED_auto_feed = 22,
	LED_Inc = 23,
	LED_Dec = 24,
	LED_feed = 25,
	LED_fill = 26,
	NC_27 = 27,
	NC_28 = 28,
	NC_29 = 29,
	NC_30 = 30,
	NB_OF_PIN = NC_30
};

#endif