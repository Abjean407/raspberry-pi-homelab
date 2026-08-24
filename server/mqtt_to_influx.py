"""MQTT -> InfluxDB bridge used by the HomeLab Raspberry Pi.

This sanitized repository version intentionally does not contain the real
InfluxDB API token. Configure INFLUX_TOKEN locally before running.
"""

import os
from datetime import datetime, timezone

import paho.mqtt.client as mqtt
import requests

MQTT_BROKER = os.getenv("MQTT_BROKER", "localhost")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))

TEMP_TOPIC = "homelab/environment/temperature"
HUMIDITY_TOPIC = "homelab/environment/humidity"

INFLUX_URL = os.getenv("INFLUX_URL", "http://localhost:8181/api/v3/write_lp")
DATABASE = os.getenv("INFLUX_DATABASE", "homelab")
INFLUX_TOKEN = os.getenv("INFLUX_TOKEN", "")

latest_temp = None
latest_humidity = None


def write_environment_to_influx():
    global latest_temp, latest_humidity

    if latest_temp is None or latest_humidity is None:
        return

    if not INFLUX_TOKEN:
        print("INFLUX_TOKEN is not configured; skipping write")
        return

    timestamp = int(datetime.now(timezone.utc).timestamp() * 1_000_000_000)

    line = (
        f"environment,device=esp32 "
        f"temperature_f={latest_temp},"
        f"humidity_percent={latest_humidity} "
        f"{timestamp}"
    )

    params = {"db": DATABASE, "precision": "ns"}
    headers = {
        "Authorization": f"Bearer {INFLUX_TOKEN}",
        "Content-Type": "text/plain",
    }

    try:
        response = requests.post(
            INFLUX_URL,
            params=params,
            headers=headers,
            data=line,
            timeout=5,
        )

        if response.status_code in (200, 204):
            print(f"Saved: {latest_temp} F | {latest_humidity} %")
        else:
            print(f"InfluxDB error {response.status_code}: {response.text}")

    except requests.RequestException as exc:
        print(f"InfluxDB connection error: {exc}")

    latest_temp = None
    latest_humidity = None


def on_connect(client, userdata, flags, reason_code, properties=None):
    print("Connected to MQTT broker")
    client.subscribe(TEMP_TOPIC)
    client.subscribe(HUMIDITY_TOPIC)


def on_message(client, userdata, msg):
    global latest_temp, latest_humidity

    try:
        value = float(msg.payload.decode())

        if msg.topic == TEMP_TOPIC:
            latest_temp = value
            print(f"Temperature received: {value}")
        elif msg.topic == HUMIDITY_TOPIC:
            latest_humidity = value
            print(f"Humidity received: {value}")

        write_environment_to_influx()

    except ValueError:
        print(f"Invalid MQTT value: {msg.payload}")


client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = on_connect
client.on_message = on_message

print("Starting MQTT -> InfluxDB bridge...")
client.connect(MQTT_BROKER, MQTT_PORT, 60)
client.loop_forever()
