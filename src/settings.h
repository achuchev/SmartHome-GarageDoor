#ifndef SETTING_H
#define SETTING_H

#define DEVICE_NAME "GarageDoor"

#define MQTT_TOPIC_GET "get/garage/door"
#define MQTT_TOPIC_SET "set/garage/door"

#define PIN_RELAY D1  // Relay for the Garage door
#define PIN_SENSOR D2 // Garage Door MUK

#define RELEY_IMPULSE_TIME 2000

#define WIFI_SSID "****"
#define WIFI_PASS "****"

#define MQTT_SERVER "****"
#define MQTT_SERVER_PORT 0000
#define MQTT_SERVER_FINGERPRINT \
  "** ** ** ** ** ** ** ** ** ** ** * * * ** ** * ** ** **"
#define MQTT_USERNAME "****"
#define MQTT_PASS "****"
#define MQTT_PUBLISH_STATUS_INTERVAL 10000 // 10 sec

#define ARDUINO_OTA_PASS_HASH "****"
#define ARDUINO_OTA_PORT 0000

#endif // ifndef SETTING_H
