#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <SPI.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "Adafruit_SHT31.h"

// Wi-Fi credentials: replace locally; never commit real credentials.
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// Raspberry Pi MQTT broker (replace with your Pi address if needed).
const char* mqtt_server = "YOUR_RASPBERRY_PI_IP";
const int mqtt_port = 1883;

// OLED SPI pins
#define OLED_MOSI 23
#define OLED_CLK  18
#define OLED_DC   17
#define OLED_CS   5
#define OLED_RST  16

Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_SH1106G display(128, 64, &SPI, OLED_DC, OLED_RST, OLED_CS);

WiFiClient espClient;
PubSubClient mqttClient(espClient);
WebServer server(80);

unsigned long lastPublish = 0;
const unsigned long publishInterval = 5000;

void updateOLED(float tempF, float humidity) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  display.setTextSize(1);
  display.setCursor(38, 0);
  display.println("HOME LAB");
  display.drawLine(0, 10, 127, 10, SH110X_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 18);
  display.print("TEMP");
  display.setTextSize(2);
  display.setCursor(42, 15);
  display.print(tempF, 1);
  display.print(" F");

  display.setTextSize(1);
  display.setCursor(0, 39);
  display.print("HUM");
  display.setTextSize(2);
  display.setCursor(42, 36);
  display.print(humidity, 1);
  display.print(" %");

  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print(mqttClient.connected() ? "MQTT: ONLINE" : "MQTT: OFFLINE");
  display.display();
}

void handleRoot() {
  float tempC = sht31.readTemperature();
  float humidity = sht31.readHumidity();

  if (isnan(tempC) || isnan(humidity)) {
    server.send(500, "text/plain", "Could not read SHT31 sensor.");
    return;
  }

  float tempF = (tempC * 9.0 / 5.0) + 32.0;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='3'>";
  html += "<style>body{font-family:Arial;background:#111;color:white;text-align:center;padding:40px 15px;}";
  html += ".card{background:#222;max-width:350px;margin:auto;padding:30px;border-radius:20px;}";
  html += ".label{font-size:18px;color:#aaa;}.value{font-size:42px;font-weight:bold;margin:10px 0 30px;}";
  html += ".status{font-size:14px;color:#aaa;}</style></head><body><div class='card'>";
  html += "<h1>Home Lab Environment</h1><div class='label'>Temperature</div><div class='value'>";
  html += String(tempF, 1) + " &deg;F</div><div class='label'>Humidity</div><div class='value'>";
  html += String(humidity, 1) + " %</div><div class='status'>SHT31 Online<br>MQTT: ";
  html += mqttClient.connected() ? "Connected" : "Disconnected";
  html += "</div></div></body></html>";

  server.send(200, "text/html", html);
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to Raspberry Pi MQTT...");
    String clientId = "ESP32-";
    clientId += String((uint32_t)ESP.getEfuseMac(), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected!");
      mqttClient.publish("homelab/devices/esp32/status", "online", true);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void publishSensorData() {
  float tempC = sht31.readTemperature();
  float humidity = sht31.readHumidity();

  if (isnan(tempC) || isnan(humidity)) {
    Serial.println("Sensor read failed.");
    return;
  }

  float tempF = (tempC * 9.0 / 5.0) + 32.0;
  char tempString[10];
  char humidityString[10];
  dtostrf(tempF, 1, 1, tempString);
  dtostrf(humidity, 1, 1, humidityString);

  mqttClient.publish("homelab/environment/temperature", tempString);
  mqttClient.publish("homelab/environment/humidity", humidityString);
  updateOLED(tempF, humidity);

  Serial.print("Published -> Temperature: ");
  Serial.print(tempString);
  Serial.print(" F | Humidity: ");
  Serial.print(humidityString);
  Serial.println(" %");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nStarting Home Lab ESP32...");

  SPI.begin(OLED_CLK, -1, OLED_MOSI, OLED_CS);
  if (!display.begin(0, true)) {
    Serial.println("ERROR: OLED failed to start!");
    while (1) delay(10);
  }

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(2);
  display.setCursor(15, 15);
  display.println("HOMELAB");
  display.setTextSize(1);
  display.setCursor(22, 42);
  display.println("STARTING...");
  display.display();

  Wire.begin(21, 22);
  if (!sht31.begin(0x44)) {
    Serial.println("ERROR: SHT31 not found!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(10, 25);
    display.println("SHT31 ERROR");
    display.display();
    while (1) delay(10);
  }

  Serial.println("SHT31 connected!");
  connectWiFi();
  mqttClient.setServer(mqtt_server, mqtt_port);
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web server started!");
}

void loop() {
  if (!mqttClient.connected()) connectMQTT();
  mqttClient.loop();
  server.handleClient();

  if (millis() - lastPublish >= publishInterval) {
    lastPublish = millis();
    publishSensorData();
  }
}
