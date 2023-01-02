// abstraction layer for ioctl calls via spidev for MCP3008
// Created by Alexandre Tortevois

#include <errno.h>
#include <fcntl.h> // open
#include <pthread.h>
#include <stdint.h> // uint8_t
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // close, read, write
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "config.h"
#include "drivers/adc_mcp3008.h"
#include "utils/stdfun.h"
#include "utils/log.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static uint8_t ADC_MCP3008_SPI_MODE = SPI_MODE_0;
static uint8_t ADC_MCP3008_SPI_BITS_PER_WORD = 8;
static uint32_t ADC_MCP3008_SPI_MAX_SPEED_HZ = 1000000;
static const char *sysfs_paths[ADC_MCP3008_SPIDEVICES_COUNT] = {
    // ADC_MCP3008_SPIDEV_0
    "/dev/spidev0.0",
    // ADC_MCP3008_SPIDEV_1
    "/dev/spidev0.1",
    // ADC_MCP3008_SPIDEV_2
    "/dev/spidev0.2",
    // ADC_MCP3008_SPIDEV_3
    "/dev/spidev0.3",
};
static int fd[ADC_MCP3008_SPIDEVICES_COUNT] = {-1, -1, -1, -1};
static adc_mcp3008_t ADC_MCP3008[ADC_INPUTS_COUNT] = {
    // ADC_DEV_0_CHAN_0
    {
        .name = "ADC_DEV_0_CHAN_0",
        .spidev_id =ADC_MCP3008_SPIDEV_0,
        .channel = 0,
    },
    // ADC_DEV_0_CHAN_1
    {
        .name = "ADC_DEV_0_CHAN_1",
        .spidev_id = ADC_MCP3008_SPIDEV_0,
        .channel = 1,
    },
    // ADC_DEV_0_CHAN_2
    {
        .name = "ADC_DEV_0_CHAN_2",
        .spidev_id = ADC_MCP3008_SPIDEV_0,
        .channel = 2,
    },
    // ADC_DEV_0_CHAN_3
    {
        .name = "ADC_DEV_0_CHAN_3",
        .spidev_id = ADC_MCP3008_SPIDEV_0,
        .channel = 3,
    },
    // ADC_DEV_0_CHAN_4
    {
        .name = "ADC_DEV_0_CHAN_4",
        .spidev_id = ADC_MCP3008_SPIDEV_0,
        .channel = 4,
    },
    // ADC_DEV_0_CHAN_5
    {
        .name = "ADC_DEV_0_CHAN_5",
        .spidev_id = ADC_MCP3008_SPIDEV_0,
        .channel = 5,
    },
    // ADC_DEV_0_CHAN_6
    {
        .name = "ADC_DEV_0_CHAN_6",
        .spidev_id = ADC_MCP3008_SPIDEV_0,
        .channel = 6,
    },
    // ADC_DEV_0_CHAN_7
    {
        .name = "ADC_DEV_0_CHAN_7",
        .spidev_id = ADC_MCP3008_SPIDEV_0,
        .channel = 7,
    },
    // ADC_DEV_1_CHAN_0
    {
        .name = "ADC_DEV_1_CHAN_0",
        .spidev_id =ADC_MCP3008_SPIDEV_1,
        .channel = 0,
    },
    // ADC_DEV_1_CHAN_1
    {
        .name = "ADC_DEV_1_CHAN_1",
        .spidev_id =ADC_MCP3008_SPIDEV_1,
        .channel = 1,
    },
    // ADC_DEV_1_CHAN_2
    {
        .name = "ADC_DEV_1_CHAN_2",
        .spidev_id =ADC_MCP3008_SPIDEV_1,
        .channel = 2,
    },
    // ADC_DEV_1_CHAN_3
    {
        .name = "ADC_DEV_1_CHAN_3",
        .spidev_id =ADC_MCP3008_SPIDEV_1,
        .channel = 3,
    },
    // ADC_DEV_1_CHAN_4
    {
        .name = "ADC_DEV_1_CHAN_4",
        .spidev_id =ADC_MCP3008_SPIDEV_1,
        .channel = 4,
    },
    // ADC_DEV_1_CHAN_5
    {
        .name = "ADC_DEV_1_CHAN_5",
        .spidev_id =ADC_MCP3008_SPIDEV_1,
        .channel = 5,
    },
    // ADC_DEV_1_CHAN_6
    {
        .name = "ADC_DEV_1_CHAN_6",
        .spidev_id =ADC_MCP3008_SPIDEV_1,
        .channel = 6,
    },
    // ADC_DEV_1_CHAN_7
    {
        .name = "ADC_DEV_1_CHAN_7",
        .spidev_id =ADC_MCP3008_SPIDEV_1,
        .channel = 7,
    },
    // ADC_DEV_2_CHAN_0
    {
        .name = "ADC_DEV_2_CHAN_0",
        .spidev_id =ADC_MCP3008_SPIDEV_2,
        .channel = 0,
    },
    // ADC_DEV_2_CHAN_1
    {
        .name = "ADC_DEV_2_CHAN_1",
        .spidev_id =ADC_MCP3008_SPIDEV_2,
        .channel = 1,
    },
    // ADC_DEV_2_CHAN_2
    {
        .name = "ADC_DEV_2_CHAN_2",
        .spidev_id =ADC_MCP3008_SPIDEV_2,
        .channel = 2,
    },
    // ADC_DEV_2_CHAN_3
    {
        .name = "ADC_DEV_2_CHAN_3",
        .spidev_id =ADC_MCP3008_SPIDEV_2,
        .channel = 3,
    },
    // ADC_DEV_2_CHAN_4
    {
        .name = "ADC_DEV_2_CHAN_4",
        .spidev_id =ADC_MCP3008_SPIDEV_2,
        .channel = 4,
    },
    // ADC_DEV_2_CHAN_5
    {
        .name = "ADC_DEV_2_CHAN_5",
        .spidev_id =ADC_MCP3008_SPIDEV_2,
        .channel = 5,
    },
    // ADC_DEV_2_CHAN_6
    {
        .name = "ADC_DEV_2_CHAN_6",
        .spidev_id =ADC_MCP3008_SPIDEV_2,
        .channel = 6,
    },
    // ADC_DEV_2_CHAN_7
    {
        .name = "ADC_DEV_2_CHAN_7",
        .spidev_id =ADC_MCP3008_SPIDEV_2,
        .channel = 7,
    },
    // ADC_DEV_3_CHAN_0
    {
        .name = "ADC_DEV_3_CHAN_0",
        .spidev_id =ADC_MCP3008_SPIDEV_3,
        .channel = 0,
    },
    // ADC_DEV_3_CHAN_1
    {
        .name = "ADC_DEV_3_CHAN_1",
        .spidev_id =ADC_MCP3008_SPIDEV_3,
        .channel = 1,
    },
    // ADC_DEV_3_CHAN_2
    {
        .name = "ADC_DEV_3_CHAN_2",
        .spidev_id =ADC_MCP3008_SPIDEV_3,
        .channel = 2,
    },
    // ADC_DEV_3_CHAN_3
    {
        .name = "ADC_DEV_3_CHAN_3",
        .spidev_id =ADC_MCP3008_SPIDEV_3,
        .channel = 3,
    },
    // ADC_DEV_3_CHAN_4
    {
        .name = "ADC_DEV_3_CHAN_4",
        .spidev_id =ADC_MCP3008_SPIDEV_3,
        .channel = 4,
    },
    // ADC_DEV_3_CHAN_5
    {
        .name = "ADC_DEV_3_CHAN_5",
        .spidev_id =ADC_MCP3008_SPIDEV_3,
        .channel = 5,
    },
    // ADC_DEV_3_CHAN_6
    {
        .name = "ADC_DEV_3_CHAN_6",
        .spidev_id =ADC_MCP3008_SPIDEV_3,
        .channel = 6,
    },
    // ADC_DEV_3_CHAN_7
    {
        .name = "ADC_DEV_3_CHAN_7",
        .spidev_id =ADC_MCP3008_SPIDEV_3,
        .channel = 7,
    },
};

// -- Private functions ---------------------------------------------------------------------

static int adc_mcp3008_spidev_id_is_available(adc_mcp3008_spidev_id id) {
    if (id >= 0 && id < ADC_MCP3008_SPIDEVICES_COUNT) {
        return 0;
    }
    return -1;
}

static int adc_mcp3008_input_id_is_available(adc_mcp3008_input_id id) {
    if (id >= 0 && id < ADC_INPUTS_COUNT) {
        return 0;
    }
    return -1;
}

// SGL/DIF = 0, D2=D1=D0=0
static uint8_t control_bits_differential(uint8_t channel) {
    return (channel & 7) << 4;
}

// SGL/DIF = 1, D2=D1=D0=0
static uint8_t control_bits(uint8_t channel) {
    return 0x8 | control_bits_differential(channel);
}

static int adc_mcp3008_init_hw(adc_mcp3008_spidev_id id) {
    int rc = 0;
    struct stat file_stat;

    if ((rc = adc_mcp3008_spidev_id_is_available(id)) < 0) {
        log_fatal("Unable to get ADC_MCP3008: spidev_id #%d is not available", id);
        exit(EXIT_FAILURE);
    }

    if (stat(sysfs_paths[id], &file_stat) < 0) {
        log_warn("Unable to find device %s", sysfs_paths[id]);
        goto __error;
    }

    if ((fd[id] = open(sysfs_paths[id], O_RDWR)) < 0) {
        rc = -1;
        log_fatal("Unable to open device %s", sysfs_paths[id]);
        goto __error;
    }

    if ((rc = ioctl(fd[id], SPI_IOC_WR_MODE, &ADC_MCP3008_SPI_MODE)) < 0) {
        log_fatal("Unable to set WR_MODE for device %s", sysfs_paths[id]);
        goto __error;
    }

    if ((rc = ioctl(fd[id], SPI_IOC_WR_BITS_PER_WORD, &ADC_MCP3008_SPI_BITS_PER_WORD)) == -1) {
        log_fatal("Unable to set WR_BITS_PER_WORD for device %s", sysfs_paths[id]);
        goto __error;
    }

    if ((rc = ioctl(fd[id], SPI_IOC_WR_MAX_SPEED_HZ, &ADC_MCP3008_SPI_MAX_SPEED_HZ)) == -1) {
        log_fatal("Unable to set WR_MAX_SPEED_HZ for device %s", sysfs_paths[id]);
        goto __error;
    }

    if ((rc = ioctl(fd[id], SPI_IOC_RD_MAX_SPEED_HZ, &ADC_MCP3008_SPI_MAX_SPEED_HZ)) == -1) {
        log_fatal("Unable to set RD_MAX_SPEED_HZ for device %s", sysfs_paths[id]);
        goto __error;
    }

    __error:
    return rc;
}

// -- Public functions ----------------------------------------------------------------------

int adc_mcp3008_init(void) {
    int rc = 0;

    // Initialize mutex
    if (pthread_mutex_init(&mutex, PTHREAD_MUTEX_DEFAULT) != 0) {
        log_fatal("Unable to initialize mutex: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Initialize file descriptors
    for (adc_mcp3008_spidev_id id = 0; id < ADC_MCP3008_SPIDEVICES_COUNT; id++) {
        if ((rc = adc_mcp3008_init_hw(id)) < 0) {
            goto __error;
        }
    }

    // Critical section -->
    pthread_mutex_lock(&mutex);

    for (adc_mcp3008_input_id id = 0; id < ADC_INPUTS_COUNT; id++) {
        memset(&ADC_MCP3008[id].fifo_values, 0, ARRAY_SIZEOF(ADC_MCP3008));
        ADC_MCP3008[id].fifo_pos = 0;
        ADC_MCP3008[id].fifo_sum = 0;
        ADC_MCP3008[id].fifo_count = 0;
        ADC_MCP3008[id].raw_value = 0;
        ADC_MCP3008[id].is_updated = false;
    }

    // <-- Critical section
    pthread_mutex_unlock(&mutex);

    __error:
    return rc;
}

int adc_mcp3008_read(adc_mcp3008_input_id id) {
    int rc = 0;

    // Check id
    if (adc_mcp3008_input_id_is_available(id) < 0) {
        log_fatal("Unable to get ADC_MCP3008: input_id #%d is not available", id);
        exit(EXIT_FAILURE);
    }

    // Check fd
    if (fd[ADC_MCP3008[id].spidev_id] < 0) {
        // log_warn("Unable to read %s: file is not open", ADC_MCP3008[id].name);
        goto __error;
    }

    uint8_t tx[] = {1, control_bits(ADC_MCP3008[id].channel), 0};
    uint8_t rx[ADC_MCP3008_SPI_RX_BUFFER_SIZE];
    memset(&rx, 0, ADC_MCP3008_SPI_RX_BUFFER_SIZE);

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long) tx,
        .rx_buf = (unsigned long) rx,
        .len = ADC_MCP3008_SPI_RX_BUFFER_SIZE,
        .delay_usecs = ADC_MCP3008_DELAY,
        .speed_hz = ADC_MCP3008_SPI_MAX_SPEED_HZ,
        .bits_per_word = ADC_MCP3008_SPI_BITS_PER_WORD,
    };

    if ((rc = ioctl(fd[ADC_MCP3008[id].spidev_id], SPI_IOC_MESSAGE(1), &tr)) < 0) {
        log_fatal("Unable to read %s: IO error", ADC_MCP3008[id].name);
        // abort();
        goto __error;
    }

    int raw_value = ((rx[1] << 8) & 0x300) | (rx[2] & 0xFF);

    // Critical section -->
    pthread_mutex_lock(&mutex);

    if (ADC_MCP3008[id].fifo_count == ADC_MCP3008_FIFO_MAX_SIZE) {
        ADC_MCP3008[id].fifo_sum -= ADC_MCP3008[id].fifo_values[ADC_MCP3008[id].fifo_pos];
    }

    if ((rc = fifo_push(ADC_MCP3008[id].fifo_values, ADC_MCP3008_FIFO_MAX_SIZE, &ADC_MCP3008[id].fifo_pos, raw_value)) < 0) {
        goto __error;
    }

    ADC_MCP3008[id].fifo_sum += raw_value;

    if (ADC_MCP3008[id].fifo_count < ADC_MCP3008_FIFO_MAX_SIZE) {
        ADC_MCP3008[id].fifo_count++;
    }

    raw_value = (int) ADC_MCP3008[id].fifo_sum / ADC_MCP3008[id].fifo_count;

    // To be sure that we have a value between min_value -> max_value
    if (raw_value < ADC_MCP3008_MIN_VALUE) {
        raw_value = ADC_MCP3008_MIN_VALUE;
    } else if (raw_value > ADC_MCP3008_MAX_VALUE) {
        raw_value = ADC_MCP3008_MAX_VALUE;
    }

    int lim_low = ADC_MCP3008[id].raw_value - ADC_MCP3008_DEAD_BAND;
    int lim_upp = ADC_MCP3008[id].raw_value + ADC_MCP3008_DEAD_BAND;
    if ((raw_value < ADC_MCP3008_MAX_VALUE && (raw_value <= lim_low || raw_value >= lim_upp)) || ((raw_value == ADC_MCP3008_MIN_VALUE || raw_value == ADC_MCP3008_MAX_VALUE) && raw_value != ADC_MCP3008[id].raw_value)) {
        ADC_MCP3008[id].raw_value = raw_value;
        ADC_MCP3008[id].is_updated = true;
        log_info("%s: %d", ADC_MCP3008[id].name, raw_value);
    }

    // <-- Critical section
    pthread_mutex_unlock(&mutex);

    __error:
    return rc;
}

int adc_mcp3008_get_raw_value(adc_mcp3008_input_id id) {
    int raw_value = 0;

    // Check id
    if (adc_mcp3008_input_id_is_available(id) < 0) {
        log_fatal("Unable to get ADC_MCP3008: input_id #%d is not available", id);
        exit(EXIT_FAILURE);
    }

    // Critical section -->
    pthread_mutex_lock(&mutex);

    raw_value = ADC_MCP3008[id].raw_value;

    // <-- Critical section
    pthread_mutex_unlock(&mutex);

    return raw_value;
}

void adc_mcp3008_set_is_updated(adc_mcp3008_input_id id, bool is_updated) {
    // Check id
    if (adc_mcp3008_input_id_is_available(id) < 0) {
        log_fatal("Unable to get ADC_MCP3008: input_id #%d is not available", id);
        exit(EXIT_FAILURE);
    }

    // Critical section -->
    pthread_mutex_lock(&mutex);

    ADC_MCP3008[id].is_updated = is_updated;

    // <-- Critical section
    pthread_mutex_unlock(&mutex);
}

bool adc_mcp3008_is_updated(adc_mcp3008_input_id id) {
    bool is_updated = false;

    // Check id
    if (adc_mcp3008_input_id_is_available(id) < 0) {
        log_fatal("Unable to get ADC_MCP3008: input_id #%d is not available", id);
        exit(EXIT_FAILURE);
    }

    // Critical section -->
    pthread_mutex_lock(&mutex);

    is_updated = ADC_MCP3008[id].is_updated;

    // <-- Critical section
    pthread_mutex_unlock(&mutex);

    return is_updated;
}

int adc_mcp3008_clean(void) {
    // Destroy mutex
    if (pthread_mutex_destroy(&mutex) > 0) {
        log_fatal("Unable to detroy the ADC mutex: %s", strerror(errno));
    }

    // Close file descriptors
    for (adc_mcp3008_spidev_id id = 0; id < ADC_MCP3008_SPIDEVICES_COUNT; id++) {
        if (fd[id] != -1) {
            if (close(fd[id]) < 0) {
                log_fatal("Unable to close %s", sysfs_paths[id]);
            }
        }
    }

    return 0;
}
