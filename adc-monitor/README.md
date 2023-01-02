## C example

Please compile the example with the following command:

```
$ gcc example.c -o mcp3008-example
```

Then, execute the binary:

```
$ chmod +x mcp3008-example
$ ./mcp3008-example
```

## Python example

For Python usage, install the packets:

```bash
$ sudo apt install python3-dev python3-smbus python3-pip
$ sudo pip3 install spidev
```

MCP3008.py:

```python
from spidev import SpiDev


class MCP3008:
    def __init__(self, bus=0, device=0):
        self.bus, self.device = bus, device
        self.spi = SpiDev()
        self.open()
        self.spi.max_speed_hz = 1000000  # 1MHz

    def open(self):
        self.spi.open(self.bus, self.device)
        self.spi.max_speed_hz = 1000000  # 1MHz

    def read(self, channel=0):
        adc = self.spi.xfer2([1, (8 + channel) << 4, 0])
        data = ((adc[1] & 3) << 8) + adc[2]
        return data

    def close(self):
        self.spi.close()
```

adc_monitor.py:

```python
import sys
import time
from MCP3008 import MCP3008

TENSION_MAX = 11.2
spidev0_1 = MCP3008(bus=0, device=1)

try:
    while True:
        raw_value = spidev0_1.read(channel=0)
        print(f"raw_value: {raw_value}    tension: {(raw_value / 1023.0 * TENSION_MAX):.2f}")
        time.sleep(1)


except KeyboardInterrupt:
    print("Interrupted")
    sys.exit(0)
```

## Resources

[Sample code using SPI IOCTL in C](https://github.com/sckulkarni246/ke-rpi-samples/blob/main/spi-c-ioctl/spi_sysfs_loopback.c)  
[Python example](https://medium.com/vacatronics/getting-started-with-spi-and-raspberry-pi-dddb66116d2b)  