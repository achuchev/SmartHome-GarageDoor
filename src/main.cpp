#include <Arduino.h>
#include <ArduinoJson.h>
#include <MqttClient.h>
#include <FotaClient.h>
#include <ESPWifiClient.h>
#include <RemotePrint.h>
#include "settings.h"

MqttClient *mqttClient    = NULL;
FotaClient *fotaClient    = new FotaClient(DEVICE_NAME);
ESPWifiClient *wifiClient = new ESPWifiClient(WIFI_SSID, WIFI_PASS);
long lastStatusMsgSentAt  = 0;

void changeDoorState() {
  PRINTLN("DOOR: Sending impulse to the relay module.");
  digitalWrite(PIN_RELAY, HIGH);
  delay(RELEY_IMPULSE_TIME);
  digitalWrite(PIN_RELAY, LOW);
}

bool isOpened() {
  bool isOpen = digitalRead(PIN_SENSOR) == LOW;

  PRINT_D("DOOR: State is '");

  if (isOpen) {
    PRINT_D("OPENED");
  } else {
    PRINT_D("CLOSED");
  }
  PRINTLN_D("'.");
  return isOpen;
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  PRINT("MQTT Message arrived in '");
  PRINT(topic);
  PRINTLN("' topic.");

  String payloadString = String((char *)payload);

  // parse the JSON
  const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(8) + 130;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root   = jsonBuffer.parseObject(payload);
  JsonObject& status = root.get<JsonObject&>("status");

  if (strcmp(topic, MQTT_TOPIC_SET) == 0) {
    const char *isOpenedChar = status.get<const char *>("isOpened");

    if (isOpenedChar) {
      bool isOpenedDeserved = false;

      if (strcasecmp(isOpenedChar, "true") == 0) {
        isOpenedDeserved = true;
      }

      if (isOpenedDeserved != isOpened()) {
        changeDoorState();

        // force the status publish
        lastStatusMsgSentAt = 0;
      } else {
        PRINTLN("DOOR: No need to change the door state as it is already set.");
      }
    }
    return;
  }
  PRINT("MQTT: Warning: Unknown topic: ");
  PRINTLN(topic);
}

void setup() {
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);
  pinMode(PIN_SENSOR, INPUT_PULLUP);

  wifiClient->init();
  mqttClient = new MqttClient(MQTT_SERVER,
                              MQTT_SERVER_PORT,
                              DEVICE_NAME,
                              MQTT_USERNAME,
                              MQTT_PASS,
                              MQTT_TOPIC_SET,
                              MQTT_SERVER_FINGERPRINT,
                              mqttCallback);
  fotaClient->init();
}

void publishStatus() {
  long now = millis();

  if (now - lastStatusMsgSentAt < MQTT_PUBLISH_STATUS_INTERVAL) {
    return;
  }
  lastStatusMsgSentAt = now;

  const size_t bufferSize = JSON_ARRAY_SIZE(4) + 5 * JSON_OBJECT_SIZE(1);
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root   = jsonBuffer.createObject();
  JsonObject& status = root.createNestedObject("status");

  status["isOpened"] = isOpened();

  // convert to String
  String outString;
  root.printTo(outString);

  // publish the message
  mqttClient->publish(MQTT_TOPIC_GET, outString);
}

void loop() {
  RemotePrint::instance()->handle();
  fotaClient->loop();
  mqttClient->loop();
  publishStatus();
}
