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
#define MOTOR_MAX_SPEED 14
/** 
 * (255(MaxStep) << 8) 0xff00 / MOTOR_MAX_SPEED) = 4529.2759
 * must be shifted back right by 8
 */
#define MOTOR_SPEED_STEP 4529
#define ESTIMATE_SPEED(a) ( ( a * MOTOR_SPEED_STEP ) >> 8 )
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
 * *************************************************************
 *  * We provide an initial Travel in rotations
 * we measured 3.75 inches of travel on the machine,
 * To get the total number of rotation from travel
 * (3.75 * 12(TPI) * 28(Tb)) / 37(Ta) = rotations
 * with: (4.5 * ( ( 12 * 28 ) / 37 ) = 3.75 * 9.081081 = 34.05405
 */
// manually tweaked.. this seems closer
#define INITIAL_ROTATION_NECESSARY_ESTIMATE 29

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

enum direction_e
{
	FORWARD,
	REVERSE
};

typedef struct motor_t_t
{
	direction_e  direction;
	uint8_t speed;
}motor_t;

motor_t motor = {FORWARD, 0};

static uint8_t compute_motor_speed(uint16_t timespan)
{
	/* assume full speed */
	uint8_t new_motor_speed = UINT8_MAX;
	if (timespan != 0)
	{
		uint16_t necessary_rpm = total_rotation_needed / timespan;
		if (necessary_rpm > MOTOR_MAX_SPEED)
		{
			necessary_rpm = MOTOR_MAX_SPEED;
		}
		
		new_motor_speed = ESTIMATE_SPEED(necessary_rpm);
	}

	return new_motor_speed;
}

static void drive(uint16_t timespan, direction_e direction)
{
	uint8_t new_motor_speed = compute_motor_speed(timespan);
	if( motor.speed != new_motor_speed 
	|| motor.direction != direction)
	{

		motor.speed = new_motor_speed;
		motor.direction = direction;
		digitalWrite(MOTOR_ENABLE, HIGH);
		if(motor.direction == FORWARD)
		{
			analogWrite(MOTOR_IN1, motor.speed);
			analogWrite(MOTOR_IN2, LOW);
		}
		else
		{
			analogWrite(MOTOR_IN1, LOW);
			analogWrite(MOTOR_IN2, motor.speed);
		}
		debug_printf("Setting motor to %s to %d\n",(motor.direction==REVERSE)? "REVERSE": "FOWARD", motor.speed);
	}
}

static void stop_if(direction_e direction)
{
	if( motor.direction == direction )
	{
		motor.speed = 0;
		digitalWrite(MOTOR_ENABLE, LOW);
	}
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
	static uint16_t previous_seconds = 0;
	uint16_t seconds_now = current_millis/1000;
	
	// we could hit this time slice twice since we run fast
	// rounding to the closest second, is sufficient
	if( seconds_now != previous_seconds )
	{
		previous_seconds = seconds_now; 
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
	/* Motor */
	setPin(MOTOR_IN1, OUTPUT);
	setPin(MOTOR_IN2, OUTPUT);
	setPin(MOTOR_ENABLE, OUTPUT);

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

	serial_printf("== INPUT Pin\n");
	/* single throw switches */
	increment_button.set_pin(B_Inc);
	decrement_button.set_pin(B_Dec);
	autofeed_button.set_pin(B_auto_feed);
	manfeed_button.set_pin(B_feed);
	manfill_button.set_pin(B_fill);
	/* double throw switches */
	emergency_button.set_pin(B_Emergency_NC, B_Emergency_NO);
	empty_switch.set_pin(empty_switch_NC, empty_switch_NO);
	full_switch.set_pin(full_switch_NC, full_switch_NO);

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
	serial_printf("READY\n");

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
	/* cannot change time */
	PAUSED,
	/* transistion phase, lock most IO */
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
	"Pause",
	"Automatic Feeding",
	"Manual Feeding",
	"Manual Refill",
	"== EMERGENCY STOP =="
};

static const char *syringe_states[] = {
	"Empty",
	"--",
	"Full",
};

void loop() 
{
	_FN_START
	static states_e previous_state = RESTARTING;
	static states_e current_state = RESTARTING;
	static content_states_e current_content_status = EMPTY;

	uint16_t current_millis = millis();

	refreshPin();

	increment_button.refresh();
	decrement_button.refresh();
	autofeed_button.refresh();
	manfeed_button.refresh();
	manfill_button.refresh();
	/* double throw switches */
	emergency_button.refresh();
	full_switch.refresh();
	empty_switch.refresh();

	if ( empty_switch.is_low() )
	{
		if(current_state != IDLE)
		{
			tone(Buzzer, 2250, 250);
		}
		current_content_status = EMPTY;
		stop_if(REVERSE);
	}
	else if( full_switch.is_low() )
	{
		if(current_state != IDLE)
		{
			tone(Buzzer, 2250, 250);
		}
		current_content_status = FULL;
		stop_if(FORWARD);
	}
	else
	{
		current_content_status = NOT_EMPTY;
	}
								
	if( emergency_button.is_low() )
	{
		if(current_state != EMERGENCY_STOP)
		{
			previous_state = current_state;
			current_state = EMERGENCY_STOP;
			stop_if(REVERSE);
			stop_if(FORWARD);	
		}
	}
	else if(current_state == EMERGENCY_STOP)
	{
		current_state = previous_state;
	}

	switch(current_state)
	{
		case RESTARTING:
		{
			tone(Buzzer, 2250, 1000);
			current_state = IDLE;
			break;
		}
		case EMERGENCY_STOP:
		{
			// nothing to do ?
			break;
		}
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

			if( current_content_status == FULL )
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
		case PAUSED:
		{
			if( autofeed_button.is_falling_edge() )
			{
				current_state = AUTO_FEED;
			}
			
			if ( manfeed_button.is_falling_edge() )
			{
				current_state = MANUAL_FEED;
			}
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
				current_state = PAUSED;
				stop_if(REVERSE);
			}
			else if( ! times_up(&glb_timer) && current_content_status != EMPTY )
			{
				/* drive the motor */
				drive(feed_time, REVERSE);
				/* count down the time */
				update_time(&glb_timer, current_millis);
			}
			else
			{
				current_state = RESTARTING;
			}

			break;
		}
		case MANUAL_FILL:
		{
			if( manfill_button.is_low() && current_content_status != FULL )
			{
				/* drive the motor */
				drive(0, FORWARD);
			}
			else
			{
				current_state = IDLE;
				stop_if(FORWARD);
			}
			break;
		}
		default:
		{
			lcd_printf("Something went\nTerribly Wrong");
			refreshPin();
			delay(10000);
			current_state = RESTARTING;
			break;
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


	lcd_printf("%s\n%02hu:%02hu:%02hu [%s]", 
		states[current_state], glb_timer.hours, glb_timer.minutes, glb_timer.seconds, syringe_states[current_content_status]);

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

	increment_button.set_pin(B_Inc);
	decrement_button.set_pin(B_Dec);
	autofeed_button.set_pin(B_auto_feed);
	manfeed_button.set_pin(B_feed);
	manfill_button.set_pin(B_fill);

	serial_printf("Done\n");

	delay(500);
	_FN_END

}
void loop()
{
	_FN_START

	refreshPin();
	refresh_debounced_pins();
	
	writePin(LED_Inc,increment_button.is_low());
	writePin(LED_Dec,decrement_button.is_low());
	writePin(LED_fill,manfill_button.is_low());
	writePin(LED_feed,manfeed_button.is_low());
	writePin(LED_auto_feed,autofeed_button.is_low());

	delay(2);
	_FN_END
}
#else
#ifdef SR_SWITCH_TEST
void setup()
{
	_FN_START

	serial_printf("Initializing\n");
	setPin(LED_fill, OUTPUT);
	setPin(LED_Inc, OUTPUT);
	setPin(LED_Dec, OUTPUT);

	emergency_button.set_pin(B_Emergency_NC, B_Emergency_NO);
	full_switch.set_pin(empty_switch_NC, empty_switch_NO);
	empty_switch.set_pin(full_switch_NC, full_switch_NO);

	serial_printf("Done\n");

	delay(500);
	_FN_END

}
void loop()
{
	_FN_START

	refreshPin();
	refresh_debounced_pins();
	
	writePin(LED_Inc,emergency_button.is_low());
	writePin(LED_Dec,full_switch.is_low());
	writePin(LED_fill,empty_switch.is_low());


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
#endif

