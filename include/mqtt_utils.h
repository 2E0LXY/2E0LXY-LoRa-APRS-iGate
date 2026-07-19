#ifndef MQTT_UTILS_H_
#define MQTT_UTILS_H_

#include <Arduino.h>

namespace MQTT_Utils {
    void sendToMqtt(const String& packet);
    void publishBeaconToMqtt(const String& beaconPacket);
    void publishTelemetry();
    void connect();
    bool isConnected();
    int state();
    void loop();
    void setup();
}

#endif
