#ifndef SYRINGE_H
#define SYRINGE_H

enum pins_e
{
	/*****************
	 * DO NOT USE IF YOU NEED SERIAL */
	COMTX = 0x0,
	COMRX = 0x1,
	/*****************/
	NC_2 = 0x2,
	NC_3 = 0x3,
	NC_4 = 0x4,
	NC_5 = 0x5,
	NC_6 = 0x6,
	NC_7 = 0x7,
	NC_8 = 0x8,
	NC_9 = 0x9,
	MCU_SRAM_CS = 0x10,
	MCU_MOSI = 0x11,
	MCU_MISO = 0x12,
	MCU_SCK = 0x13,
    // We start expander here (section A)
	EXP_A_START = 0x14, 
	B_auto_feed = 0x14,
	B_Inc = 0x15,
	B_Dec = 0x16,
	B_feed = 0x17,
	B_fill = 0x18,
	B_Emergency = 0x19,
	empty_switch = 0x20,
	full_switch = 0x21,
    // We start expander here (section B)
	EXP_B_START = 0x22, 
	LED_auto_feed = 0x22,
	LED_Inc = 0x23,
	LED_Dec = 0x24,
	LED_feed = 0x25,
	LED_fill = 0x26,
	Buzzer = 0x27,
	NC_28 = 0x28,
	NC_29 = 0x29,
	NC_30 = 0x30,
	NB_OF_PIN = NC_30
};

#endif