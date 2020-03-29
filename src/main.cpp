#include <Arduino.h>
#include <Wire.h>
#include <stdio.h>
#include <stdlib.h>

#include "MicrochipSRAM.h"                // Include the library
#include "my_mcp_library.h"
#include "my_I2C_LCD.h"

#define PIN_REFRESH_RATE 1
#define SHORT_DEBOUNCE_COUNT 1
#define LONG_DEBOUNCE_COUNT 1
#define DEBOUNCE_ON (PIN_REFRESH_RATE*LONG_DEBOUNCE_COUNT)
#define DEBOUNCE_OFF (PIN_REFRESH_RATE*SHORT_DEBOUNCE_COUNT)

#define MCU_SDA A4
#define MCU_SCL A5

#define MAX_SPI_SPEED 20000000

#define READ 0x03
#define WRITE 0x02
#define EDIO 0x3B
#define RSTIO 0xFF
#define RDMR 0x05
#define WRMR 0x01

/* DO NOT USE IF YOU NEED SERIAL */
#define COMTX 0
#define COMRX 1
///////////////////

enum pins_e
{
	NC_0 = 0x0,
	NC_1 = 0x1,
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
	EXP_A_START = 0x14, // We start expander here (section A)
	B_auto_feed = 0x14,
	B_Inc = 0x15,
	B_Dec = 0x16,
	B_feed = 0x17,
	B_fill = 0x18,
	B_Emergency = 0x19,
	empty_switch = 0x20,
	full_switch = 0x21,
	EXP_B_START = 0x22, // We start expander here (section B)
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

void setPin(uint8_t pin, uint8_t val)
{
	if(pin < EXP_A_START)
	{
		pinMode(pin, val);
	}
	else if (pin < EXP_B_START)
	{
		mcp_set(A, pin, val);
	}
	else
	{
		mcp_set(B, pin, val);
	}
}

uint8_t readPin(uint8_t pin)
{
	uint8_t val = 0x0;
	if(pin < EXP_A_START)
	{
		val = digitalRead(pin);
	}
	else if (pin < EXP_B_START)
	{
		val = mcp_read(A,pin);
	}
	else
	{
		val = mcp_read(B,pin);
	}
	return val;
}

void writePin(uint8_t pin, uint8_t val)
{
	if(pin < EXP_A_START)
	{
		digitalWrite(pin, val);
	}
	else if (pin < EXP_B_START)
	{
		mcp_write(A, pin, val);
	}
	else
	{
		mcp_write(B, pin, val);
	}
}

static MicrochipSRAM memory(MCU_SRAM_CS); // Instantiate the class to the given pin

#define MEM_ADDR 0x100

typedef struct my_time_t_t
{
	long seconds;
	long minutes;
	long hours;
} my_time_t;

typedef struct trigger_t_t
{
	long count[2];
	long millis;
	bool count_up;
} trigger_t;

long pin_trigger_millis[NB_OF_PIN] = { 0 };

bool man_feed = false;
bool auto_feed = false;
bool fill = false;

// lcd is 2X20
#define LCD_BUFFER_LEN 40

char previous_buffer[LCD_BUFFER_LEN] = { 0 };

void lcd_print(const char *str)
{
	// if a different line is sent
	bool is_diff = false;
	int i = 0;
	while (str[i] && i < LCD_BUFFER_LEN)
	{
		if (str[i])
		{
			if(previous_buffer[i] != str[i])
			{
				// putc
				// remove is_diff when we can putc
				is_diff = true;
			}
			previous_buffer[i] = str[i];
		}
		else
		{
			previous_buffer[i] = '\0';
		}
		i += 1;
	}

	// this could go away if we change only one char
	if(is_diff)
	{
		Serial.println(previous_buffer);
		init_LCD();
		transmit_data(previous_buffer);
	}
}

void init_count(trigger_t *pin)
{
	pin->count[0] = 0;
	pin->count[1] = 0;
}

void stop_timer(trigger_t *pin)
{
	init_count(pin);
	pin->millis = 0;
	pin->count_up = true;
}

void start_count_up(trigger_t *pin, long millis)
{
	init_count(pin);
	pin->millis = millis;
	pin->count_up = true;
}

void start_count_down(trigger_t *pin, long millis)
{
	init_count(pin);
	pin->millis = millis;
	pin->count_up = false;
}

bool timer_is_counting_up(trigger_t *pin)
{
	return pin->count_up;
}

bool timer_is_started(trigger_t *pin)
{
	return (pin->millis > 0);
}

bool timer_is_type(trigger_t *pin, short type)
{
	return (pin->count[type] >pin->count[!type]);
}

void add_timer_count(trigger_t *pin, short type)
{
	pin->count[type]+= 1;
}

long time_since_start(trigger_t *pin, long millis)
{
	return millis - abs(pin->millis);
}

trigger_t pin_trigger[NB_OF_PIN];

bool pin_has_changed(long current_millis, short pin_id, short trigger)
{
	bool changes = false;
	uint8_t current_pin = readPin(pin_id);  

	// if we were waiting for an event
	if (! timer_is_started(&pin_trigger[pin_id]))
	{
		// if the event happened
		if (current_pin == trigger)
		{    
			// start the debouncer
			start_count_up(&pin_trigger[pin_id], current_millis);
		}
	}
	else
	{
		// we add the current type (HIGH,LOW) to the counter to average later
		add_timer_count(&pin_trigger[pin_id], current_pin);

		// if we are looking for a rising edge
		if(timer_is_counting_up(&pin_trigger[pin_id]))
		{
			// if we are past the debounce period
			if (time_since_start(&pin_trigger[pin_id], current_millis) > DEBOUNCE_ON)
			{
				// average the pin, if we see high more often than its a key press
				changes = timer_is_type(&pin_trigger[pin_id], trigger);
				// send the key press and then start looking for a falling edge
				start_count_down(&pin_trigger[pin_id], current_millis);
			}
		}
		else
		{
			// if we are past the debounce period
			if (time_since_start(&pin_trigger[pin_id], current_millis) > DEBOUNCE_OFF)
			{
				// we wait for a falling edge and prevent quick double press
				if(! timer_is_type(&pin_trigger[pin_id], trigger))
				{
					// we reset the timer
					stop_timer(&pin_trigger[pin_id]);
				}
				init_count(&pin_trigger[pin_id]);

			}
		}
	}
	
	if(changes)
	{
		Serial.print("Key<");
		Serial.print(pin_id);
		Serial.println("> pressed");
	}
	return changes;
}

void add_minute(my_time_t *timer)
{
	timer->minutes += 1;
	if(timer->minutes == 60)
	{
		timer->minutes = 0;
		timer->hours += 1;
	}
}

void count_down(long *q)
{
	if(q)
	{
		*q -= 1;
		if(*q < 0)
		{
			if(q+1)
			{
				*q = 59;
				count_down(q+1);
			}
			else
			{
				*q = 0;
			}
		}
	}
}

long previous_millis = 0;
void update_time(my_time_t *timer, long current_millis)
{
	// we could hit this time slice twice since we run fast
	// rounding to the closest second, is sufficient
	if( current_millis != previous_millis
	&& (current_millis % 1000) == 0)
	{
		previous_millis = current_millis; 
		count_down((long *)timer);
	}
}

void set_time(my_time_t *timer, long hour, long minute, long second)
{
	timer->hours = hour;
	timer->minutes = minute;
	timer->seconds = second;
}

bool times_up(my_time_t *timer)
{
	return (timer->seconds <= 0 && timer->minutes <= 0 && timer->hours <= 0);
}

/*
 * This function converts the time in minutes to an ascii value that can be printed to the LCD
 */

long mem_feed_time = -1;
long feed_time = 0;

my_time_t glb_timer;
void display_time(my_time_t *timer)
{
	char out_time[LCD_BUFFER_LEN] = { 0 };
	sprintf(out_time, "   TIME: %02ld:%02ld:%02ld   ", timer->hours, timer->minutes, timer->seconds);
	lcd_print(out_time);
}

void setup()
{

	Wire.begin();

	setPin(MCU_SRAM_CS, OUTPUT);
	setPin(MCU_MISO, INPUT);
	setPin(MCU_SCK, OUTPUT);
	setPin(MCU_MOSI, OUTPUT);
	setPin(MCU_SCL, OUTPUT);
	setPin(MCU_SDA, OUTPUT);
	setPin(B_feed, INPUT);
	setPin(B_auto_feed, INPUT);
	setPin(B_fill, INPUT);
	setPin(B_Inc, INPUT);
	setPin(B_Dec, INPUT);
	// setPin(B_Emergency, INPUT);
	// setPin(empty_switch, INPUT);
	// setPin(full_switch, INPUT);
	setPin(LED_auto_feed, OUTPUT);
	setPin(LED_Inc, OUTPUT);
	setPin(LED_Dec, OUTPUT);
	setPin(LED_feed, OUTPUT);
	setPin(LED_fill, OUTPUT);
	// setPin(Buzzer, OUTPUT);
	// Set the default pin value

	writePin(MCU_SCL, LOW);
	writePin(MCU_SDA, LOW);
	writePin(B_feed, HIGH);
	writePin(B_auto_feed, HIGH);
	writePin(B_fill, HIGH);
	writePin(B_Inc, HIGH);
	writePin(B_Dec, HIGH);
	// writePin(B_Emergency, HIGH);
	// writePin(empty_switch, HIGH);
	// writePin(full_switch, HIGH);
	writePin(LED_auto_feed, LOW);
	writePin(LED_Inc, LOW);
	writePin(LED_Dec, LOW);
	writePin(LED_feed, LOW);
	writePin(LED_fill, LOW);
	// writePin(Buzzer, LOW);

	delay(10);

	Serial.begin(9600);

	lcd_print("Initializing");

	if(memory.SRAMBytes > 0) 
	{
		memory.get(MEM_ADDR, mem_feed_time);
		/* init the memory for the first time */
		if ( mem_feed_time < 0 || mem_feed_time > 150 ) 
		{  
				memory.put(MEM_ADDR, 0);
				mem_feed_time = 0;
		}
		Serial.print("Loading feed time from memory <");
		Serial.print(mem_feed_time);
		Serial.println(">");
		feed_time = mem_feed_time;
	}
	else
	{
		Serial.print("- Error detecting SPI memory.\n");
		Serial.print(" - Either an incorrect SPI pin was specified,\n- or the ");
		Serial.print("Microchip memory has been wired incorrectly,\n- or it is ");
		Serial.print("not a Microchip memory.\nSupported memories are:\n23x640");
		Serial.print(", 23x256, 23x512, 23xx1024, 23LCV512 & 23LCV1024");
		feed_time = 0;
	}

	set_time(&glb_timer, feed_time/60, feed_time%60, 0);
	// reset all the timers
	for(int i=0; i < NB_OF_PIN; i++)
	{
	  stop_timer(&pin_trigger[i]);
	}
	man_feed = false;
	auto_feed = false;
	fill = false;

	delay(1000);
}

long last_time = 0;
void loop() 
{
	long current_millis = millis();
	
	if(times_up(&glb_timer))
	{
		auto_feed = false;
		man_feed = false;
	}
	display_time(&glb_timer);
	
	if ((current_millis - last_time) > PIN_REFRESH_RATE)
	{
		last_time = current_millis;
		mcp_refresh();

		// /* Check your limit switch */
		// if( pin_has_changed(current_millis, empty_switch, HIGH) )
		// {

		// }

		// if( pin_has_changed(current_millis, full_switch, HIGH) )
		// {

		// }

		// /* Check the emergency button */
		// if( pin_has_changed(current_millis, B_Emergency, HIGH) )
		// {

		// }

		/* CHECK_TIMER_BUTTONS */
		if( pin_has_changed(current_millis, B_Inc, LOW) )
		{
			if ( ! man_feed && ! auto_feed && ! fill )
			{
				feed_time += 1;
				writePin(LED_Inc, HIGH);
			}
		}


		if( pin_has_changed(current_millis, B_Dec, LOW) )
		{
			if ( ! man_feed && ! auto_feed && ! fill )
			{
				feed_time -= 1;
			}
		}

		/* CHECK_FEED_BUTTON */
		if(pin_has_changed(current_millis, B_feed, LOW))     
		{
			if ( ! auto_feed && ! fill )
			{
				man_feed = true;
			}
		}
 
		// B_auto_feed has not yet been defined because we haven't decided how to wire extra buttons that were not in the schematic
		if(pin_has_changed(current_millis, B_auto_feed, LOW))   
		{
			if ( ! man_feed && ! fill )
			{
				auto_feed = true;
			}
		}


		/* CHECK_FILL_BUTTON */
		if(pin_has_changed(current_millis, B_fill, LOW))
		{
			if ( ! auto_feed && ! man_feed)
			{
				fill = true;
			}
		}
		
	}

	// if( man_feed || auto_feed )
	// {

	// 	/*
	// 	 *        Main operation happens here:
	// 	 *        Motor driving, checking stop switches, down-count timer,
	// 	 *        Buzzer
	// 	 */

	// 	update_time(&glb_timer, current_millis);
		
	// 	/* store in memeory if necessary */
	// 	if ( mem_feed_time >= 0 && feed_time != mem_feed_time ) 
	// 	{  
	// 			Serial.print("Storing feed time to memory <");
	// 			Serial.print(feed_time);
	// 			Serial.println(">");
	// 			memory.put(MEM_ADDR, feed_time);
	// 			/* sanity check that we wrote */
	// 			memory.get(MEM_ADDR, mem_feed_time);
	// 			if ( mem_feed_time != feed_time ) 
	// 			{  
	// 					Serial.print("Unable to store the feed time to memory");
	// 			}

	// 			mem_feed_time = -1;
	// 	}
	// }
	// else
	// {
		/* if it is not running, you can still change the time */
		set_time(&glb_timer, feed_time / 60, feed_time % 60, 0);
	// }
	
}