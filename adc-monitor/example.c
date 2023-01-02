//
// Created by Alexandre Tortevois

// Compile with: gcc example.c -o mcp3008-example

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#define ARRAY_SIZEOF(array) sizeof(array) / sizeof(array[0])

static const char *SPI_DEVICE = "/dev/spidev0.1";
static uint8_t ADC_MCP3008_SPI_MODE = SPI_MODE_0;
static uint8_t ADC_MCP3008_SPI_BITS_PER_WORD = 8;
static uint32_t ADC_MCP3008_SPI_MAX_SPEED_HZ = 1000000;
static uint16_t ADC_MCP3008_DELAY = 5;

// Ensure all settings are correct for the ADC
static int prepare(int fd) {

    if (ioctl(fd, SPI_IOC_WR_MODE, &ADC_MCP3008_SPI_MODE) == -1) {
        perror("Can't set MODE");
        return -1;
    }

    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &ADC_MCP3008_SPI_BITS_PER_WORD) == -1) {
        perror("Can't set number of BITS");
        return -1;
    }

    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &ADC_MCP3008_SPI_MAX_SPEED_HZ) == -1) {
        perror("Can't set write CLOCK");
        return -1;
    }

    if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &ADC_MCP3008_SPI_MAX_SPEED_HZ) == -1) {
        perror("Can't set read CLOCK");
        return -1;
    }

    return 0;
}

// SGL/DIF = 0, D2=D1=D0=0
uint8_t control_bits_differential(uint8_t channel) {
    return (channel & 7) << 4;
}

// SGL/DIF = 1, D2=D1=D0=0
uint8_t control_bits(uint8_t channel) {
    return 0x8 | control_bits_differential(channel);
}

// Given a file descriptor, and an ADC channel, fetch the raw ADC value for the given channel.
int readadc(int fd, uint8_t channel) {
    uint8_t tx[] = {1, control_bits(channel), 0};
    uint8_t rx[3];

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long) tx,
        .rx_buf = (unsigned long) rx,
        .len = ARRAY_SIZEOF(tx),
        .delay_usecs = ADC_MCP3008_DELAY,
        .speed_hz = ADC_MCP3008_SPI_MAX_SPEED_HZ,
        .bits_per_word = ADC_MCP3008_SPI_BITS_PER_WORD,
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == 1) {
        perror("IO Error");
        abort();
    }

    return ((rx[1] << 8) & 0x300) | (rx[2] & 0xFF);
}

int main(int argc, char **argv) {

    int fd = open(SPI_DEVICE, O_RDWR);

    if (fd <= 0) {
        printf("Device %s not found\n", SPI_DEVICE);
        return -1;
    }

    if (prepare(fd) == -1) {
        return -1;
    }

    for (uint8_t i = 0; i < 8; i++) {
        printf("Channel %d: %d\n", i + 1, readadc(fd, i));
    }

    close(fd);

    return 0;
}