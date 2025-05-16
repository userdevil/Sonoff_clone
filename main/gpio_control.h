// main/gpio_control.h

#pragma once

typedef enum {
    BUTTON_SHORT_PRESS,
    BUTTON_LONG_PRESS
} button_event_t;

void gpio_control_init(void);
void gpio_control_set_relay(bool on);
bool gpio_control_get_relay_state(void);
void gpio_control_handle_button_event(button_event_t event);
