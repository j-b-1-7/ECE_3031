#include <Arduino.h>
#include <Wire.h>
#include <stdio.h>
#include <stdlib.h>

#include "MicrochipSRAM.h"                // Include the library
#include "my_mcp_library.h"
#include "my_I2C_LCD.h"

#define NB_OF_PIN 14

#define PIN_REFRESH_RATE 15
#define SHORT_DEBOUNCE_COUNT 5
#define LONG_DEBOUNCE_COUNT 15
#define DEBOUNCE_ON (PIN_REFRESH_RATE*LONG_DEBOUNCE_COUNT)
#define DEBOUNCE_OFF (PIN_REFRESH_RATE*SHORT_DEBOUNCE_COUNT)

#define MCU_SDA A4
#define MCU_SCL A5

#define MCU_SRAM_CS 10
#define MCU_MISO 12
#define MCU_SCK 13
#define MCU_MOSI 11

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

#define B_auto_feed 0
#define B_Inc 1
#define B_Dec 2
#define B_feed 3
#define B_fill 4
#define B_Emergency 5
#define empty_switch 6
#define full_switch 7

#define LED_auto_feed 0 
#define LED_Inc 1
#define LED_Dec 2
#define LED_feed 3
#define LED_fill 4

#define Buzzer 5

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
	uint8_t current_pin = mcp_read(A, pin_id);  

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

	mcp_set(A, B_feed, INPUT_PIN);
	mcp_set(A, B_auto_feed, INPUT_PIN);
	mcp_set(A, B_fill, INPUT_PIN);
	mcp_set(A, B_Inc, INPUT_PIN);
	mcp_set(A, B_Dec, INPUT_PIN);
	//mcp_set(A, B_Emergency, INPUT_PIN);
	//mcp_set(A, empty_switch, INPUT_PIN);
	//mcp_set(A, full_switch, INPUT_PIN);

	// Set the default pin value

	mcp_write(A, B_feed, HIGH);
	mcp_write(A, B_auto_feed, HIGH);
	mcp_write(A, B_fill, HIGH);
	mcp_write(A, B_Inc, HIGH);
	mcp_write(A, B_Dec, HIGH);

	mcp_set(B, LED_auto_feed, OUTPUT_PIN);
	mcp_set(B, LED_Inc, OUTPUT_PIN);
	mcp_set(B, LED_Dec, OUTPUT_PIN);
	mcp_set(B, LED_feed, OUTPUT_PIN);
	mcp_set(B, LED_fill, OUTPUT_PIN);
	// mcp_set(B, Buzzer, OUTPUT_PIN);


	mcp_write(B, LED_auto_feed, LOW);
	mcp_write(B, LED_Inc, LOW);
	mcp_write(B, LED_Dec, LOW);
	mcp_write(B, LED_feed, LOW);
	mcp_write(B, LED_fill, LOW);

	pinMode(MCU_SRAM_CS, OUTPUT);
	pinMode(MCU_MISO, INPUT);
	pinMode(MCU_SCK, OUTPUT);
	pinMode(MCU_MOSI, OUTPUT);
	
	pinMode(MCU_SCL, OUTPUT);
	pinMode(MCU_SDA, OUTPUT);
	digitalWrite(MCU_SCL, LOW);
	digitalWrite(MCU_SDA, LOW);
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
	// // stop all the timers
	// for(int i=0; i < NB_OF_PIN; i++)
	// {
	//   stop_timer(&pin_trigger[i]);
	// }
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
				mcp_write(B, LED_Inc, HIGH);
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

	if( man_feed || auto_feed )
	{

		/*
		 *        Main operation happens here:
		 *        Motor driving, checking stop switches, down-count timer,
		 *        Buzzer
		 */

		update_time(&glb_timer, current_millis);
		
		/* store in memeory if necessary */
		if ( mem_feed_time >= 0 && feed_time != mem_feed_time ) 
		{  
				Serial.print("Storing feed time to memory <");
				Serial.print(feed_time);
				Serial.println(">");
				memory.put(MEM_ADDR, feed_time);
				/* sanity check that we wrote */
				memory.get(MEM_ADDR, mem_feed_time);
				if ( mem_feed_time != feed_time ) 
				{  
						Serial.print("Unable to store the feed time to memory");
				}

				mem_feed_time = -1;
		}
	}
	else
	{
		/* if it is not running, you can still change the time */
		set_time(&glb_timer, feed_time / 60, feed_time % 60, 0);
	}
	
}