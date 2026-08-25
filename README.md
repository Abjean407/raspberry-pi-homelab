# Raspberry Pi IoT HomeLab

A hands-on embedded systems and IoT project built around a Raspberry Pi 4, ESP32, MQTT, Python, Docker, InfluxDB 3, and a local OLED display.

The current system monitors temperature and humidity from an SHT31 sensor connected to an ESP32. The ESP32 displays the readings locally on a 1.3-inch SH1106 OLED, serves a local web page, and publishes telemetry over Wi-Fi using MQTT. A Raspberry Pi runs Mosquitto, a Python MQTT-to-InfluxDB bridge, InfluxDB 3 Core, and InfluxDB Explorer for persistent time-series storage and visualization.

## Current architecture

```text
                     SHT31
                       |
                       | I2C
                       v
                     ESP32
              +--------+--------+
              |        |        |
              v        v        v
          SPI OLED   Web UI    MQTT
                               |
                               v
                    Mosquitto on Raspberry Pi
                               |
                               v
                    Python MQTT -> InfluxDB bridge
                               |
                               v
                        InfluxDB 3 Core
                               |
                               v
                    InfluxDB Explorer dashboard
```

## What is working

- ESP32 reads live temperature and humidity from an Adafruit SHT31-D over I2C (GPIO21 SDA / GPIO22 SCL).
- Inland 1.3-inch 128x64 OLED is hardware-verified and displays live temperature, humidity, and MQTT status over SPI.
- ESP32 serves a local environmental status web page.
- ESP32 publishes temperature and humidity every five seconds over MQTT.
- Raspberry Pi runs a Mosquitto MQTT broker.
- A Python service subscribes to MQTT and writes timestamped readings into InfluxDB 3.
- Temperature and humidity are stored as time-series data.
- InfluxDB Core and Explorer run in Docker with persistent storage.
- The Python bridge is managed by systemd.
- InfluxDB Explorer displays current and historical environmental readings.
- The system has been recovered successfully from an InfluxDB WAL corruption event without losing the main database.

## Technology stack

| Layer | Technology |
| --- | --- |
| Sensor | Adafruit SHT31-D |
| Microcontroller | ESP32 |
| Sensor bus | I2C |
| Local display | Inland 1.3-inch 128x64 SH1106 OLED |
| Display bus | SPI |
| Messaging | MQTT / Mosquitto |
| Edge server | Raspberry Pi 4 |
| OS | Raspberry Pi OS / Linux |
| Bridge | Python + Paho MQTT + Requests |
| Database | InfluxDB 3 Core |
| Visualization | InfluxDB Explorer |
| Containers | Docker |
| Service management | systemd |
| Administration | SSH |

## ESP32 pin map

### SHT31 (I2C)

| Function | ESP32 GPIO |
| --- | --- |
| SDA | GPIO21 |
| SCL | GPIO22 |
| Power | 3.3V breadboard rail |
| Ground | GND breadboard rail |

### OLED (SPI)

| OLED | ESP32 |
| --- | --- |
| CLK | GPIO18 |
| MOSI | GPIO23 |
| RES | GPIO16 |
| DC | GPIO17 |
| CS | GPIO5 |
| VCC | 3.3V breadboard rail |
| GND | GND breadboard rail |

The breadboard rails distribute the ESP32's 3.3V and ground to both the SHT31 and OLED.

## MQTT topics

```text
homelab/environment/temperature
homelab/environment/humidity
homelab/devices/esp32/status
```

## Repository layout

```text
.
├── README.md
├── .gitignore
├── esp32/
│   └── homelab_environment_monitor.ino
├── server/
│   └── mqtt_to_influx.py
├── systemd/
│   └── mqtt-influx-bridge.service
└── docs/
    ├── architecture.md
    ├── troubleshooting.md
    └── roadmap.md
```

## Security

No Wi-Fi passwords, API tokens, SSH keys, or other credentials should be committed to this repository. The public ESP32 sketch contains placeholders for Wi-Fi credentials and the Raspberry Pi address. Real secrets remain only on the local devices.

## Current project phase

### Phase 1 - Environmental sensing - complete

ESP32 + SHT31 temperature/humidity monitoring over I2C, local web interface, and MQTT publishing.

### Phase 2 - Raspberry Pi data platform - complete

Mosquitto, Python bridge, Dockerized InfluxDB 3, persistent storage, systemd startup, and dashboarding.

### OLED local display - complete

A 1.3-inch SH1106 OLED is connected to the ESP32 over SPI. The display starts with a HomeLab boot screen and then continuously shows live temperature, humidity, and MQTT connection status. The OLED reuses the same SHT31 readings that are published to MQTT rather than reading the sensor independently.

### Phase 3 - Soil monitoring - in progress

The next integration is an Adafruit STEMMA Soil Sensor (I2C capacitive moisture sensor). It will share the ESP32 I2C bus with the SHT31, publish soil telemetry over MQTT, and add soil data to InfluxDB and the dashboard.

### Future phases

- Calibrated soil moisture monitoring
- Automated irrigation with safe pump control
- Reservoir-level monitoring
- Lighting and ventilation control
- Automatic Wi-Fi/MQTT reconnection on the ESP32
- Secure remote dashboard access
- Multi-zone germination and grow-tent support
- Grafana or another persistent cross-device dashboard

## Lessons learned

This project has provided practical experience with Linux services, Docker networking, persistent volumes, MQTT, Python, I2C, SPI, time-series databases, system logs, SSH administration, breadboard power distribution, embedded displays, troubleshooting, and embedded systems architecture.

A major recovery exercise involved diagnosing an InfluxDB 3 Core startup failure caused by zero-byte WAL files after an unclean shutdown. The database directory was backed up, dependent writes were stopped, the invalid empty WAL files were removed, InfluxDB was restarted, and sensor ingestion was restored. See `docs/troubleshooting.md`.

## Status

Active development. Environmental sensing, local OLED visualization, MQTT telemetry, persistent InfluxDB storage, and dashboarding are operational. Soil monitoring is the next hardware integration.
