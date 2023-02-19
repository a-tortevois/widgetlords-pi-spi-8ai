//
// Created by Alexandre on 19/02/2023.

#ifndef H_GPIO_H
#define H_GPIO_H

#include <linux/gpio.h>
#include <stdbool.h>

typedef enum {
    IN_TOR = 1,
    OUT_TOR,
} io_type;

typedef enum {
    IO_OPEN,
    IO_CLOSE,
    IO_STATE_COUNT
} io_state;

typedef enum {
    IO_EVENT_NONE,
    IO_EVENT_RISING_EDGE,
    IO_EVENT_FALLING_EDGE,
    IO_EVENT_COUNT
} io_event;

typedef enum {
    O_KA1,
    O_KA2,
    IO_COUNT
} gpio_id;

typedef struct {
    const char *name;
    int pin;
    io_type type;
    bool is_reversed;
    io_state init_state;
    struct gpiohandle_request handler;
    io_state value;
    io_event event;
    bool is_updated;
} gpio_struc;

int gpio_init(void);
int gpio_id_is_available(int id);
// int gpio_read(gpio_id id);
int gpio_write(gpio_id id, io_state value);
// bool gpio_get_is_reversed(gpio_id id);
io_state gpio_get_value(gpio_id id);
// io_event gpio_get_event(gpio_id id);
bool gpio_is_updated(gpio_id id);
void gpio_set_is_updated(gpio_id id, bool value);
int gpio_clean(void);

#endif //H_GPIO_H
