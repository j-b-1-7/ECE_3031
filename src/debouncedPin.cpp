#include "debouncedPin.h"
#include "pin_wrapper.h"
#include "message.h"

static uint8_t cycle_id = 0;
void refresh_debounced_pins()
{
	cycle_id += 1;
}

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

void debouncedPin::_refresh()
{
	if(cycle_id != this->_last_cycle_checked)
	{
		this->_last_cycle_checked = cycle_id;

		uint8_t pin_value = readPin(this->_pin_id);  

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
				uint8_t new_state = LOW;
				if(this->_count[LOW] < this->_count[HIGH])
				{
					new_state = HIGH;
				}

				this->_changed = (this->_state != new_state);
				this->_state = new_state;

				if(this->_changed)
				{
					debug_printf("button %02hu %s\n", this->_pin_id, (this->_state)? "HIGH": "LOW");
				}

				// we reset the timer
				this->_stop_timer();
			}
		}
	}
}

void debouncedPin::set_pin(uint8_t pin_id)
{
	this->_pin_id = pin_id;
    setPin(this->_pin_id, INPUT);
}

uint8_t debouncedPin::is_high()
{
	_refresh();
    return (this->_state == HIGH);
}

uint8_t debouncedPin::is_low()
{
	_refresh();
    return (this->_state == HIGH);
}

uint8_t debouncedPin::is_falling_edge()
{
	_refresh();
    return (this->_changed && this->is_low());
}

uint8_t debouncedPin::is_rising_edge()
{
	_refresh();
    return (this->_changed && this->is_high());
}