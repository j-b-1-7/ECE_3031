#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <Arduino.h>

// debounce for N millis
#define DEBOUNCE_TIME 50

class debouncedPin
{
private:
    uint8_t _pin_id_no;
    uint8_t _pin_id_nc;
    
	uint8_t _count[2];
	uint8_t _millis;
    uint8_t _last_cycle_checked;

    uint8_t _state: 1;
    uint8_t _changed: 1;

    void _refresh();
    void _stop_timer();
    void _start_timer();
    bool _timer_is_started();

public:
    debouncedPin()
    {
        this->_pin_id_no = UINT8_MAX; // some really high value
        this->_pin_id_nc = UINT8_MAX; // some really high value
        this->_count[LOW] = 0;
        this->_count[HIGH] = 0;
        this->_millis = 0;
    }

    void set_pin(uint8_t pin_id);
    void set_pin(uint8_t pin_id_nc, uint8_t pin_id_no);

    uint8_t is_high();
    uint8_t is_low();
    uint8_t is_falling_edge();
    uint8_t is_rising_edge();
};

void refresh_debounced_pins();

#endif