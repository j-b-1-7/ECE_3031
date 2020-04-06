#include "debouncedPin.h"
#include "pin_wrapper.h"
#include "message.h"


static uint16_t time_since(uint16_t from)
{
    uint16_t _current_time = millis();
	if(from > _current_time)
	{
		// we overflowed so we count from (previous to max value) + current 
		return (UINT16_MAX - from) + _current_time;
	}
	else
	{
		return _current_time - from;
	}
}

void debouncedPin::_stop_timer()
{
	this->_count[LOW] = 0;
	this->_count[HIGH] = 0;
	this->_millis = 0;
}

void debouncedPin::_start_timer()
{
	this->_count[LOW] = 0;
	this->_count[HIGH] = 0;
	this->_millis = millis();
}

bool debouncedPin::_timer_is_started()
{
	return (this->_millis > 0);
}

/**
 * software SR latch circuit
 * 
 *   S --|``-.
 *       |    o--,-----q
 *    ,--|..-`  /
 *    |        /
 *    |_______/_
 *   ,-------'  |
 *   '--|``-.   |
 *      |    o--'not_q
 *  R --|..-`
 */
static bool SR_debounce(uint8_t s, uint8_t r, uint8_t q)
{
	uint8_t not_q = !(q && r);
	return  ! (s && not_q );
}

void debouncedPin::refresh()
{
	uint8_t new_state = this->_state;

	/* default pin is normally open */
	/* check if normally closed pin was set for double throw switches */
	if(this->_pin_id_nc != DISCONNECTED)
	{
		uint8_t pin_value_no = readPin(this->_pin_id_no);
		uint8_t pin_value_nc = readPin(this->_pin_id_nc);


		/**
		 *  grab the value using SR debounce for failsafe
		 * if we have a double throw switch 
		 */
		new_state = SR_debounce(
			pin_value_no,
			pin_value_nc,
			this->_state
		);
	}
	/* this is a single throw switch so we need to buffer to debounce */
	else
	{
		uint8_t pin_value = readPin(this->_pin_id_no);

		// if we were waiting for an event
		if ( ! this->_timer_is_started() )
		{
			// if the event happened
			if (pin_value != this->_state)
			{    
				// start the debouncer
				this->_start_timer();
			}
		}
		else
		{
			// we add the current type (HIGH,LOW) to the counter to average later
			this->_count[pin_value] += 1;

			// if we are past the debounce period
			if (time_since(this->_millis) > DEBOUNCE_TIME)
			{
				// assume low
				new_state = LOW;
				if(this->_count[LOW] < this->_count[HIGH])
				{
					new_state = HIGH;
				}

				// we reset the timer
				this->_stop_timer();
			}
		}
	}
	this->_changed = (this->_state != new_state);
	this->_state = new_state;

	if(this->_changed)
	{
		if(this->_pin_id_nc != DISCONNECTED)
		{
			debug_printf("button %02hu - %02hu %s\n", this->_pin_id_no, this->_pin_id_nc, (this->_state)? "HIGH": "LOW");
		}
		else
		{
			debug_printf("button %02hu %s\n", this->_pin_id_no, (this->_state)? "HIGH": "LOW");
		}
	}
}

void debouncedPin::set_pin(uint8_t pin_id)
{
	this->_pin_id_no = pin_id;
    setPin(this->_pin_id_no, INPUT);
}

void debouncedPin::set_pin(uint8_t pin_id_no, uint8_t pin_id_nc)
{
	this->_pin_id_no = pin_id_no;
    setPin(this->_pin_id_no, INPUT);

	this->_pin_id_nc = pin_id_nc;
    setPin(this->_pin_id_nc, INPUT);
}

uint8_t debouncedPin::is_high()
{
    return (this->_state == HIGH);
}

uint8_t debouncedPin::is_low()
{
    return (this->_state == LOW);
}

uint8_t debouncedPin::is_falling_edge()
{
    return (this->_changed && this->is_low());
}

uint8_t debouncedPin::is_rising_edge()
{
    return (this->_changed && this->is_high());
}