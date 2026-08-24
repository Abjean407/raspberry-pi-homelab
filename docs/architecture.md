# Architecture

## Overview

The HomeLab uses an ESP32 as the edge device and a Raspberry Pi as the central server.

```text
Physical environment
      |
      v
SHT31 sensor
      |
      | I2C
      v
ESP32
      |
      | Wi-Fi / MQTT
      v
Mosquitto broker
      |
      v
Python MQTT bridge
      |
      | InfluxDB line protocol over HTTP
      v
InfluxDB 3 Core
      |
      v
InfluxDB Explorer
```

## Edge device

The ESP32 reads the SHT31 temperature/humidity sensor over I2C. In the current wiring, the standard ESP32 I2C pins are used:

- SDA: GPIO 21
- SCL: GPIO 22
- Sensor power: 3.3 V
- Ground: GND

The ESP32 publishes readings to:

```text
homelab/environment/temperature
homelab/environment/humidity
```

## Raspberry Pi server

The Raspberry Pi provides several services:

- Mosquitto MQTT broker
- Docker Engine
- InfluxDB 3 Core
- InfluxDB Explorer
- Python MQTT-to-InfluxDB bridge
- systemd service management
- SSH administration

## Data flow

1. The SHT31 measures temperature and humidity.
2. The ESP32 publishes each reading over MQTT.
3. Mosquitto receives the MQTT messages on the Pi.
4. `mqtt_to_influx.py` subscribes to both topics.
5. The bridge waits until it has the latest value from each topic.
6. It writes a combined `environment` record to InfluxDB.
7. InfluxDB stores the record with a nanosecond timestamp.
8. Explorer queries the data for current-value and history dashboard cells.

Example line-protocol shape:

```text
environment,device=esp32 temperature_f=76.5,humidity_percent=42.0 <timestamp>
```

## Docker persistence

InfluxDB data is persisted outside the container on the Raspberry Pi host:

```text
/home/anthony/.influxdb/data    -> /var/lib/influxdb3/data
/home/anthony/.influxdb/plugins -> /var/lib/influxdb3/plugins
```

This is important because containers can be restarted or recreated independently from the stored database files.

## Service startup

The MQTT bridge is managed by systemd and configured to restart automatically. Docker containers are configured with restart policies so the stack can return after a normal Raspberry Pi reboot.

## Planned expansion

The same architecture is intentionally expandable. Future I2C devices can share the same SDA/SCL bus when addresses do not conflict.

Planned additions include:

- Adafruit STEMMA Soil Sensor at I2C address `0x36`
- Local OLED display
- Pump/irrigation control through appropriate driver hardware
- Reservoir-level sensor
- Additional ESP32 nodes for separate grow zones
- Secure remote access
