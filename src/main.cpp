#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPWifiClient.h>
#include <MqttClient.h>
#include <FotaClient.h>
#include <RemotePrint.h>
#include "settings.h"

enum DoorState {
  opened,
  closed,
  inProgress
};

ESPWifiClient *wifiClient       = new ESPWifiClient(WIFI_SSID, WIFI_PASS);
MqttClient    *mqttClient       = NULL;
FotaClient    *fotaClient       = new FotaClient(DEVICE_NAME);
long lastStatusMsgSentAt        = 0;
DoorState lastReportedDoorState = DoorState::inProgress;

void changeDoorState() {
  PRINTLN("DOOR: Sending impulse to the relay module.");
  digitalWrite(PIN_RELAY, HIGH);
  delay(RELEY_IMPULSE_TIME);
  digitalWrite(PIN_RELAY, LOW);
}

bool isDoorOpened() {
  return digitalRead(PIN_SENSOR_OPENED) == LOW;
}

bool isDoorClosed() {
  return digitalRead(PIN_SENSOR_CLOSED) == LOW;
}

DoorState getDoorState() {
  PRINT_D("DOOR: State is '");

  if (isDoorOpened()) {
    PRINTLN_D("OPENED'.");
    return DoorState::opened;
  }

  if (isDoorClosed()) {
    PRINTLN_D("CLOSED'.");
    return DoorState::closed;
  }
  PRINTLN_D("IN PROGRESS'.");
  return DoorState::inProgress;
}

const char* getDoorStateAsChar(DoorState doorState) {
  switch (doorState) {
    case DoorState::opened: return "opened";
    case DoorState::closed: return "closed";
    case DoorState::inProgress: return "inProgress";
    default: { PRINTLN_E("DOOR: Unknow door state."); return "error"; }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  PRINT("MQTT Message arrived in '");
  PRINT(topic);
  PRINTLN("' topic.");

  if (strcmp(topic, MQTT_TOPIC_SET) != 0) {
    PRINT("MQTT: Warning: Unknown topic: ");
    PRINTLN(topic);
  }
  String payloadString        = String((char *)payload);
  DoorState deservedDoorState = inProgress;

  if (payloadString.equals("0")) {
    deservedDoorState = DoorState::closed;
  } else if (payloadString.equals("1")) {
    deservedDoorState = DoorState::opened;
  } else {
    // we assume JSON, so let's parse it
    const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(8) + 130;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject& root   = jsonBuffer.parseObject(payload);
    JsonObject& status = root.get<JsonObject&>("status");

    const char *doorStateChar = status.get<const char *>("doorState");

    if (doorStateChar) {
      if (strcasecmp(doorStateChar, "opened") == 0) {
        deservedDoorState = DoorState::opened;
      } else if (strcasecmp(doorStateChar, "closed") == 0) {
        deservedDoorState = DoorState::closed;
      } else if (strcasecmp(doorStateChar, "inProgress") == 0) {
        deservedDoorState = DoorState::inProgress;
      } else {
        PRINT_E("DOOR: Unknown door state '");
        PRINT_E(doorStateChar);
        PRINTLN_E("'.'");
        return;
      }
    }
  }

  if (deservedDoorState == getDoorState()) {
    PRINTLN_W("DOOR: No need to change the door state as it is already set.");
    return;
  }
  changeDoorState();

  // force the status publish
  lastStatusMsgSentAt = 0;
}

void setup() {
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);
  pinMode(PIN_SENSOR_OPENED, INPUT_PULLUP);
  pinMode(PIN_SENSOR_CLOSED, INPUT_PULLUP);

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
  long now                   = millis();
  DoorState currentDoorState = getDoorState();

  if ((lastReportedDoorState == currentDoorState) &&
      (now - lastStatusMsgSentAt < MQTT_PUBLISH_STATUS_INTERVAL)) {
    // Not yet
    return;
  }
  lastStatusMsgSentAt = now;

  const size_t bufferSize = JSON_ARRAY_SIZE(4) + 5 * JSON_OBJECT_SIZE(1);
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root   = jsonBuffer.createObject();
  JsonObject& status = root.createNestedObject("status");

  lastReportedDoorState = currentDoorState;
  status["doorState"]   = getDoorStateAsChar(currentDoorState);

  // convert to String
  String outString;
  root.printTo(outString);

  // publish the message
  mqttClient->publish(MQTT_TOPIC_GET, outString);
}

void loop() {
  wifiClient->reconnectIfNeeded();
  RemotePrint::instance()->handle();
  fotaClient->loop();
  mqttClient->loop();
  publishStatus();
}
