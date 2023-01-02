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
const loadContextData = () => {
    const contextData = {
        adc: {},
        timestamp: 0,
        sw_version: '',
        ws_version: '1.0.0',
    };

    for (let i in ADC) {
        if (ADC.hasOwnProperty(i)) {
            contextData.adc[i] = 0;
        }
    }

    return contextData;
}

const contextData = loadContextData();