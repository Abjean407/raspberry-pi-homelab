#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <SPI.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_seesaw.h>

// Wi-Fi credentials: replace locally; never commit real credentials.
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// Raspberry Pi MQTT broker: replace locally with the Pi's IP address.
const char* mqtt_server = "YOUR_RASPBERRY_PI_IP";
const int mqtt_port = 1883;

const char* temp_topic = "homelab/environment/temperature";
const char* humidity_topic = "homelab/environment/humidity";
const char* soil_topic = "homelab/environment/soil_moisture_raw";
const char* status_topic = "homelab/devices/esp32/status";

// OLED SPI pins
#define OLED_MOSI 23
#define OLED_CLK 18
#define OLED_DC 17
#define OLED_CS 5
#define OLED_RST 16

Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_seesaw soilSensor;
Adafruit_SH1106G display(128, 64, &SPI, OLED_DC, OLED_RST, OLED_CS);

WiFiClient espClient;
PubSubClient mqttClient(espClient);
WebServer server(80);

float temperatureF = 0.0;
float humidity = 0.0;
uint16_t soilMoistureRaw = 0;

unsigned long lastSensorRead = 0;
const unsigned long sensorInterval = 5000;

void connectWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to Raspberry Pi MQTT...");
    String clientId = "ESP32-PlantMonitor-" + String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str(), status_topic, 0, true, "offline")) {
      Serial.println("connected!");
      mqttClient.publish(status_topic, "online", true);
    } else {
      Serial.print("failed, rc=");
      Serial.println(mqttClient.state());
      delay(3000);
    }
  }
}

void updateOLED() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("HOME LAB");
  display.drawLine(0, 10, 127, 10, SH110X_WHITE);

  display.setCursor(0, 16);
  display.print("TEMP: ");
  display.print(temperatureF, 1);
  display.println(" F");

  display.setCursor(0, 28);
  display.print("HUM : ");
  display.print(humidity, 1);
  display.println(" %");

  display.setCursor(0, 40);
  display.print("SOIL: ");
  display.println(soilMoistureRaw);

  display.setCursor(0, 52);
  display.print("MQTT: ");
  display.println(mqttClient.connected() ? "ONLINE" : "OFFLINE");
  display.display();
}

void readSensors() {
  float tempC = sht31.readTemperature();
  float hum = sht31.readHumidity();

  if (!isnan(tempC)) temperatureF = (tempC * 9.0 / 5.0) + 32.0;
  if (!isnan(hum)) humidity = hum;

  soilMoistureRaw = soilSensor.touchRead(0);

  Serial.print("Temperature: ");
  Serial.print(temperatureF, 1);
  Serial.print(" F | Humidity: ");
  Serial.print(humidity, 1);
  Serial.print(" % | Soil raw: ");
  Serial.println(soilMoistureRaw);
}

void publishSensorData() {
  char tempString[10];
  char humidityString[10];
  char soilString[10];

  dtostrf(temperatureF, 1, 1, tempString);
  dtostrf(humidity, 1, 1, humidityString);
  snprintf(soilString, sizeof(soilString), "%u", soilMoistureRaw);

  mqttClient.publish(temp_topic, tempString);
  mqttClient.publish(humidity_topic, humidityString);
  mqttClient.publish(soil_topic, soilString);

  Serial.println("MQTT data published.");
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='5'>";
  html += "<title>HomeLab Plant Monitor</title>";
  html += "<style>body{font-family:Arial;background:#111;color:#fff;text-align:center;padding:30px;}";
  html += ".card{background:#222;padding:20px;margin:15px auto;border-radius:15px;max-width:400px;}";
  html += ".value{font-size:32px;font-weight:bold;}</style></head><body>";
  html += "<h1>HomeLab Plant Monitor</h1>";
  html += "<div class='card'><h2>Temperature</h2><div class='value'>" + String(temperatureF, 1) + " &deg;F</div></div>";
  html += "<div class='card'><h2>Humidity</h2><div class='value'>" + String(humidity, 1) + " %</div></div>";
  html += "<div class='card'><h2>Soil Moisture</h2><div class='value'>" + String(soilMoistureRaw) + "</div><p>Raw capacitive reading</p></div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nStarting HomeLab Plant Monitor...");

  SPI.begin(OLED_CLK, -1, OLED_MOSI, OLED_CS);
  if (!display.begin(0, true)) {
    Serial.println("ERROR: OLED failed to start!");
    while (1) delay(10);
  }

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(20, 25);
  display.println("HOME LAB");
  display.setCursor(12, 40);
  display.println("Starting...");
  display.display();

  Wire.begin(21, 22);

  if (!sht31.begin(0x44)) {
    Serial.println("ERROR: SHT31 not found!");
    while (1) delay(100);
  }
  Serial.println("SHT31 connected.");

  if (!soilSensor.begin(0x36)) {
    Serial.println("ERROR: Soil sensor not found!");
    while (1) delay(100);
  }
  Serial.println("Soil sensor connected.");

  connectWiFi();
  mqttClient.setServer(mqtt_server, mqtt_port);

  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web server started.");

  readSensors();
  updateOLED();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqttClient.connected()) reconnectMQTT();

  mqttClient.loop();
  server.handleClient();

  unsigned long currentTime = millis();
  if (currentTime - lastSensorRead >= sensorInterval) {
    lastSensorRead = currentTime;
    readSensors();
    publishSensorData();
    updateOLED();
  }
}
