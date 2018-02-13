#ifndef SETTING_H
#define SETTING_H

#define DEVICE_NAME "GarageDoor"

#define MQTT_TOPIC_GET "get/garage/door"
#define MQTT_TOPIC_SET "set/garage/door"
#define MQTT_PUBLISH_STATUS_INTERVAL 10000

#define PIN_RELAY D1         // Relay for the Garage door
#define PIN_SENSOR_OPENED D2 // Garage Door MUK for opened door
#define PIN_SENSOR_CLOSED D5 // Garage Door MUK for closed door

#define RELEY_IMPULSE_TIME 2000


#endif // ifndef SETTING_H
