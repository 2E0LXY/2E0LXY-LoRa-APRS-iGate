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

#include "configuration.h"
#include "network_manager.h"
#include "ntp_utils.h"
#include "time.h"


extern      Configuration  Config;
extern      NetworkManager *networkManager;


namespace NTP_Utils {

    static unsigned long lastSetupAttemptMs = 0;

    static String fixedOffsetRule(float hours) {
        const bool localAheadOfUtc = hours >= 0;
        int totalMinutes = static_cast<int>(roundf(fabsf(hours) * 60.0f));
        const int wholeHours = totalMinutes / 60;
        const int minutes = totalMinutes % 60;
        // POSIX TZ offsets have the opposite sign: UTC-1 means UTC+01:00.
        String rule = "UTC";
        rule += localAheadOfUtc ? "-" : "+";
        rule += String(wholeHours);
        if (minutes > 0) {
            rule += ":";
            if (minutes < 10) rule += "0";
            rule += String(minutes);
        }
        return rule;
    }

    bool setup() {
        if (networkManager->isConnected() && Config.digi.ecoMode == 0 && Config.callsign != "NOCALL-10") {
            lastSetupAttemptMs = millis();
            String timezoneRule = Config.ntp.timezoneRule;
            if (timezoneRule.isEmpty()) timezoneRule = fixedOffsetRule(Config.ntp.gmtCorrection);
            setenv("TZ", timezoneRule.c_str(), 1);
            tzset();
            configTime(0, 0, Config.ntp.server.c_str());
            Serial.println("[NTP] Setting up, timezone: " + Config.ntp.timezone +
                           " (" + timezoneRule + ") Server: " + Config.ntp.server);
            return true;
        }
        return false;
    }

    void update() {
        if (!networkManager->isConnected() || Config.digi.ecoMode != 0 || Config.callsign == "NOCALL-10") {
            return;
        }
        // ESP-IDF SNTP refreshes automatically after configTime(). Retry only
        // every 30 seconds while unsynchronised so the main loop and DNS are
        // not flooded if the configured NTP server is unreachable.
        if (!isTimeSet() && (lastSetupAttemptMs == 0 || millis() - lastSetupAttemptMs >= 30000UL)) {
            setup();
        }
    }

    String getFormatedTime() {
        if (!isTimeSet()) return Config.digi.ecoMode == 0 ? "Waiting for NTP" : "DigiEcoMode Active";
        const time_t now = time(nullptr);
        struct tm timeInfo;
        localtime_r(&now, &timeInfo);
        char buffer[12];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeInfo);
        return String(buffer);
    }

    bool isTimeSet() {
        return time(nullptr) > 1577836800; // 2020-01-01 UTC
    }

    String getDateTime() {
        if (!isTimeSet()) return "Waiting for NTP sync";

        const time_t epoch = time(nullptr);
        struct tm timeInfo;
        localtime_r(&epoch, &timeInfo);

        char buffer[40];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %Z", &timeInfo);
        return String(buffer);
    }

}
