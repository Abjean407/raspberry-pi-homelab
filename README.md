# Raspberry Pi IoT HomeLab

A hands-on embedded systems and IoT project built around a Raspberry Pi 4, ESP32, MQTT, Python, Docker, and InfluxDB 3.

The current system monitors temperature and humidity from an SHT31 sensor connected to an ESP32. The ESP32 publishes telemetry over Wi-Fi using MQTT. A Raspberry Pi runs Mosquitto, a Python MQTT-to-InfluxDB bridge, InfluxDB 3 Core, and InfluxDB Explorer for persistent time-series storage and visualization.

## Current architecture

```text
SHT31 temperature/humidity sensor
            |
            | I2C
            v
          ESP32
            |
            | Wi-Fi / MQTT
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

- ESP32 reads live temperature and humidity from an SHT31 over I2C.
- ESP32 publishes sensor readings to MQTT topics on the local network.
- Raspberry Pi runs a Mosquitto MQTT broker.
- A Python service subscribes to MQTT and writes timestamped readings into InfluxDB 3.
- Temperature and humidity are stored together as time-series records.
- InfluxDB Core and Explorer run in Docker with persistent storage mounted under the Pi user's home directory.
- The Python bridge is managed by `systemd` and automatically restarts on failure or reboot.
- The dashboard includes current temperature, current humidity, temperature history, and humidity history.
- The system has been recovered successfully from an InfluxDB WAL corruption event without losing the main database.

## Technology stack

| Layer | Technology |
| --- | --- |
| Sensor | Adafruit SHT31-D |
| Microcontroller | ESP32 |
| Sensor bus | I2C |
| Messaging | MQTT / Mosquitto |
| Edge server | Raspberry Pi 4 |
| OS | Raspberry Pi OS / Linux |
| Bridge | Python + Paho MQTT + Requests |
| Database | InfluxDB 3 Core |
| Visualization | InfluxDB Explorer |
| Containers | Docker |
| Service management | systemd |
| Administration | SSH |

## MQTT topics

```text
homelab/environment/temperature
homelab/environment/humidity
```

The bridge combines the latest temperature and humidity readings into one InfluxDB record.

## Repository layout

```text
.
├── README.md
├── .gitignore
├── esp32/
│   └── README.md
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

No Wi-Fi passwords, API tokens, SSH keys, or other credentials should ever be committed to this repository. The included Python bridge uses a placeholder for the InfluxDB token. Store real secrets only on the device running the service.

## Current project phase

### Phase 1 - Environmental sensing - complete

ESP32 + SHT31 temperature/humidity monitoring over I2C and MQTT.

### Phase 2 - Raspberry Pi data platform - complete

Mosquitto, Python bridge, Dockerized InfluxDB 3, persistent storage, systemd startup, and dashboarding.

### Phase 3 - Soil monitoring - in progress

The next integration is an Adafruit STEMMA Soil Sensor (I2C capacitive moisture sensor). It will share the ESP32 I2C bus with the SHT31, publish soil telemetry over MQTT, and add soil data to InfluxDB and the dashboard.

### Future phases

- Calibrated soil moisture monitoring
- Automated irrigation with safe pump control
- Reservoir-level monitoring
- Local OLED status display
- Lighting and ventilation control
- Automatic Wi-Fi/MQTT reconnection on the ESP32
- Secure remote dashboard access
- Multi-zone germination and grow-tent support
- Grafana or another persistent cross-device dashboard

## Lessons learned

This project has provided practical experience with Linux services, Docker networking, persistent volumes, MQTT, Python, I2C, time-series databases, system logs, SSH administration, troubleshooting, and embedded systems architecture.

A major recovery exercise involved diagnosing an InfluxDB 3 Core startup failure caused by zero-byte WAL files after an unclean shutdown. The database directory was backed up, dependent writes were stopped, the invalid empty WAL files were removed, InfluxDB was restarted, and sensor ingestion was restored. See [`docs/troubleshooting.md`](docs/troubleshooting.md).

## Status

Active development. The environmental monitoring pipeline is operational and continuously collecting data while new sensing and automation features are added.
