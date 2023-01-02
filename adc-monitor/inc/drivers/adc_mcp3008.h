// abstraction layer for ioctl calls via spidev for MCP3008
// Created by Alexandre Tortevois

#ifndef H_ADC_MCP3008_H
#define H_ADC_MCP3008_H

#include <stdbool.h>

#define ADC_MCP3008_DELAY                                                5
#define ADC_MCP3008_SPI_RX_BUFFER_SIZE                                   3
#define ADC_MCP3008_FIFO_MAX_SIZE                                       16 // value smoothing (resolution)
#define ADC_MCP3008_DEAD_BAND                                           10
#define ADC_MCP3008_MIN_VALUE                                            0
#define ADC_MCP3008_MAX_VALUE                                         1000

typedef enum {
    ADC_MCP3008_SPIDEV_0,
    ADC_MCP3008_SPIDEV_1,
    ADC_MCP3008_SPIDEV_2,
    ADC_MCP3008_SPIDEV_3,
    ADC_MCP3008_SPIDEVICES_COUNT,
} adc_mcp3008_spidev_id;

typedef enum {
    ADC_DEV_0_CHAN_0,
    ADC_DEV_0_CHAN_1,
    ADC_DEV_0_CHAN_2,
    ADC_DEV_0_CHAN_3,
    ADC_DEV_0_CHAN_4,
    ADC_DEV_0_CHAN_5,
    ADC_DEV_0_CHAN_6,
    ADC_DEV_0_CHAN_7,
    ADC_DEV_1_CHAN_0,
    ADC_DEV_1_CHAN_1,
    ADC_DEV_1_CHAN_2,
    ADC_DEV_1_CHAN_3,
    ADC_DEV_1_CHAN_4,
    ADC_DEV_1_CHAN_5,
    ADC_DEV_1_CHAN_6,
    ADC_DEV_1_CHAN_7,
    ADC_DEV_2_CHAN_0,
    ADC_DEV_2_CHAN_1,
    ADC_DEV_2_CHAN_2,
    ADC_DEV_2_CHAN_3,
    ADC_DEV_2_CHAN_4,
    ADC_DEV_2_CHAN_5,
    ADC_DEV_2_CHAN_6,
    ADC_DEV_2_CHAN_7,
    ADC_DEV_3_CHAN_0,
    ADC_DEV_3_CHAN_1,
    ADC_DEV_3_CHAN_2,
    ADC_DEV_3_CHAN_3,
    ADC_DEV_3_CHAN_4,
    ADC_DEV_3_CHAN_5,
    ADC_DEV_3_CHAN_6,
    ADC_DEV_3_CHAN_7,
    ADC_INPUTS_COUNT,
} adc_mcp3008_input_id;

typedef struct {
    const char *name;
    adc_mcp3008_spidev_id spidev_id;
    int channel;
    int fifo_values[ADC_MCP3008_FIFO_MAX_SIZE];
    int fifo_pos;
    int fifo_sum;
    int fifo_count;
    int raw_value;
    bool is_updated;
} adc_mcp3008_t;

int adc_mcp3008_init(void);
int adc_mcp3008_read(adc_mcp3008_input_id id);
int adc_mcp3008_get_raw_value(adc_mcp3008_input_id id);
void adc_mcp3008_set_is_updated(adc_mcp3008_input_id id, bool is_updated);
bool adc_mcp3008_is_updated(adc_mcp3008_input_id id);
int adc_mcp3008_clean(void);

#endif //H_ADC_MCP3008_H
