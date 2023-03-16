#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_CCS811.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "config.h"

Adafruit_BME280 bme;
Adafruit_CCS811 ccs;
WiFiClient espClient;
PubSubClient pubSubClient(espClient);

void reconnectNetwork() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.print("Connecting to network");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("!");
  Serial.println(WiFi.localIP());
}

void reconnectPubSub() {
  if (pubSubClient.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT");
  while (!pubSubClient.connected()) {
    if (pubSubClient.connect("FermentationSupervisor")) {
      Serial.println("!");
    } else {
      Serial.print(".");
    }
    delay(1000);
  }
}

void setup() {
  Serial.begin(9600);
  delay(100);
  Serial.println("Fermentation Supervisor v0");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  delay(500);

  reconnectNetwork();

  pubSubClient.setServer(MQTT_SERVER, 1883);

  reconnectPubSub();

  bme.begin(I2C_ADDR_BME280);
  if (!ccs.begin(I2C_ADDR_CCS811)) {
    Serial.println("Failed to start CCS811.");
  } else {
    Serial.print("Waiting for CCS811 to finish calibration");
    while (!ccs.available()) {
      Serial.print(".");
      delay(500);
    }
    Serial.println("!");
  }

  // We don't need much, so reduce clock speed.
  setCpuFrequencyMhz(80);
}

float measureBrightnessPercent() {
  // Dark shade and direct sunlight cover the entire measurable analog range,
  // so no need to map.
  return (float)analogRead(PIN_LIGHT_SENSOR) / 4096;
}

void publishIntMeasurement(const char *topic, int measurement) {
  static char buffer[7];
  dtostrf(measurement, 6, 2, buffer);
  pubSubClient.publish(topic, buffer);
}

void publishFloatMeasurement(const char *topic, float measurement) {
  String floatStr = String(measurement);
  pubSubClient.publish(topic, floatStr.c_str());
}

void measureBme280() {
  Serial.printf("Brightess: %f%%\n", measureBrightnessPercent() * 100);
  publishFloatMeasurement("fermentation-supervisor/brightness", measureBrightnessPercent());
  Serial.printf("Temperature: %f C\n", bme.readTemperature());
  publishFloatMeasurement("fermentation-supervisor/temperature", bme.readTemperature());
  Serial.printf("Humidity: %f %%\n", bme.readHumidity());
  publishFloatMeasurement("fermentation-supervisor/humidity", bme.readHumidity());
  Serial.printf("Pressure: %f hPa\n", bme.readPressure() / 100);
  publishFloatMeasurement("fermentation-supervisor/pressure", bme.readPressure() / 100);
}

void measureCcs811() {
  int ccsReadStatus = ccs.readData();
  if (ccsReadStatus) {
    Serial.printf("Error %d while reading CO2 sensor.\n", ccsReadStatus);
  } else {
    Serial.printf("CO2: %d ppm\n", ccs.geteCO2());
    publishIntMeasurement("fermentation-supervisor/co2", ccs.geteCO2());
    Serial.printf("TVOC: %d ppb\n", ccs.getTVOC());
    publishIntMeasurement("fermentation-supervisor/tvoc", ccs.getTVOC());
  }
}

void loop() {
  long start = millis();

  reconnectNetwork();
  reconnectPubSub();

  measureBme280();
  measureCcs811();

  long loopTimeSpentMs = millis() - start;
  Serial.printf("Measurement loop took %d ms.\n", loopTimeSpentMs);
  if (loopTimeSpentMs < MEASUREMENT_INTERVAL_MS) {
    delay(MEASUREMENT_INTERVAL_MS - loopTimeSpentMs);
  } else {
    Serial.println("Measurement took too long - skipping delay.");
  }
}