const ADC = [
    // ADC_DEV_0_CHAN_0
    {
        name: 'ADC_DEV_0_CHAN_0',
    },
    // ADC_DEV_0_CHAN_1
    {
        name: 'ADC_DEV_0_CHAN_1',
    },
    // ADC_DEV_0_CHAN_2
    {
        name: 'ADC_DEV_0_CHAN_2',
    },
    // ADC_DEV_0_CHAN_3
    {
        name: 'ADC_DEV_0_CHAN_3',
    },
    // ADC_DEV_0_CHAN_4
    {
        name: 'ADC_DEV_0_CHAN_4',
    },
    // ADC_DEV_0_CHAN_5
    {
        name: 'ADC_DEV_0_CHAN_5',
    },
    // ADC_DEV_0_CHAN_6
    {
        name: 'ADC_DEV_0_CHAN_6',
    },
    // ADC_DEV_0_CHAN_7
    {
        name: 'ADC_DEV_0_CHAN_7',
    },
    // ADC_DEV_1_CHAN_0
    {
        name: 'ADC_DEV_1_CHAN_0',
    },
    // ADC_DEV_1_CHAN_1
    {
        name: 'ADC_DEV_1_CHAN_1',
    },
    // ADC_DEV_1_CHAN_2
    {
        name: 'ADC_DEV_1_CHAN_2',
    },
    // ADC_DEV_1_CHAN_3
    {
        name: 'ADC_DEV_1_CHAN_3',
    },
    // ADC_DEV_1_CHAN_4
    {
        name: 'ADC_DEV_1_CHAN_4',
    },
    // ADC_DEV_1_CHAN_5
    {
        name: 'ADC_DEV_1_CHAN_5',
    },
    // ADC_DEV_1_CHAN_6
    {
        name: 'ADC_DEV_1_CHAN_6',
    },
    // ADC_DEV_1_CHAN_7
    {
        name: 'ADC_DEV_1_CHAN_7',
    },
    // ADC_DEV_2_CHAN_0
    {
        name: 'ADC_DEV_2_CHAN_0',
    },
    // ADC_DEV_2_CHAN_1
    {
        name: 'ADC_DEV_2_CHAN_1',
    },
    // ADC_DEV_2_CHAN_2
    {
        name: 'ADC_DEV_2_CHAN_2',
    },
    // ADC_DEV_2_CHAN_3
    {
        name: 'ADC_DEV_2_CHAN_3',
    },
    // ADC_DEV_2_CHAN_4
    {
        name: 'ADC_DEV_2_CHAN_4',
    },
    // ADC_DEV_2_CHAN_5
    {
        name: 'ADC_DEV_2_CHAN_5',
    },
    // ADC_DEV_2_CHAN_6
    {
        name: 'ADC_DEV_2_CHAN_6',
    },
    // ADC_DEV_2_CHAN_7
    {
        name: 'ADC_DEV_2_CHAN_7',
    },
    // ADC_DEV_3_CHAN_0
    {
        name: 'ADC_DEV_3_CHAN_0',
    },
    // ADC_DEV_3_CHAN_1
    {
        name: 'ADC_DEV_3_CHAN_1',
    },
    // ADC_DEV_3_CHAN_2
    {
        name: 'ADC_DEV_3_CHAN_2',
    },
    // ADC_DEV_3_CHAN_3
    {
        name: 'ADC_DEV_3_CHAN_3',
    },
    // ADC_DEV_3_CHAN_4
    {
        name: 'ADC_DEV_3_CHAN_4',
    },
    // ADC_DEV_3_CHAN_5
    {
        name: 'ADC_DEV_3_CHAN_5',
    },
    // ADC_DEV_3_CHAN_6
    {
        name: 'ADC_DEV_3_CHAN_6',
    },
    // ADC_DEV_3_CHAN_7
    {
        name: 'ADC_DEV_3_CHAN_7',
    },
];

const IO = [
    // O_KA1
    {
        name: 'O_KA1',
        type: 'range',
        min_value: 0,
        max_value: 1,
    },
    // O_KA2
    {
        name: 'O_KA2',
        type: 'range',
        min_value: 0,
        max_value: 1,
    },
    // O_KA3
    {
        name: 'O_KA3',
        type: 'range',
        min_value: 0,
        max_value: 1,
    },
    // O_KA4
    {
        name: 'O_KA4',
        type: 'range',
        min_value: 0,
        max_value: 1,
    },
    // O_KA5
    {
        name: 'O_KA5',
        type: 'range',
        min_value: 0,
        max_value: 1,
    },
    // O_KA6
    {
        name: 'O_KA6',
        type: 'range',
        min_value: 0,
        max_value: 1,
    },
    // O_KA7
    {
        name: 'O_KA7',
        type: 'range',
        min_value: 0,
        max_value: 1,
    },
    // O_KA8
    {
        name: 'O_KA8',
        type: 'range',
        min_value: 0,
        max_value: 1,
    },
];

const loadContextData = () => {
    const contextData = {
        adc: {},
        io: {},
        timestamp: 0,
        sw_version: '',
        ws_version: '1.1.0',
    };

    for (let i = 0; i < ADC.length; i++) {
        contextData.adc[i] = 0;
    }

    for (let i = 0; i < IO.length; i++) {
        contextData.io[i] = 0;
    }

    return contextData;
}

const getIndexFromAdcName = (adcName) => {
    return ADC.findIndex(element => element.name === adcName);
}

const contextData = loadContextData();