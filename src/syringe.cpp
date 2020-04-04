// needs to be first to grab the defines
#include "syringe.h"

#include <Arduino.h>
#include <Wire.h>
#include <stdio.h>
#include <stdlib.h>

#include "debouncedPin.h"
#include "MicrochipSRAM.h"
#include "pin_wrapper.h"
#include "i2c_lcd.h"
#include "i2c.h"
#include "message.h"

#define SERIAL_BAUD_RATE 9600
#define MEM_IS_SET UINT16_MAX
#define MEM_ADDR 0x100

typedef struct my_time_t_t
{
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
} my_time_t;
static MicrochipSRAM memory(MCU_SRAM_CS); // Instantiate the class to the given pin

static debouncedPin increment_button;
static debouncedPin decrement_button;
static debouncedPin autofeed_button;
static debouncedPin manfeed_button;
static debouncedPin manfill_button;

uint16_t mem_feed_time = -1;
uint16_t feed_time = 0;
my_time_t glb_timer;

/**
 * we check that either case ( Normally off is on or Normally on if off )
 */
bool SR_latch_state_is(uint16_t current_millis, uint8_t pin_id_NC, uint8_t pin_id_NO, uint8_t trigger)
{
	uint8_t current_pin_NC = readPin(pin_id_NC);  
	uint8_t current_pin_NO = readPin(pin_id_NO); 

	return (current_pin_NC != trigger && current_pin_NO == trigger);
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

void count_down(uint8_t *q)
{
	if(q)
	{
		*q -= 1;
		if(*q < 0 || *q > 59)
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

void update_time(my_time_t *timer, uint16_t current_millis)
{
	static uint16_t previous_millis = 0;

	// we could hit this time slice twice since we run fast
	// rounding to the closest second, is sufficient
	if( current_millis != previous_millis
	&& (current_millis % 1000) == 0)
	{
		previous_millis = current_millis; 
		count_down((uint8_t *)timer);
	}
}

void set_time(my_time_t *timer, uint8_t hour, uint8_t minute, uint8_t second)
{
	timer->hours = hour;
	timer->minutes = minute;
	timer->seconds = second;
}

bool times_up(my_time_t *timer)
{
	return (timer->seconds <= 0 && timer->minutes <= 0 && timer->hours <= 0);
}

#ifdef MAIN
void setup()
{
	lcd_printf("Initializing");
	
	serial_printf("Initializing\n");

	serial_printf("== Pin\n");
	setPin(MCU_SRAM_CS, OUTPUT);
	setPin(MCU_MISO, INPUT);
	setPin(MCU_SCK, OUTPUT);
	setPin(MCU_MOSI, OUTPUT);

	setPin(LED_auto_feed, OUTPUT);
	setPin(LED_Inc, OUTPUT);
	setPin(LED_Dec, OUTPUT);
	setPin(LED_feed, OUTPUT);
	setPin(LED_fill, OUTPUT);
	setPin(Buzzer, OUTPUT);

	// Set the default pin value
	increment_button.set_pin(B_Inc);
	decrement_button.set_pin(B_Dec);
	autofeed_button.set_pin(B_auto_feed);
	manfeed_button.set_pin(B_feed);
	manfill_button.set_pin(B_fill);
	emergency_button.set_pin(B_Emergency_NC, B_Emergency_NO);
	full_switch.set_pin(empty_switch_NC, empty_switch_NO);
	empty_switch.set_pin(full_switch_NC, full_switch_NO);

	delay(10);


	serial_printf("== Spi\n");
	if(memory.SRAMBytes > 0) 
	{
		memory.get(MEM_ADDR, mem_feed_time);
		/* init the memory for the first time */
		if (mem_feed_time < MIN_FEED_MINUTE 
		|| mem_feed_time > MAX_FEED_MINUTE ) 
		{  
			memory.put(MEM_ADDR, MIN_FEED_MINUTE);
			mem_feed_time = MIN_FEED_MINUTE;
		}
		serial_printf(" - Loading feed time from memory <%hu>\n", mem_feed_time);
		feed_time = mem_feed_time;
	}
	else
	{
		serial_printf("- Error detecting SPI memory ==\n");
		feed_time = 0;
	}
}

uint8_t compute_feed_time(uint8_t new_feedtime)
{
	if( new_feedtime > MAX_FEED_MINUTE )
	{
		new_feedtime = MIN_FEED_MINUTE;
	}
	else if( new_feedtime < MIN_FEED_MINUTE  || new_feedtime == MEM_IS_SET )
	{
		new_feedtime = MAX_FEED_MINUTE;
	}
	return new_feedtime;
}

enum states_e { 
	/* refresh */
	RESTARTING,
	/* can read most IO */
	IDLE, 
	/* transistion phase, lock most IO */
	AUTO_FEED, /* empty -> IDLE_EMPTY
	MANUAL_FEED,
	MANUAL_FILL,
	/* Stop the world, something gone baaaad */
	EMERGENCY_STOP
};

static const char *states[] = {
	"Restarting",
	"Idle",
	"Automatic Feeding",
	"Manual Feeding",
	"Manual Refill",
	"== EMERGENCY STOP =="
};

void loop() 
{
	_FN_START
	static states_e current_state = RESTARTING;
	static bool has_content = false;

	uint16_t current_millis = millis();
	refreshPin();
	refresh_debounced_pins();

	/* always check this switch */
	if ( full_switch.is_high() )
	{
		has_content = true;
	}

	// always check this switch
	if( empty_switch.is_high() )
	{
		has_content = false;
	}

	// always check this switch
	if( emergency_button.is_high() )
	{
		switch(current_state)
		{
			case IDLE:
			{
				if( increment_button.is_falling_edge() )
				{
					feed_time = compute_feed_time(feed_time + 1);
				}

				if( decrement_button.is_falling_edge() )
				{
					feed_time = compute_feed_time(feed_time - 1);
				}

				// there is precedence here, autofeed takes priority
				if( autofeed_button.is_falling_edge() )
				{
					current_state = AUTO_FEED;
				}
				else if ( manfeed_button.is_falling_edge() )
				{
					current_state = MANUAL_FEED;
				}
				else if ( manfill_button.is_falling_edge() )
				{
					current_state = MANUAL_FILL;
				}

				/* if it is not running, you can change the time */
				set_time(&glb_timer, feed_time / 60, feed_time % 60, 0);

				break;
			}
			case MANUAL_FEED:
			case AUTO_FEED:
			{
				/* store in memory if necessary */
				if ( mem_feed_time != MEM_IS_SET && feed_time != mem_feed_time ) 
				{  
					serial_printf("Storing feed time to memory <%hu>: ", feed_time);
					memory.put(MEM_ADDR, feed_time);
					/* sanity check that we wrote */
					memory.get(MEM_ADDR, mem_feed_time);
					if ( mem_feed_time != feed_time ) 
					{  
						serial_printf("Error\n");
					}
					else
					{
						serial_printf("Done\n");
					}

					mem_feed_time = MEM_IS_SET;
				}

				// check that the button is still pressed
				if ( current_state == MANUAL_FEED && manfeed_button.is_high() )
				{
					current_state = IDLE;
				}
				else if( times_up(&glb_timer) )
				{
					current_state = RESTARTING;
				}
				else if(! has_content)
				{
					current_state = RESTARTING;
				}
				else
				{
					/* drive the motor */
				}

				/* count down the time */
				update_time(&glb_timer, current_millis);

				break;
			}
			case MANUAL_FILL:
			{
				if(full_switch.is_high())
				{
					current_state = IDLE;
				}
				else if( manfill_button.is_high() )
				{
					current_state = IDLE;
				}
				else
				{
					/* drive the motor */
				}
			}
			default:
			{
				tone(Buzzer, 2250, 1000);
				current_state = IDLE;
				break;
			}
		}
	}


	/**
	 * update the Displayables
	 */
	writePin(LED_Inc,increment_button.is_low());
	writePin(LED_Dec,decrement_button.is_low());
	writePin(LED_fill,manfill_button.is_low());
	writePin(LED_feed,manfeed_button.is_low());
	writePin(LED_auto_feed,auto_feed);
	lcd_printf("%s\nTIME(h:m:s): %02hu:%02hu:%02hu", 
		states[current_state], glb_timer.hours, glb_timer.minutes, glb_timer.seconds);

	_FN_END
}
#else
#ifdef MCP_LED_TEST
void setup()
{
	_FN_START

	serial_printf("Initializing\n");
	setPin(LED_auto_feed, OUTPUT);
	setPin(LED_Inc, OUTPUT);
	setPin(LED_Dec, OUTPUT);
	setPin(LED_feed, OUTPUT);
	setPin(LED_fill, OUTPUT);
	// setPin(Buzzer, OUTPUT);
	serial_printf("Done\n");

	delay(500);
	_FN_END

}

uint8_t set_to = LOW;
void loop()
{
	_FN_START
	writePin(LED_auto_feed, set_to);
	writePin(LED_Inc, set_to);
	writePin(LED_Dec, set_to);
	writePin(LED_feed, set_to);
	writePin(LED_fill, set_to);
	refreshPin();
	set_to = !set_to;
	delay(500);
	_FN_END
}
#else
#ifdef MCP_SWITCH_TEST
void setup()
{
	_FN_START

	serial_printf("Initializing\n");
	setPin(LED_auto_feed, OUTPUT);
	setPin(LED_Inc, OUTPUT);
	setPin(LED_Dec, OUTPUT);
	setPin(LED_feed, OUTPUT);
	setPin(LED_fill, OUTPUT);

	setPin(B_auto_feed, INPUT);
	setPin(B_Inc, INPUT);
	setPin(B_Dec, INPUT);
	setPin(B_feed, INPUT);
	setPin(B_fill, INPUT);

	serial_printf("Done\n");

	delay(500);
	_FN_END

}
void loop()
{
	_FN_START

	refreshPin();
	writePin(LED_auto_feed, !readPin(B_auto_feed));
	writePin(LED_Inc, !readPin(B_Inc));
	writePin(LED_Dec, !readPin(B_Dec));
	writePin(LED_feed, !readPin(B_feed));
	writePin(LED_fill, !readPin(B_fill));

	delay(2);
	_FN_END
}
#else
#ifdef LCD_TEST
static uint8_t current_value = 0;
void setup()
{
	_FN_START
	_FN_END
}
void loop()
{
	_FN_START
	lcd_printf("Hello World\ncount: %hhu", current_value);
	delay(1000);
	current_value += 1;
	_FN_END
}
#else
#ifdef LCD_W_BUTTON_TEST
void setup()
{
	_FN_START

	serial_printf("Initializing\n");
	setPin(LED_auto_feed, OUTPUT);
	setPin(LED_Inc, OUTPUT);
	setPin(LED_Dec, OUTPUT);
	setPin(LED_feed, OUTPUT);
	setPin(LED_fill, OUTPUT);

	setPin(B_auto_feed, INPUT);
	setPin(B_Inc, INPUT);
	setPin(B_Dec, INPUT);
	setPin(B_feed, INPUT);
	setPin(B_fill, INPUT);

	serial_printf("Done\n");

	delay(500);
	_FN_END

}
void loop()
{
	_FN_START

	refreshPin();

	char status[] = {
		'0' + readPin(B_auto_feed), ' ',
		'0' + readPin(B_Inc), ' ',
		'0' + readPin(B_Dec), ' ',
		'0' + readPin(B_feed), ' ',
		'0' + readPin(B_fill), ' '
	};
	
	writePin(LED_auto_feed, !readPin(B_auto_feed));
	writePin(LED_Inc, !readPin(B_Inc));
	writePin(LED_Dec, !readPin(B_Dec));
	writePin(LED_feed, !readPin(B_feed));
	writePin(LED_fill, !readPin(B_fill));

	lcd_printf("GPIO [msb:lsb]\n%s" , status);

	delay(2);
	_FN_END
}
#endif
#endif
#endif
#endif
#endif
