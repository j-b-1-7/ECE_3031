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
#define MEM_IS_SET INT16_MAX
#define MEM_ADDR 0x100

/**
 * converting the spec we need (0.750 N/m) to oz-in = 106 oz-in
 * the motor is rated at 16.7rpm@0oz-in, 0rpm@774oz-in
 * -> linear rpm/oz-in = (-16.7 / 774) −0.021576227
 * The max speed is 16.7 + ( −0.021576227 * 106 ) (as per spec) = 14.412919938
 * 
 * *** MOTOR MAX SPEED: 14.4129 RPM
 * ****************************
 */
#define MOTOR_MAX_SPEED 14.413
 /**
 * The gear ratio on the machine itself is:
 * 	thread: 12 TPI
 * 	nut is 28 teeth gear
 * 	Motor Gear is 37 teeths
 * 
 * knowing Sa *Ta = Sb * Tb
 * 	(14.4129 * 37) / 28 = 19.0456 rotation on the nut
 * 
 * 1 inch / 12 rotation
 * 19.0456 * (1/12) = 1.5871 inch per minute.
 * 
 * with a travel of ~4.5 inch 
 * that is ~ 3 minutes to do a full travel.
 * ----------------------------------------
 * 	This seems too slow :( based on our calculation
 * 
 * *************************************************************
 *  * We provide an initial Travel in rotations
 * we measured 4.5 inches of travel on the machine,
 * To get the total number of rotation from travel
 * (4.5 * 12(TPI) * 28(Tb)) / 37(Ta) = rotations
 * with: (4.5 * ( ( 12 * 28 ) / 37 ) = 4.5 * 9.081081 = 40.8648645
 */
#define INITIAL_ROTATION_NECESSARY_ESTIMATE 40.865

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
static debouncedPin emergency_button;
static debouncedPin full_switch;
static debouncedPin empty_switch;

/**
 * this will store the total travel in rotations that is exactly necessary once one full travel
 * is achieved on the system
 */
uint16_t rotations = 0;
uint16_t total_rotation_needed = INITIAL_ROTATION_NECESSARY_ESTIMATE;

int16_t mem_feed_time = -1;
int16_t feed_time = 0;
my_time_t glb_timer;

/**
 * expects:
 *  feed time; if ( 0 ) -> full throtle else adjust speed to feed time
 *  foward; if (true): feed else: fill
 */
static int16_t motor_speed = 0;
static void forward_motor(uint8_t feedfor)
{
	uint8_t new_motor_speed = 255;
	if (feedfor != 0)
	{
		double necessary_rpm = total_rotation_needed / (double)feedfor;
		double ratio = necessary_rpm / MOTOR_MAX_SPEED;
		if (ratio > 1)
		{
			ratio = 1;
		}
		new_motor_speed = 255 * ratio;
		if(new_motor_speed == 0)
		{
			new_motor_speed = 1;
		}
	}

	if( motor_speed != new_motor_speed )
	{
		motor_speed = new_motor_speed;
		analogWrite(MOTOR_PWM,motor_speed);
		digitalWrite(MOTOR_IO, 0);
	}
}

static void stop_motor()
{
	if( motor_speed != 0 )
	{
		motor_speed = 0;
	}
	analogWrite(MOTOR_PWM,motor_speed);
	digitalWrite(MOTOR_IO, 0);
}

static void reverse_motor()
{
	if( motor_speed != -1 )
	{
		motor_speed = -1;
	}
	/* reverse full chuch ahead*/
	analogWrite(MOTOR_PWM,0);
	digitalWrite(MOTOR_IO, 1);
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

	serial_printf("== OUTPUT Pin\n");
	/* SPI */
	setPin(MCU_SRAM_CS, OUTPUT);
	setPin(MCU_MISO, INPUT);
	setPin(MCU_SCK, OUTPUT);
	setPin(MCU_MOSI, OUTPUT);
	/* LED */
	setPin(LED_auto_feed, OUTPUT);
	setPin(LED_Inc, OUTPUT);
	setPin(LED_Dec, OUTPUT);
	setPin(LED_feed, OUTPUT);
	setPin(LED_fill, OUTPUT);
	/* Buzzer */
	setPin(Buzzer, OUTPUT);
	/* Motor */
	setPin(MOTOR_IO, OUTPUT);
	setPin(MOTOR_PWM, OUTPUT);

	serial_printf("== INPUT Pin\n");
	/* single throw switches */
	increment_button.set_pin(B_Inc);
	decrement_button.set_pin(B_Dec);
	autofeed_button.set_pin(B_auto_feed);
	manfeed_button.set_pin(B_feed);
	manfill_button.set_pin(B_fill);
	/* double throw switches */
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

int16_t compute_feed_time(int16_t new_feedtime)
{
	if( new_feedtime > MAX_FEED_MINUTE )
	{
		new_feedtime = MIN_FEED_MINUTE;
	}
	else if( new_feedtime < MIN_FEED_MINUTE  || new_feedtime == MEM_IS_SET )
	{
		new_feedtime = MAX_FEED_MINUTE;
	}
	return (uint8_t)new_feedtime;
}

enum states_e { 
	/* refresh */
	RESTARTING,
	/* can read most IO */
	IDLE, 
	/* transistion phase, lock most IO */
	START_AUTO_FEED,
	START_MANUAL_FEED,
	START_MANUAL_FILL,
	AUTO_FEED,
	MANUAL_FEED,
	MANUAL_FILL,
	/* Stop the world, something gone baaaad */
	EMERGENCY_STOP
};

enum content_states_e { 
	EMPTY,
	NOT_EMPTY,
	FULL
};

static const char *states[] = {
	"Restarting",
	"Idle",
	"Automatic Feeding",
	"Manual Feeding",
	"Manual Refill",
	"Automatic Feeding",
	"Manual Feeding",
	"Manual Refill",
	"== EMERGENCY STOP =="
};

void loop() 
{
	_FN_START
	static states_e current_state = RESTARTING;
	static content_states_e current_content_status = EMPTY;

	uint16_t current_millis = millis();
	refreshPin();
	refresh_debounced_pins();

	/* always check this switch */
	if ( full_switch.is_high() )
	{
		current_content_status = FULL;
		stop_motor();
	}

	// always check this switch
	if( empty_switch.is_high() )
	{
		current_content_status = EMPTY;
		stop_motor();
	}

	// always check this switch
	if( emergency_button.is_high() )
	{
		stop_motor();
	}
	else
	{
		switch(current_state)
		{
			case IDLE:
			{
				if( increment_button.is_falling_edge() )
				{
					feed_time = compute_feed_time((int)feed_time + 1);
				}

				if( decrement_button.is_falling_edge() )
				{
					feed_time = compute_feed_time((int)feed_time - 1);
				}

				if( current_content_status != EMPTY )
				{
					if( autofeed_button.is_falling_edge() )
					{
						current_state = AUTO_FEED;
					}
					
					if ( manfeed_button.is_falling_edge() )
					{
						current_state = MANUAL_FEED;
					}
				}

				if( current_content_status != FULL )
				{
					if ( manfill_button.is_falling_edge() )
					{
						current_state = MANUAL_FILL;
					}
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
				else if(current_content_status == EMPTY)
				{
					current_state = RESTARTING;
				}
				else
				{
					/* drive the motor */
					forward_motor(feed_time);
					/* hall effect can give better positionning */
					current_content_status = NOT_EMPTY;
					/* count down the time */
					update_time(&glb_timer, current_millis);
				}

				break;
			}
			case START_MANUAL_FILL:
			{
				if(full_switch.is_high())
				{
					current_state = IDLE;
				}
				else if( current_content_status == FULL )
				{
					current_state = IDLE;
				}
				else
				{
					/* drive the motor */
					reverse_motor();
					/* hall effect can give better positionning */
					current_content_status = NOT_EMPTY;
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
	writePin(LED_auto_feed,current_state == AUTO_FEED);
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
