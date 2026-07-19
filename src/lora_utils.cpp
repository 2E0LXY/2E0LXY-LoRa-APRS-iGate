/* Copyright (C) 2025 Ricardo Guzman - CA2RXU
 *
 * This file is part of LoRa APRS iGate.
 *
 * LoRa APRS iGate is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LoRa APRS iGate is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LoRa APRS iGate. If not, see <https://www.gnu.org/licenses/>.
 */

#include <RadioLib.h>
#include <algorithm>
#include "lora_utils.h"
#include "configuration.h"
#include "network_manager.h"
#include "aprs_is_utils.h"
#include "station_utils.h"
#include "board_pinout.h"
#include "syslog_utils.h"
#include "ntp_utils.h"
#include "display.h"
#include "utils.h"


extern Configuration    Config;
extern NetworkManager   *networkManager;
extern uint32_t         lastRxTime;
extern bool             packetIsBeacon;

extern std::vector<ReceivedPacket> receivedPackets;

bool operationDone   = true;
bool transmitFlag    = true;

#ifdef HAS_SX1262
    SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif
#ifdef HAS_SX1268
    #if defined(LIGHTGATEWAY_1_0) || defined(LIGHTGATEWAY_PLUS_1_0)
        SPIClass loraSPI(FSPI);
        SX1268 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN, loraSPI);
    #else
        SX1268 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
    #endif
#endif
#ifdef HAS_SX1278
    SX1278 radio = new Module(RADIO_CS_PIN, RADIO_BUSY_PIN, RADIO_RST_PIN);
#endif
#ifdef HAS_SX1276
    SX1276 radio = new Module(RADIO_CS_PIN, RADIO_BUSY_PIN, RADIO_RST_PIN);
#endif
#if defined(HAS_LLCC68)         //LLCC68 supports spreading factor only in range of 5-11!
    LLCC68 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif

int rssi, freqError;
float snr;

namespace {
    RadioDiagnostics radioDiagnostics;
    std::vector<HeardStationDiagnostic> heardStationDiagnostics;
    struct DuplicateEntry { uint32_t hash; uint32_t seenAt; };
    std::vector<DuplicateEntry> duplicateEntries;
    bool scanProfileActive = false;
    uint32_t lastScanStart = 0;
    uint32_t scanStartedAt = 0;

    #ifdef HELTEC_V4
    void setupHeltecV4Fem() {
        pinMode(LORA_VFEM_CTRL, OUTPUT);
        digitalWrite(LORA_VFEM_CTRL, HIGH);
        delay(5);
        pinMode(LORA_FEM_CSD, OUTPUT);
        digitalWrite(LORA_FEM_CSD, HIGH);
        pinMode(LORA_V42_FEM_CPS, OUTPUT);
        pinMode(LORA_V43_FEM_CTX, OUTPUT);
        digitalWrite(LORA_V42_FEM_CPS, LOW);
        digitalWrite(LORA_V43_FEM_CTX, LOW);
        Utils::println("Heltec V4 RF front end initialized (V4.2/V4.3)");
    }

    void setHeltecV4TxMode() {
        digitalWrite(LORA_VFEM_CTRL, HIGH);
        digitalWrite(LORA_FEM_CSD, HIGH);
        digitalWrite(LORA_V42_FEM_CPS, HIGH);
        digitalWrite(LORA_V43_FEM_CTX, HIGH);
    }

    void setHeltecV4RxMode() {
        digitalWrite(LORA_VFEM_CTRL, HIGH);
        digitalWrite(LORA_FEM_CSD, HIGH);
        digitalWrite(LORA_V42_FEM_CPS, LOW);
        digitalWrite(LORA_V43_FEM_CTX, LOW);
    }
    #endif

    uint32_t packetHash(const String& packet) {
        uint32_t hash = 2166136261UL;
        for (size_t i = 0; i < packet.length(); i++) {
            hash ^= static_cast<uint8_t>(packet[i]);
            hash *= 16777619UL;
        }
        return hash;
    }

    bool isStructurallyValidLoRaAprsFrame(const String& packet) {
        if (packet.length() < 10 || !packet.startsWith("\x3c\xff\x01")) return false;
        const int separator = packet.indexOf('>', 3);
        const int info = packet.indexOf(':', separator + 1);
        return separator >= 7 && info > separator + 1 && info + 1 < packet.length();
    }

    bool isDuplicatePacket(const String& packet) {
        const uint32_t now = millis();
        const uint32_t hash = packetHash(packet);
        for (auto it = duplicateEntries.begin(); it != duplicateEntries.end();) {
            if (now - it->seenAt > 30000UL) {
                it = duplicateEntries.erase(it);
            } else {
                if (it->hash == hash) return true;
                ++it;
            }
        }
        if (duplicateEntries.size() >= 32) duplicateEntries.erase(duplicateEntries.begin());
        duplicateEntries.push_back({hash, now});
        return false;
    }

    void updateHeardStation(const String& callsign, int stationRssi, float stationSnr, int stationFreqError) {
        auto found = std::find_if(heardStationDiagnostics.begin(), heardStationDiagnostics.end(),
            [&](const HeardStationDiagnostic& item) { return item.callsign == callsign; });
        if (found == heardStationDiagnostics.end()) {
            if (heardStationDiagnostics.size() >= 32) heardStationDiagnostics.erase(heardStationDiagnostics.begin());
            HeardStationDiagnostic item;
            item.callsign = callsign;
            item.minRssi = item.maxRssi = stationRssi;
            item.minSnr = item.maxSnr = stationSnr;
            heardStationDiagnostics.push_back(item);
            found = heardStationDiagnostics.end() - 1;
        }
        found->packets++;
        found->lastRssi = stationRssi;
        found->lastSnr = stationSnr;
        found->lastFreqError = stationFreqError;
        found->minRssi = min(found->minRssi, stationRssi);
        found->maxRssi = max(found->maxRssi, stationRssi);
        found->minSnr = min(found->minSnr, stationSnr);
        found->maxSnr = max(found->maxSnr, stationSnr);
        found->avgRssi += (stationRssi - found->avgRssi) / found->packets;
        found->avgSnr += (stationSnr - found->avgSnr) / found->packets;
        found->lastHeard = NTP_Utils::getFormatedTime();
    }
}


namespace LoRa_Utils {

    void setFlag(void) {
        operationDone = true;
    }

    void setup() {
        #ifdef HELTEC_V4
            setupHeltecV4Fem();
        #endif
        #if defined (LIGHTGATEWAY_1_0) || defined(LIGHTGATEWAY_PLUS_1_0)
            pinMode(RADIO_VCC_PIN,OUTPUT);
            digitalWrite(RADIO_VCC_PIN,HIGH);
            loraSPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
        #else
            SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
        #endif
        float freq = (float)Config.loramodule.rxFreq / 1000000;
        #if defined(RADIO_HAS_XTAL)
            radio.XTAL = true;
        #endif
        int state = radio.begin(freq);
        if (state != RADIOLIB_ERR_NONE) {
            Utils::println("Starting LoRa failed! State: " + String(state));
            while (true);
        }
        #if defined(HAS_SX1262) || defined(HAS_SX1268) || defined(HAS_LLCC68)
            radio.setDio1Action(setFlag);
        #endif
        #if defined(HAS_SX1278) || defined(HAS_SX1276)
            radio.setDio0Action(setFlag, RISING);
        #endif

        /*#ifdef SX126X_DIO3_TCXO_VOLTAGE
            if (radio.setTCXO(float(SX126X_DIO3_TCXO_VOLTAGE)) == RADIOLIB_ERR_NONE) {
                Utils::println("Set LoRa Module TCXO Voltage to:" + String(SX126X_DIO3_TCXO_VOLTAGE));
            } else {
                Utils::println("Set LoRa Module TCXO Voltage failed! State: " + String(state));
                while (true);
        }
         #endif*/

        radio.setSpreadingFactor(Config.loramodule.rxSpreadingFactor);
        radio.setCodingRate(Config.loramodule.rxCodingRate4);
        float signalBandwidth = Config.loramodule.rxSignalBandwidth / 1000;
        radio.setBandwidth(signalBandwidth);
        radio.setCRC(true);

        #if (defined(RADIO_RXEN) && defined(RADIO_TXEN))    // QRP Labs LightGateway has 400M22S (SX1268)
            radio.setRfSwitchPins(RADIO_RXEN, RADIO_TXEN);
        #endif

        /*#ifdef SX126X_DIO2_AS_RF_SWITCH
        radio.setRfSwitchPins(RADIO_RXEN, RADIOLIB_NC);
        radio.setDio2AsRfSwitch(true);
        #endif*/

        #ifdef HAS_1W_LORA  // Ebyte E22 400M30S (SX1268) / 900M30S (SX1262) / Ebyte E220 400M30S (LLCC68)
            state = radio.setOutputPower(Config.loramodule.power); // max value 20dB for 1W modules as they have Low Noise Amp
            radio.setCurrentLimit(140); // to be validated (100 , 120, 140)?
        #endif
        #if defined(HAS_SX1278) || defined(HAS_SX1276)
            state = radio.setOutputPower(Config.loramodule.power); // max value 20dB for 400M30S as it has Low Noise Amp
            radio.setCurrentLimit(100); // to be validated (80 , 100)?
        #endif
        #ifdef HELTEC_V4
            // V4 includes an external PA. Treat the configured value as approximate
            // antenna output. Use a conservative compensation that is safe for
            // both the V4.2 GC1109 and V4.3 KCT8103L front ends.
            state = radio.setOutputPower(constrain(Config.loramodule.power - 12, -9, 17));
            radio.setCurrentLimit(140);
        #elif (defined(HAS_SX1268) || defined(HAS_SX1262)) && !defined(HAS_1W_LORA)
            state = radio.setOutputPower(Config.loramodule.power + 2); // values available: 10, 17, 22 --> if 20 in tracker_conf.json it will be updated to 22.
            radio.setCurrentLimit(140);
        #endif

        #if defined(HAS_SX1262) || defined(HAS_SX1268) || defined(HAS_LLCC68)
            radio.setRxBoostedGainMode(true);
        #endif

        #if defined(HAS_TCXO) && !defined(HAS_1W_LORA)
            radio.setDio2AsRfSwitch();
        #endif
        #ifdef HAS_TCXO
            radio.setTCXO(1.8);
        #endif

        if (state == RADIOLIB_ERR_NONE) {
            Utils::println("init : LoRa Module    ...     done!");
        } else {
            Utils::println("Starting LoRa failed! State: " + String(state));
            while (true);
        }
    }

    void changeFreqTx() {
        scanProfileActive = false;
        float freq = (float)Config.loramodule.txFreq / 1000000;
        radio.setFrequency(freq);
        radio.setSpreadingFactor(Config.loramodule.txSpreadingFactor);
        radio.setCodingRate(Config.loramodule.txCodingRate4);
        float signalBandwidth = Config.loramodule.txSignalBandwidth / 1000;
        radio.setBandwidth(signalBandwidth);
    }

    void changeFreqRx() {
        scanProfileActive = false;
        float freq = (float)Config.loramodule.rxFreq / 1000000;
        radio.setFrequency(freq);
        radio.setSpreadingFactor(Config.loramodule.rxSpreadingFactor);
        radio.setCodingRate(Config.loramodule.rxCodingRate4);
        float signalBandwidth = Config.loramodule.rxSignalBandwidth / 1000;
        radio.setBandwidth(signalBandwidth);
    }

    void sendNewPacket(const String& newPacket) {
        if (!Config.loramodule.txActive) return;

        if (Config.loramodule.txFreq != Config.loramodule.rxFreq) {
            if (!packetIsBeacon || (packetIsBeacon && Config.beacon.beaconFreq == 1)) {
                changeFreqTx();
            }
        }

        #ifdef INTERNAL_LED_PIN
            if (Config.digi.ecoMode != 1) digitalWrite(INTERNAL_LED_PIN, HIGH);     // disabled in Ultra Eco Mode
        #endif
        #ifdef HELTEC_V4
            setHeltecV4TxMode();
        #endif
        int state = radio.transmit("\x3c\xff\x01" + newPacket);
        #ifdef HELTEC_V4
            setHeltecV4RxMode();
        #endif
        transmitFlag = true;
        if (state == RADIOLIB_ERR_NONE) {
            radioDiagnostics.txSuccess++;
            if (Config.digi.ecoMode == 0) {
                if (receivedPackets.size() >= 50) {
                    receivedPackets.erase(receivedPackets.begin());
                }
                ReceivedPacket transmittedPacket;
                transmittedPacket.packetTime = NTP_Utils::getFormatedTime();
                transmittedPacket.direction  = "TX";
                transmittedPacket.packet     = newPacket;
                transmittedPacket.RSSI       = 0;
                transmittedPacket.SNR        = 0;
                receivedPackets.push_back(transmittedPacket);
            }
            if (Config.syslog.active && networkManager->isConnected()) {
                SYSLOG_Utils::log(3, newPacket, 0, 0.0, 0);    // TX
            }
            Utils::print("---> LoRa Packet Tx : ");
            Utils::println(newPacket);
        } else {
            radioDiagnostics.txErrors++;
            Utils::print(F("failed, code "));
            Utils::println(String(state));
        }
        #ifdef INTERNAL_LED_PIN
            if (Config.digi.ecoMode != 1) digitalWrite(INTERNAL_LED_PIN, LOW);      // disabled in Ultra Eco Mode
        #endif
        if (Config.loramodule.txFreq != Config.loramodule.rxFreq) {
            if (!packetIsBeacon || (packetIsBeacon && Config.beacon.beaconFreq == 1)) {
                changeFreqRx();
            }
        }
    }

    String receivePacketFromSleep() {
        String packet = "";
        int state = radio.readData(packet);
        if (state == RADIOLIB_ERR_NONE) {
            Utils::println("<--- LoRa Packet Rx : " + packet.substring(3));
        } else {
            packet = "";
        }
        return packet;
    }

    String receivePacket() {
        String packet = "";
        if (operationDone) {
            operationDone = false;
            if (transmitFlag) {
                radio.startReceive();
                transmitFlag = false;
            } else {
                int state = radio.readData(packet);
                if (state == RADIOLIB_ERR_NONE) {
                    if (packet != "") {
                        if (!isStructurallyValidLoRaAprsFrame(packet)) {
                            radioDiagnostics.rxOtherErrors++;
                            Utils::println("Invalid LoRa APRS frame rejected");
                            packet = "";
                        } else {
                            String sender = packet.substring(3, packet.indexOf('>', 3));
                            if (STATION_Utils::isBlacklisted(sender)) {
                                packet = "";
                            } else {
                                rssi        = radio.getRSSI();
                                snr         = radio.getSNR();
                                freqError   = radio.getFrequencyError();
                                radioDiagnostics.rxValid++;
                                updateHeardStation(sender, rssi, snr, freqError);
                                if (isDuplicatePacket(packet.substring(3))) {
                                    radioDiagnostics.rxDuplicates++;
                                    Utils::println("Duplicate RF packet suppressed: " + sender);
                                    packet = "";
                                    lastRxTime = millis();
                                    return packet;
                                }
                                Utils::println("<--- LoRa Packet Rx : " + packet.substring(3));
                                Utils::println("(RSSI:" + String(rssi) + " / SNR:" + String(snr) + " / FreqErr:" + String(freqError) + ")");

                                if (Config.digi.ecoMode == 0) {
                                    if (receivedPackets.size() >= 50) {
                                        receivedPackets.erase(receivedPackets.begin());
                                    }
                                    ReceivedPacket receivedPacket;
                                    receivedPacket.packetTime = NTP_Utils::getFormatedTime();
                                    receivedPacket.direction  = "RX";
                                    receivedPacket.packet     = packet.substring(3);
                                    receivedPacket.RSSI       = rssi;
                                    receivedPacket.SNR        = snr;
                                    receivedPackets.push_back(receivedPacket);
                                }

                                if (Config.syslog.active && networkManager->isConnected()) {
                                    SYSLOG_Utils::log(1, packet, rssi, snr, freqError); // RX
                                }
                            }
                        }
                        lastRxTime = millis();
                        return packet;
                    }
                } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
                    radioDiagnostics.rxCrcErrors++;
                    radioDiagnostics.recoveryAttempts++;
                    rssi        = radio.getRSSI();
                    snr         = radio.getSNR();
                    freqError   = radio.getFrequencyError();
                    Utils::println(F("CRC error!"));
                    if (Config.syslog.active && networkManager->isConnected()) {
                        SYSLOG_Utils::log(0, packet, rssi, snr, freqError); // CRC
                    }
                    packet = "";
                } else {
                    radioDiagnostics.rxOtherErrors++;
                    Utils::print(F("failed, code "));
                    Utils::println(String(state));
                    packet = "";
                }
            }
        }
        return packet;
    }

    void wakeRadio() {
        radio.startReceive();
    }

    void sleepRadio() {
        radio.sleep();
    }

    void serviceProfileScanner() {
        if (!Config.loramodule.scanActive || Config.loramodule.scanFreq <= 0) return;
        const uint32_t now = millis();
        const uint32_t intervalMs = max(15, Config.loramodule.scanInterval) * 1000UL;
        const uint32_t dwellMs = max(1, Config.loramodule.scanDwell) * 1000UL;
        if (!scanProfileActive && now - lastScanStart >= intervalMs) {
            radio.setFrequency((float)Config.loramodule.scanFreq / 1000000);
            radio.setSpreadingFactor(Config.loramodule.scanSpreadingFactor);
            radio.setCodingRate(Config.loramodule.scanCodingRate4);
            radio.setBandwidth((float)Config.loramodule.scanSignalBandwidth / 1000);
            radio.startReceive();
            scanProfileActive = true;
            scanStartedAt = lastScanStart = now;
            radioDiagnostics.profileSwitches++;
        } else if (scanProfileActive && now - scanStartedAt >= dwellMs) {
            changeFreqRx();
            radio.startReceive();
            scanProfileActive = false;
            radioDiagnostics.profileSwitches++;
        }
    }

    const RadioDiagnostics& diagnostics() { return radioDiagnostics; }
    const std::vector<HeardStationDiagnostic>& heardStations() { return heardStationDiagnostics; }
    bool secondaryProfileActive() { return scanProfileActive; }

}
