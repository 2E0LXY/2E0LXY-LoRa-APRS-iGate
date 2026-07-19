/* Copyright (C) 2025 Ricardo Guzman - CA2RXU
 * Modified for aprsnet.uk per-member iGate management.
 * Adds: periodic telemetry publish + JSON command handler.
 */

#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "configuration.h"
#include "gps_utils.h"
#include "lora_utils.h"
#include "ota_utils.h"
#include "station_utils.h"
#include "mqtt_utils.h"

extern Configuration  Config;
extern WiFiClient     mqttClient;
extern String         versionDate;
extern uint32_t       lastBeaconTx;

PubSubClient          pubSub;

static uint32_t       lastTelemetryMs = 0;
static const uint32_t TELEMETRY_INTERVAL_MS = 60000; // every 60 s
static uint32_t       lastConnectAttemptMs = 0;
static bool           connectAttempted = false;
static const uint32_t MQTT_RECONNECT_INTERVAL_MS = 30000;


namespace MQTT_Utils {

    static const char* boardName() {
        #if defined(HELTEC_V4)
        return "Heltec WiFi LoRa 32 V4";
        #elif defined(HELTEC_V3_2)
        return "Heltec WiFi LoRa 32 V3.2";
        #elif defined(TTGO_LORA32_V2_1)
        return "LilyGO TTGO LoRa32 V2.1";
        #else
        return "ESP32 LoRa APRS";
        #endif
    }

    // Return a clean topic base without accidental empty path components.
    static String mqttBaseTopic() {
        String base = Config.mqtt.topic;
        base.trim();
        while (base.startsWith("/")) base.remove(0, 1);
        while (base.endsWith("/") && !base.isEmpty()) base.remove(base.length() - 1);
        return base.isEmpty() ? "aprsnet" : base;
    }

    // MQTT authentication accepts callsigns case-insensitively, but topic
    // matching is case-sensitive. Always use the canonical uppercase owner.
    static String mqttOwnerTopic() {
        String owner = Config.mqtt.username;
        owner.trim();
        owner.toUpperCase();
        return owner;
    }

    // ── Forward received LoRa packet to MQTT ──────────────────────────────
    void sendToMqtt(const String& packet) {
        if (!pubSub.connected()) return;
        if (packet.length() <= 3) return;
        const String cleanPacket = packet.substring(3);   // strip LoRa header bytes
        const int separator = cleanPacket.indexOf(">");
        if (separator <= 0) {
            Serial.println("MQTT: ignored packet without a valid source callsign");
            return;
        }
        String sender = cleanPacket.substring(0, separator);
        sender.trim();
        sender.toUpperCase();
        const String topic = mqttBaseTopic() + "/" + mqttOwnerTopic() + "/" + sender;
        if (pubSub.publish(topic.c_str(), cleanPacket.c_str())) {
            Serial.print("MQTT TX: "); Serial.println(topic);
        } else {
            Serial.print("MQTT packet publish failed, state="); Serial.println(pubSub.state());
        }
    }

    // ── Publish iGate's own beacon to MQTT (beaconOverMqtt flag) ─────────
    // Uses a dedicated function because beaconPacket is already a clean APRS
    // string — unlike received LoRa packets it has no 3-byte header to strip.
    void publishBeaconToMqtt(const String& beaconPacket) {
        if (!pubSub.connected()) return;
        const String topic = mqttBaseTopic() + "/" + mqttOwnerTopic()
                             + "/" + Config.callsign + "/beacon";
        if (pubSub.publish(topic.c_str(), beaconPacket.c_str())) {
            Serial.print("MQTT beacon TX: "); Serial.println(topic);
        } else {
            Serial.print("MQTT beacon publish failed, state="); Serial.println(pubSub.state());
        }
    }

    // ── Publish device telemetry to aprsnet server ─────────────────────
    void publishTelemetry() {
        if (!pubSub.connected()) return;

        JsonDocument doc;
        doc["fw"]         = versionDate;
        doc["uptime"]     = millis() / 1000;
        doc["heap"]       = ESP.getFreeHeap();
        doc["wifi_rssi"]  = WiFi.RSSI();
        const RadioDiagnostics& radio = LoRa_Utils::diagnostics();
        doc["rx"]         = radio.rxValid;
        doc["tx"]         = radio.txSuccess;
        doc["aprs_server"] = Config.aprs_is.server;
        doc["board"]       = boardName();
        doc["local_ip"]    = WiFi.localIP().toString();
        doc["region_profile"] = Config.regional.profile;
        doc["profile_confirmed"] = Config.regional.profileConfirmed;
        doc["country_code"] = Config.regional.countryCode;
        doc["hardware_band"] = Config.regional.hardwareBand;
        doc["timezone"] = Config.ntp.timezone;
        doc["distance_unit"] = Config.regional.distanceUnit;
        doc["altitude_unit"] = Config.regional.altitudeUnit;
        doc["speed_unit"] = Config.regional.speedUnit;
        doc["temperature_unit"] = Config.regional.temperatureUnit;
        doc["rx_frequency"] = Config.loramodule.rxFreq;
        doc["tx_frequency"] = Config.loramodule.txFreq;
        doc["spreading_factor"] = Config.loramodule.rxSpreadingFactor;
        doc["bandwidth"] = Config.loramodule.rxSignalBandwidth;
        doc["coding_rate"] = Config.loramodule.rxCodingRate4;
        doc["tx_power"] = Config.loramodule.power;
        doc["mqtt_state"]  = pubSub.state();
        doc["update_state"] = OTA_Utils::remoteUpdateState();
        doc["update_message"] = OTA_Utils::remoteUpdateMessage();

        double latitude = 0.0;
        double longitude = 0.0;
        bool liveFix = false;
        if (GPS_Utils::getCurrentPosition(latitude, longitude, liveFix)) {
            doc["latitude"] = latitude;
            doc["longitude"] = longitude;
            doc["position_source"] = liveFix ? "GPS" : "fixed";
        }

        JsonArray heard = doc["heard"].to<JsonArray>();
        for (const HeardStationDiagnostic& station : LoRa_Utils::heardStations()) {
            JsonObject item = heard.add<JsonObject>();
            item["callsign"] = station.callsign;
            item["packets"] = station.packets;
            item["last_rssi"] = station.lastRssi;
            item["avg_rssi"] = station.avgRssi;
            item["min_rssi"] = station.minRssi;
            item["max_rssi"] = station.maxRssi;
            item["last_snr"] = station.lastSnr;
            item["avg_snr"] = station.avgSnr;
            item["min_snr"] = station.minSnr;
            item["max_snr"] = station.maxSnr;
            item["freq_error"] = station.lastFreqError;
            item["first_uptime"] = station.firstHeardMillis / 1000;
            item["last_uptime"] = station.lastHeardMillis / 1000;
        }
        // Battery voltage if available (external ADC)
        // doc["batt"] = analogRead(PIN_ADC) * 3.3 / 4095.0 * DIVIDER;

        String payload;
        payload.reserve(8192);
        serializeJson(doc, payload);

        const String topic = mqttBaseTopic() + "/" + mqttOwnerTopic()
                             + "/" + Config.callsign + "/telemetry";
        if (pubSub.publish(topic.c_str(), payload.c_str())) {
            Serial.println("MQTT telemetry published");
        } else {
            Serial.print("MQTT telemetry publish failed, state="); Serial.println(pubSub.state());
        }
    }

    // ── Handle command received from aprsnet server ─────────────────────
    void receivedFromMqtt(char* topic, byte* payload, unsigned int length) {
        const String raw = String(payload, length);
        Serial.print("MQTT CMD: "); Serial.println(raw);

        // Check if it's a JSON command from the aprsnet control panel
        JsonDocument doc;
        if (deserializeJson(doc, raw) == DeserializationError::Ok) {
            const String cmd = doc["cmd"] | "";
            if (cmd == "restart") {
                Serial.println("MQTT: reboot command received");
                delay(500);
                ESP.restart();
            } else if (cmd == "beacon") {
                Serial.println("MQTT: force beacon command received");
                // Set lastBeaconTx = 0 to force immediate beacon on next loop
                lastBeaconTx = 0;
            } else if (cmd == "telemetry") {
                publishTelemetry();
            } else if (cmd == "update") {
                Serial.println("MQTT: GitHub firmware update requested");
                publishTelemetry();
                delay(100);
                if (!OTA_Utils::installLatestFromGitHub()) publishTelemetry();
            } else if (cmd == "aprs") {
                const String packetPayload = doc["payload"] | "";
                if (packetPayload.length() > 0) {
                    STATION_Utils::addToOutputPacketBuffer(packetPayload);
                }
            } else if (cmd != "") {
                Serial.print("MQTT: ignored unknown command: "); Serial.println(cmd);
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

        const uint32_t now = millis();
        if (connectAttempted && now - lastConnectAttemptMs < MQTT_RECONNECT_INTERVAL_MS) return;
        connectAttempted = true;
        lastConnectAttemptMs = now;

        pubSub.setServer(Config.mqtt.server.c_str(), Config.mqtt.port);
        Serial.print("MQTT connecting to " + Config.mqtt.server + "...");

        bool connected = (!Config.mqtt.username.isEmpty())
            ? pubSub.connect(Config.callsign.c_str(),
                             Config.mqtt.username.c_str(),
                             Config.mqtt.password.c_str())
            : pubSub.connect(Config.callsign.c_str());

        if (connected) {
            connectAttempted = false;
            Serial.println(" OK");
            // Subscribe to command topic: aprsnet/{owner}/{device}/cmd
            const String cmdTopic = mqttBaseTopic() + "/" + mqttOwnerTopic()
                                    + "/" + Config.callsign + "/cmd";
            if (pubSub.subscribe(cmdTopic.c_str())) {
                Serial.print("MQTT subscribed: "); Serial.println(cmdTopic);
            } else {
                Serial.print("MQTT subscription failed, state="); Serial.println(pubSub.state());
            }
            // Immediately publish telemetry so server knows we're online
            publishTelemetry();
            lastTelemetryMs = now;
        } else {
            Serial.print(" FAILED, state=");
            Serial.print(pubSub.state());
            Serial.println(" (retry in 30 seconds)");
            mqttClient.stop();
        }
    }

    bool isConnected() {
        return pubSub.connected();
    }

    int state() {
        return pubSub.state();
    }

    // ── Called every main loop iteration ─────────────────────────────────
    void loop() {
        if (!Config.mqtt.active) return;
        if (!pubSub.connected()) {
            connect();
            return;
        }
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
        // A full station report can exceed 12 KB once regional and position
        // metadata are present. Reserve enough room for payload, topic and
        // MQTT framing, and report allocation failure rather than silently
        // dropping every telemetry publish.
        if (!pubSub.setBufferSize(24576)) {
            Serial.println("MQTT: could not allocate 24 KB telemetry buffer");
        }
    }

}
