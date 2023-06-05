// abstraction layer for ioctl calls
// Created by Alexandre Tortevois

#include <errno.h>
#include <fcntl.h> // open
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //close, read, write
#include <linux/gpio.h>
#include <sys/ioctl.h>

#include "drivers/gpio.h"
#include "utils/log.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int gpio_fd = -1;
static const char gpio_adapter[] = "/dev/gpiochip0";

static const char *L_IO_STATE[IO_STATE_COUNT] = {"Open", "Close"};
static const char *L_IO_EVENT[IO_EVENT_COUNT] = {"IO_EVENT_NONE", "IO_EVENT_RISING_EDGE", "IO_EVENT_FALLING_EDGE"};

static gpio_struc GPIO[IO_COUNT] = {
    // O_KA1
    {
        .name = "O_KA1",
        .pin = 23,
        .type = OUT_TOR,
        .is_reversed = true,
        .init_state = IO_CLOSE,
    },
    // O_KA2
    {
        .name = "O_KA2",
        .pin = 24,
        .type = OUT_TOR,
        .is_reversed = true,
        .init_state = IO_CLOSE
    },
    // O_KA3
    {
        .name = "O_KA3",
        .pin = 5,
        .type = OUT_TOR,
        .is_reversed = true,
        .init_state = IO_CLOSE
    },
    // O_KA4
    {
        .name = "O_KA4",
        .pin = 6,
        .type = OUT_TOR,
        .is_reversed = true,
        .init_state = IO_CLOSE
    },
    // O_KA5
    {
        .name = "O_KA5",
        .pin = 26,
        .type = OUT_TOR,
        .is_reversed = true,
        .init_state = IO_CLOSE
    },
    // O_KA6
    {
        .name = "O_KA6",
        .pin = 16,
        .type = OUT_TOR,
        .is_reversed = true,
        .init_state = IO_CLOSE
    },
    // O_KA7
    {
        .name = "O_KA7",
        .pin = 20,
        .type = OUT_TOR,
        .is_reversed = true,
        .init_state = IO_CLOSE
    },
    // O_KA8
    {
        .name = "O_KA8",
        .pin = 21,
        .type = OUT_TOR,
        .is_reversed = true,
        .init_state = IO_CLOSE
    },
};

// -- Private functions ---------------------------------------------------------------------

/** Convert value switch unless_state */
static io_state gpio_compute_value(bool is_reverse, io_state value) {
    if (is_reverse)
        return (value == IO_CLOSE) ? IO_OPEN : IO_CLOSE;

    return value;
}

/** Read GPIO with ioctl */
static int gpio_ioctl_read(int fd, io_state *value) {
    int rc = 0;
    struct gpiohandle_data data;
    if ((rc = ioctl(fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data)) < 0) {
        goto __error;
    }
    *value = (io_state) data.values[0];

    __error:
    return rc;
}

/** Write GPIO with ioctl */
static int gpio_ioctl_write(int fd, io_state value) {
    int rc = 0;
    struct gpiohandle_data data;
    data.values[0] = (int) value;
    if ((rc = ioctl(fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data)) < 0) {
        goto __error;
    }

    __error:
    return rc;
}

static void gpio_log(gpio_id id, io_state value) {
    char buffer[255];
    switch (id) {
        // case O_LED:
        // No logs
        // return;
        default:
            sprintf(buffer, "%s: %s", GPIO[id].name, L_IO_STATE[value]);
            break;
    }
    log_info(buffer);
}

// ------------------------------------------------------------------------------------------

/** GPIO Initializer */
int gpio_init(void) {
    int rc = 0;

    // Initialize mutex
    if (pthread_mutex_init(&mutex, PTHREAD_MUTEX_DEFAULT) != 0) {
        log_fatal("Unable to initialize mutex: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Open GPIO file descriptor
    if ((gpio_fd = open(gpio_adapter, O_RDONLY)) < 0) {
        log_fatal("Unable to open %s: %s", gpio_adapter, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Check if file descriptor is open
    struct gpiochip_info info;
    if (ioctl(gpio_fd, GPIO_GET_CHIPINFO_IOCTL, &info) < 0) {
        log_fatal("Unable to read %s: %s", gpio_adapter, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Critical section -->
    pthread_mutex_lock(&mutex);

    for (gpio_id id = 0; id < IO_COUNT; id++) {
        // Initialization
        GPIO[id].handler.fd = -1;
        GPIO[id].value = 0;
        GPIO[id].event = IO_EVENT_NONE;
        GPIO[id].is_updated = false;

        // Set a GPIO handle request
        struct gpiohandle_request req;
        memset(&req, 0, sizeof(req));
        req.lineoffsets[0] = GPIO[id].pin;
        req.lines = 1;
        switch (GPIO[id].type) {
            case IN_TOR :
                req.flags = GPIOHANDLE_REQUEST_INPUT;
                break;
            case OUT_TOR:
                req.flags = GPIOHANDLE_REQUEST_OUTPUT;
                req.default_values[0] = (__u8) GPIO[id].init_state; // this specifies the default output value, should be 0 (low) or 1 (high), anything else than 0 or 1 will be interpreted as 1 (high)
                break;
            default:
                log_fatal("Unable to initialize GPIO[%d] %s: Unknown IO_TYPE", id, GPIO[id].name);
                exit(EXIT_FAILURE);
        }
        strcpy(req.consumer_label, GPIO[id].name);

        // Request & check
        if (ioctl(gpio_fd, GPIO_GET_LINEHANDLE_IOCTL, &req) < 0) {
            rc = -1;
            log_fatal("Unable to request handle for pin #%d %s: %s", GPIO[id].pin, GPIO[id].name, strerror(errno));
            goto __error;
        }

        // Initial IO reading
        io_state value = -1;
        if ((rc = gpio_ioctl_read(req.fd, &value)) < 0) {
            log_fatal("Unable to read data for pin #%d %s: %s", GPIO[id].pin, GPIO[id].name, strerror(errno));
            goto __error;
        }
        io_state compute_value = gpio_compute_value(GPIO[id].is_reversed, value);
        gpio_log(id, compute_value);

        GPIO[id].handler = req;
        GPIO[id].value = compute_value;
        GPIO[id].event = IO_EVENT_NONE;
        GPIO[id].is_updated = true;
    }

    __error:
    // <-- Critical section
    pthread_mutex_unlock(&mutex);

    return rc;
}

/** Check if GPIO[id] is Available */
int gpio_id_is_available(int id) {
    if (id >= 0 && id < IO_COUNT)
        return 0;

    return -1;
}

/** Read the state of Input or Output GPIO pin */
int gpio_read(gpio_id id) {
    int rc = 0;

    if (gpio_id_is_available(id) < 0) {
        log_fatal("Unable to read GPIO: id #%d is not available", id);
        exit(EXIT_FAILURE);
    }

    // Critical section -->
    pthread_mutex_lock(&mutex);

    io_state read_value;
    if ((rc = gpio_ioctl_read(GPIO[id].handler.fd, &read_value)) < 0) {
        log_fatal("Unable to read data for pin #%d %s: %s", GPIO[id].pin, GPIO[id].name, strerror(errno));
        goto __error;
    }

    io_state virtual_value = gpio_compute_value(GPIO[id].is_reversed, read_value);
    if (virtual_value != GPIO[id].value) {
        GPIO[id].value = virtual_value;
        GPIO[id].is_updated = true;
        // Pulse on edge
        GPIO[id].event = (virtual_value == IO_OPEN) ? IO_EVENT_FALLING_EDGE : IO_EVENT_RISING_EDGE;
        //log_info("%s: %s", GPIO[id].name, L_IO_STATE[virtual_value]);
        gpio_log(id, virtual_value);
    } else if (GPIO[id].event != IO_EVENT_NONE) {
        GPIO[id].event = IO_EVENT_NONE;
        log_info("%s: %s", GPIO[id].name, L_IO_EVENT[GPIO[id].event]);
    }

    __error:
    // <-- Critical section
    pthread_mutex_unlock(&mutex);

    return rc;
}

/** Write on Output GPIO pin */
int gpio_write(gpio_id id, io_state value) {
    int rc = 0;

    // Check id
    if ((rc = gpio_id_is_available(id)) < 0) {
        log_fatal("Unable to write GPIO: id #%d is not available", id);
        exit(EXIT_FAILURE);
    }

    // Check value
    if (value != IO_CLOSE && value != IO_OPEN) {
        log_fatal("Unable to write GPIO: Unknown IO_TYPE %d", value);
        exit(EXIT_FAILURE);
    }

    // Critical section -->
    pthread_mutex_lock(&mutex);

    if (GPIO[id].type != OUT_TOR) {
        rc = -1;
        log_fatal("Unable to write an input pin #%d %s", GPIO[id].pin, GPIO[id].name);
        goto __error;
    }

    io_state real_value = gpio_compute_value(GPIO[id].is_reversed, value);
    if (value != GPIO[id].value) {
        if ((rc = gpio_ioctl_write(GPIO[id].handler.fd, real_value)) < 0) {
            log_fatal("Unable to write data for pin #%d: %s", GPIO[id].pin, strerror(errno));
            goto __error;
        }

        // Store the last known state
        GPIO[id].value = value;
        GPIO[id].is_updated = true;
        // log_info("%s: %s", GPIO[id].name, L_IO_STATE[value]);
        gpio_log(id, value);
    }

    __error:
    // <-- Critical section
    pthread_mutex_unlock(&mutex);

    return rc;
}

/** Safe getter for GPIO.is_reversed */
/* Unused
bool gpio_get_is_reversed(gpio_id id) {
    bool is_reversed;

    if (gpio_id_is_available(id) < 0) {
        log_fatal("Unable to write GPIO: id #%d is not available", id);
        exit(EXIT_FAILURE);
    }

    // Critical section -->
    pthread_mutex_lock(&mutex);

    is_reversed = GPIO[id].is_reversed;

    // <-- Critical section
    pthread_mutex_unlock(&mutex);

    return is_reversed;
}
*/

/** Safe getter for GPIO.value */
io_state gpio_get_value(gpio_id id) {
    io_state value;

    if (gpio_id_is_available(id) < 0) {
        log_fatal("Unable to write GPIO: id #%d is not available", id);
        exit(EXIT_FAILURE);
    }

    // Critical section -->
    pthread_mutex_lock(&mutex);

    value = GPIO[id].value;

    // <-- Critical section
    pthread_mutex_unlock(&mutex);

    return value;
}

/** Safe getter for GPIO.event */
/* Unused
io_event gpio_get_event(gpio_id id) {
    io_event event;

    if (gpio_id_is_available(id) < 0) {
        log_fatal("Unable to write GPIO: id #%d is not available", id);
        exit(EXIT_FAILURE);
    }

    // Critical section -->
    pthread_mutex_lock(&mutex);

    event = GPIO[id].event;

    // <-- Critical section
    pthread_mutex_unlock(&mutex);

    return event;
}
*/

/** Safe getter for GPIO.is_updated */
bool gpio_is_updated(gpio_id id) {
    bool is_updated = false;

    if (gpio_id_is_available(id) < 0) {
        log_fatal("Unable to set GLOBAL: id #%d is not available", id);
        exit(EXIT_FAILURE);
    }

    // Critical section -->
    pthread_mutex_lock(&mutex);

    is_updated = GPIO[id].is_updated;

    // <-- Critical section
    pthread_mutex_unlock(&mutex);

    return is_updated;
}

/** Safe setter for GPIO.is_updated */
void gpio_set_is_updated(gpio_id id, bool value) {
    if (gpio_id_is_available(id) < 0) {
        log_fatal("Unable to set GLOBAL: id #%d is not available", id);
        exit(EXIT_FAILURE);
    }

    // Critical section -->
    pthread_mutex_lock(&mutex);

    GPIO[id].is_updated = value;

    // <-- Critical section
    pthread_mutex_unlock(&mutex);
}

/** Safe Cleaner */
int gpio_clean(void) {
    // Critical section -->
    pthread_mutex_lock(&mutex);

    for (gpio_id id = 0; id < IO_COUNT; id++) {
        if (GPIO[id].pin != 0 && GPIO[id].handler.fd > 0) {
            if (GPIO[id].type == OUT_TOR) {
                if (gpio_ioctl_write(GPIO[id].handler.fd, IO_OPEN) < 0) {
                    log_fatal("Unable to open pin #%d %s : %s", GPIO[id].pin, GPIO[id].name, strerror(errno));
                }
            }

            if (close(GPIO[id].handler.fd) < 0) {
                log_fatal("Unable to close pin #%d %s : %s", GPIO[id].pin, GPIO[id].name, strerror(errno));
            }
        }
    }

    // <-- Critical section
    pthread_mutex_unlock(&mutex);

    // Check if GPIO fd is open
    if (gpio_fd != -1) {
        // Close gpio_adapter file descriptor
        if (close(gpio_fd) < 0) {
            log_fatal("Unable to close %s: %s", gpio_adapter, strerror(errno));
        }
    }

    // Destroy GPIO mutex
    if (pthread_mutex_destroy(&mutex) > 0) {
        log_fatal("Unable to detroy the mutex: %s", strerror(errno));
    }

    return 0;
}