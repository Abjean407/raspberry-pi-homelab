# Raspberry Pi IoT HomeLab

A hands-on embedded systems and IoT project built around a Raspberry Pi 4, ESP32, MQTT, Python, Docker, InfluxDB 3, environmental sensors, and a local OLED display.

The current ESP32 system monitors temperature, humidity, and raw capacitive soil moisture. It displays the readings locally on a 1.3-inch SH1106 OLED, serves a local web page, and publishes telemetry over Wi-Fi using MQTT. A Raspberry Pi runs Mosquitto, a Python MQTT-to-InfluxDB bridge, InfluxDB 3 Core, and InfluxDB Explorer for persistent time-series storage and visualization.

## Current architecture

```text
              SHT31                 Soil Sensor
        temperature/humidity       capacitive moisture
                 |                       |
                 +---------- I2C --------+
                             |
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

- ESP32 reads live temperature and humidity from an Adafruit SHT31-D over I2C.
- Adafruit STEMMA I2C capacitive soil sensor is hardware-verified at address `0x36` and returns live raw moisture readings.
- SHT31 and soil sensor successfully share the ESP32 I2C bus on GPIO21 SDA / GPIO22 SCL.
- Inland 1.3-inch 128x64 SH1106 OLED displays temperature, humidity, raw soil moisture, and MQTT status over SPI.
- ESP32 serves a local plant-monitor web page containing all three environmental readings.
- ESP32 publishes temperature, humidity, and raw soil moisture every five seconds over MQTT.
- Raspberry Pi runs a Mosquitto MQTT broker.
- A Python service subscribes to MQTT and writes environmental telemetry into InfluxDB 3.
- Temperature and humidity historical storage/dashboarding are operational; soil persistence is the next server-side integration step.
- InfluxDB Core and Explorer run in Docker with persistent storage.
- The Python bridge is managed by systemd.
- The system has been recovered successfully from an InfluxDB WAL corruption event without losing the main database.

## Technology stack

| Layer | Technology |
| --- | --- |
| Environmental sensor | Adafruit SHT31-D |
| Soil sensor | Adafruit STEMMA I2C Capacitive Soil Sensor |
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

### Shared I2C bus

| Device / Function | ESP32 connection |
| --- | --- |
| SHT31 SDA | GPIO21 |
| SHT31 SCL | GPIO22 |
| Soil sensor SDA (white) | GPIO21 |
| Soil sensor SCL (green) | GPIO22 |
| Sensor power | 3.3V breadboard rail |
| Sensor ground | GND breadboard rail |

The SHT31 uses I2C address `0x44`. The soil sensor uses `0x36`.

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

The breadboard distributes 3.3V, ground, SDA, and SCL so both I2C sensors can share the ESP32 bus.

## MQTT topics

```text
homelab/environment/temperature
homelab/environment/humidity
homelab/environment/soil_moisture_raw
homelab/devices/esp32/status
```

Soil moisture is intentionally published as a raw capacitive value until the probe is calibrated in the actual growing medium. The project does not claim that the raw reading is a scientific volumetric water-content percentage.

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

A 1.3-inch SH1106 OLED is connected to the ESP32 over SPI and displays live environmental readings and MQTT connection status.

### Phase 3 - Soil monitoring - ESP32 integration complete

The Adafruit STEMMA soil sensor is connected at I2C address `0x36` alongside the SHT31 at `0x44`. Hardware communication has been verified, the probe responds to changes in its capacitive environment, and the combined ESP32 firmware reads and displays raw soil moisture. The firmware also publishes `homelab/environment/soil_moisture_raw` over MQTT.

Remaining Phase 3 work is to extend the Raspberry Pi MQTT-to-InfluxDB bridge for the new soil topic, store the readings historically, add soil data to the dashboard, and later calibrate the raw values against the actual growing medium.

### Phase 4 - Automated irrigation - planned

The next hardware-control phase will add a water reservoir, low-voltage pump, safe ESP32 pump driver such as a logic-level MOSFET module, tubing, and appropriate safety logic. Pump control will not be based on a single sensor sample; the design will use validated thresholds, timing limits, cooldowns, and other safeguards before unattended watering is enabled.

### Future phases

- Calibrated relative soil moisture monitoring
- Automated irrigation with safe pump control
- Reservoir-level monitoring
- Lighting and ventilation control
- Secure remote dashboard access
- Multi-zone germination and grow-tent support
- Grafana or another persistent cross-device dashboard

## Lessons learned

This project has provided practical experience with Linux services, Docker networking, persistent volumes, MQTT, Python, C++/Arduino, I2C, SPI, time-series databases, system logs, SSH administration, breadboard power/signal distribution, embedded displays, sensor calibration, troubleshooting, and embedded systems architecture.

A major recovery exercise involved diagnosing an InfluxDB 3 Core startup failure caused by zero-byte WAL files after an unclean shutdown. The database directory was backed up, dependent writes were stopped, the invalid empty WAL files were removed, InfluxDB was restarted, and sensor ingestion was restored. See `docs/troubleshooting.md`.

## Status

Active development. Temperature/humidity monitoring, soil-sensor acquisition, OLED visualization, local web monitoring, MQTT telemetry, and the Raspberry Pi data platform are operational. The next step is adding the new soil MQTT topic to the Python bridge and InfluxDB dashboard, followed by secure remote dashboard access and Phase 4 automated irrigation.
