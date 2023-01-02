# Getting Started

- [Stack Architecture](#stack-architecture)
- [RPi hardware configuration (for MCP3008)](#rpi-hardware-configuration--for-mcp3008-)
- [Software installation](#software-installation)

## Stack Architecture

![Stack Architecture](stack-architecture.svg)

This stack is composed with :

- ADC Monitor provide an API to get all input values for [Widgetlords Pi-SPi-8AI](https://widgetlords.com/products/pi-spi-8ai-raspberry-pi-analog-input-interface) boards with MCP3008 chip. Monitor is written in C,
  with two tasks:
    - A reading loop to update internally all inputs values,
    - A TCP socket API server available on port 3000 to expose all inputs values in JSON for external service,
- A basic WebSocket Gateway available on port 8888, written in JavaScript and runnable with node.JS,
- A lightweight Web Server, with only one page to expose the HTML/Javascript and WebSocket Client interface.

The TCP socket API server accepts only one client: the `WebSocket Gateway` which more easily provides multi-client mode.  
The API provide only one method, `get/all` to get all values.

<table>
<tr>
<th>
Query
</th>
<th>
Response
</th>
</tr>
<tr>
<td valign="top">
<pre lang="json">
{
    "topic": "get/all",
    "payload": {}
}
</pre>
</td>
<td valign="top">
<pre lang="json">
{
    "status": 200,
    "timestamp": &lt;timestamp>
    "payload": {
        "adc": {
            "i": [&lt;index>],
            "v": [&lt;value>]
        },
        "sw_version": "&lt;version>"
    }
}
</pre>
</td>
</tr>
</table>

The field `payload` takes an object with the `i` and `v` fields, which are two arrays:

- Array `i` contains all the variables indexes
- Array `v` contains all the variables values

Once connected, the TCP socket API server automatically sends all (but only) updated inputs. Then, you have to associate the indexes with the values contained in the two arrays.

## RPi hardware configuration (for MCP3008)

1. Enable the SPI interface, with the command:

```bash
$ sudo raspi-config
```

Then, select `Interface Options` > `SPI` > `Yes`

2. Edit the `/boot/config.txt` file add to dtoverlay (for each boards):

```
dtoverlay=mcp3008,spi0-0-present
```

Then `reboot` your Raspberry

3. Check if the SPI devices are properly mounted with the command:

```bash
$ dmesg | grep mcp
```

```bash
$ ls /dev | grep spidev
```

## Software installation

Run the following commands:

```bash
$ git clone https://github.com/a-tortevois/widgetlords-pi-spi-8ai.git
$ cd widgetlords-pi-spi-8ai
$ chmod +x run.sh
$ ./run.sh --install
$ systemctl start adc_monitor.service
$ systemctl start adc_monitor_websocket_gateway.service
$ systemctl start adc_monitor_web_server.service
$ systemctl list-units --type=service | grep adc_monitor
```