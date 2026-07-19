/* Copyright (C) 2025 Ricardo Guzman - CA2RXU
 * Modified for aprsnet.uk per-member iGate management.
 * Adds: periodic telemetry publish + JSON command handler.
 */

#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "configuration.h"
#include "station_utils.h"
#include "mqtt_utils.h"

extern Configuration  Config;
extern WiFiClient     mqttClient;
extern String         versionDate;

// Counters exposed from the main loop
int                   mqttPacketsRx = 0;
int                   mqttPacketsTx = 0;

PubSubClient          pubSub;

static uint32_t       lastTelemetryMs = 0;
static const uint32_t TELEMETRY_INTERVAL_MS = 60000; // every 60 s


namespace MQTT_Utils {

    // ── Forward received LoRa packet to MQTT ──────────────────────────────
    void sendToMqtt(const String& packet) {
        if (!pubSub.connected()) return;
        const String cleanPacket = packet.substring(3);
        const String sender      = cleanPacket.substring(0, cleanPacket.indexOf(">"));
        const String topic       = Config.mqtt.topic + "/" + Config.mqtt.username + "/" + sender;
        if (pubSub.publish(topic.c_str(), cleanPacket.c_str())) {
            Serial.print("MQTT TX: "); Serial.println(topic);
        }
    }

    // ── Publish device telemetry to aprsnet server ─────────────────────
    void publishTelemetry() {
        if (!pubSub.connected()) return;

        StaticJsonDocument<256> doc;
        doc["fw"]         = versionDate;
        doc["uptime"]     = millis() / 1000;
        doc["heap"]       = ESP.getFreeHeap();
        doc["wifi_rssi"]  = WiFi.RSSI();
        doc["rx"]         = mqttPacketsRx;
        doc["tx"]         = mqttPacketsTx;
        doc["aprs_server"] = Config.aprs_is.server;
        // Battery voltage if available (external ADC)
        // doc["batt"] = analogRead(PIN_ADC) * 3.3 / 4095.0 * DIVIDER;

        char payload[256];
        serializeJson(doc, payload);

        const String topic = Config.mqtt.topic + "/" + Config.mqtt.username
                             + "/" + Config.callsign + "/telemetry";
        pubSub.publish(topic.c_str(), payload);
        Serial.println("MQTT telemetry published");
    }

    // ── Handle command received from aprsnet server ─────────────────────
    void receivedFromMqtt(char* topic, byte* payload, unsigned int length) {
        const String raw = String(payload, length);
        Serial.print("MQTT CMD: "); Serial.println(raw);

        // Check if it's a JSON command from the aprsnet control panel
        StaticJsonDocument<256> doc;
        if (deserializeJson(doc, raw) == DeserializationError::Ok) {
            const String cmd = doc["cmd"] | "";
            if (cmd == "restart") {
                Serial.println("MQTT: reboot command received");
                delay(500);
                ESP.restart();
            } else if (cmd == "beacon") {
                Serial.println("MQTT: force beacon command received");
                // Set lastBeaconTx = 0 to force immediate beacon on next loop
                extern uint32_t lastBeaconTx;
                lastBeaconTx = 0;
            } else if (cmd == "telemetry") {
                publishTelemetry();
            } else if (cmd != "") {
                // Unknown command — pass to APRS output buffer as raw packet
                const String packetPayload = doc["payload"] | "";
                if (packetPayload.length() > 0) {
                    STATION_Utils::addToOutputPacketBuffer(packetPayload);
                }
            }
        } else {
            // Legacy: raw APRS packet (backward compatible)
            STATION_Utils::addToOutputPacketBuffer(raw);
        }
    }

    // ── Connect / reconnect ───────────────────────────────────────────────
    void connect() {
        if (pubSub.connected()) return;
        if (Config.mqtt.server.isEmpty() || Config.mqtt.port <= 0) return;

        pubSub.setServer(Config.mqtt.server.c_str(), Config.mqtt.port);
        Serial.print("MQTT connecting to " + Config.mqtt.server + "...");

        bool connected = (!Config.mqtt.username.isEmpty())
            ? pubSub.connect(Config.callsign.c_str(),
                             Config.mqtt.username.c_str(),
                             Config.mqtt.password.c_str())
            : pubSub.connect(Config.callsign.c_str());

        if (connected) {
            Serial.println(" OK");
            // Subscribe to command topic: aprsnet/{owner}/{device}/cmd
            const String cmdTopic = Config.mqtt.topic + "/" + Config.mqtt.username
                                    + "/" + Config.callsign + "/cmd";
            pubSub.subscribe(cmdTopic.c_str());
            Serial.print("MQTT subscribed: "); Serial.println(cmdTopic);
            // Immediately publish telemetry so server knows we're online
            publishTelemetry();
        } else {
            Serial.println(" FAILED (retry soon)");
        }
    }

    // ── Called every main loop iteration ─────────────────────────────────
    void loop() {
        if (!Config.mqtt.active) return;
        if (!pubSub.connected()) return;
        pubSub.loop();
        // Periodic telemetry
        if (millis() - lastTelemetryMs > TELEMETRY_INTERVAL_MS) {
            lastTelemetryMs = millis();
            publishTelemetry();
        }
    }

    void setup() {
        if (!Config.mqtt.active) return;
        pubSub.setClient(mqttClient);
        pubSub.setCallback(receivedFromMqtt);
        pubSub.setBufferSize(512);
    }

}
