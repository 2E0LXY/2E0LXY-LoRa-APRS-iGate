#ifndef MQTT_UTILS_H_
#define MQTT_UTILS_H_

#include <Arduino.h>

extern int mqttPacketsRx;
extern int mqttPacketsTx;

namespace MQTT_Utils {
    void sendToMqtt(const String& packet);
    void publishTelemetry();
    void connect();
    void loop();
    void setup();
}

#endif
