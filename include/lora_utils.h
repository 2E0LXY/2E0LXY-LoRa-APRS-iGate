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

#ifndef LORA_UTILS_H_
#define LORA_UTILS_H_

#include <Arduino.h>
#include <vector>

struct RadioDiagnostics {
    uint32_t rxValid = 0;
    uint32_t rxDuplicates = 0;
    uint32_t rxCrcErrors = 0;
    uint32_t rxOtherErrors = 0;
    uint32_t txSuccess = 0;
    uint32_t txErrors = 0;
    uint32_t recoveryAttempts = 0;
    uint32_t recoveryAccepted = 0;
    uint32_t profileSwitches = 0;
};

struct HeardStationDiagnostic {
    String callsign;
    uint32_t packets = 0;
    int lastRssi = 0;
    int minRssi = 0;
    int maxRssi = 0;
    float avgRssi = 0;
    float lastSnr = 0;
    float minSnr = 0;
    float maxSnr = 0;
    float avgSnr = 0;
    int lastFreqError = 0;
    String lastHeard;
    uint32_t firstHeardMillis = 0;
    uint32_t lastHeardMillis = 0;
};


namespace LoRa_Utils {

    void    setup();
    void    sendNewPacket(const String& newPacket);
    String  receivePacketFromSleep();
    String  receivePacket();
    void    changeFreqTx();
    void    changeFreqRx();
    void    wakeRadio();
    void    sleepRadio();
    void    serviceProfileScanner();
    const RadioDiagnostics& diagnostics();
    const std::vector<HeardStationDiagnostic>& heardStations();
    bool    secondaryProfileActive();

}

#endif
