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

#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <AsyncTCP.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include "configuration.h"
#include "ota_utils.h"
#include "display.h"


extern Configuration        Config;
extern uint32_t             lastScreenOn;
extern bool                 isUpdatingOTA;

unsigned long ota_progress_millis = 0;
static String githubUpdateState = "idle";
static String githubUpdateMessage = "";


namespace OTA_Utils {

    static const char* firmwareAssetName() {
        #if defined(HELTEC_V4)
        return "2E0LXY-Heltec-V4-firmware.bin";
        #elif defined(HELTEC_V3_2)
        return "2E0LXY-Heltec-V3.2-firmware.bin";
        #elif defined(TTGO_LORA32_V2_1)
        return "2E0LXY-LilyGO-LoRa32-V2.1-firmware.bin";
        #else
        return nullptr;
        #endif
    }

    const String& remoteUpdateState() { return githubUpdateState; }
    const String& remoteUpdateMessage() { return githubUpdateMessage; }

    void setup(AsyncWebServer *server) {
        if (Config.ota.username != ""  && Config.ota.password != "") {
            ElegantOTA.begin(server, Config.ota.username.c_str(), Config.ota.password.c_str());
        } else if (Config.webadmin.active) {
            // Do not leave the upload endpoints open when the rest of the web
            // interface is protected and no separate OTA account is configured.
            ElegantOTA.begin(server, Config.webadmin.username.c_str(), Config.webadmin.password.c_str());
        } else {
            ElegantOTA.begin(server);
        }

        ElegantOTA.setAutoReboot(true);
        ElegantOTA.onStart(onOTAStart);
        ElegantOTA.onProgress(onOTAProgress);
        ElegantOTA.onEnd(onOTAEnd);
    }

    void onOTAStart() {
        Serial.println("OTA update started!");
        displayToggle(true);
        lastScreenOn = millis();
        displayShow("", "", "", " OTA update started!", "", "", "", 1000);
        isUpdatingOTA = true;
    }

    void onOTAProgress(size_t current, size_t final) {
        if (millis() - ota_progress_millis > 1000) {
            displayToggle(true);
            lastScreenOn = millis();
            ota_progress_millis = millis();
            Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
            const size_t percent = final > 0 ? (current * 100) / final : 0;
            displayShow("", "", "  OTA Progress : " + String(percent) + "%", "", "", "", "", 100);
        }
    }

    void onOTAEnd(bool success) {
        displayToggle(true);
        lastScreenOn = millis();

        String statusMessage = success ? "OTA update success!" : "OTA update fail!";
        String rebootMessage = success ? "Rebooting ..." : "";

        Serial.println(success ? "OTA update finished successfully!" : "There was an error during OTA update!");
        displayShow("", "", statusMessage, "", rebootMessage, "", "", 4000);

        if (!success) isUpdatingOTA = false;
    }

    bool installLatestFromGitHub() {
        const char* asset = firmwareAssetName();
        if (asset == nullptr) {
            githubUpdateState = "error";
            githubUpdateMessage = "No GitHub OTA image is defined for this board";
            return false;
        }

        githubUpdateState = "downloading";
        githubUpdateMessage = "Downloading the board-specific release from GitHub";
        onOTAStart();

        const String url = String("https://github.com/2E0LXY/2E0LXY-LoRa-APRS-iGate/releases/latest/download/") + asset;
        WiFiClientSecure secureClient;
        // GitHub rotates its certificate chain. The release URL and board asset
        // are fixed here, and Update.end(true) validates the ESP image before
        // changing the boot partition.
        secureClient.setInsecure();
        secureClient.setTimeout(20);

        HTTPClient http;
        http.setConnectTimeout(15000);
        http.setTimeout(30000);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        if (!http.begin(secureClient, url)) {
            githubUpdateState = "error";
            githubUpdateMessage = "Could not start the GitHub download";
            onOTAEnd(false);
            return false;
        }

        const int status = http.GET();
        if (status != HTTP_CODE_OK) {
            githubUpdateState = "error";
            githubUpdateMessage = "GitHub download failed with HTTP " + String(status);
            http.end();
            onOTAEnd(false);
            return false;
        }

        const int contentLength = http.getSize();
        if (!Update.begin(contentLength > 0 ? static_cast<size_t>(contentLength) : UPDATE_SIZE_UNKNOWN)) {
            githubUpdateState = "error";
            githubUpdateMessage = "Not enough OTA partition space";
            http.end();
            onOTAEnd(false);
            return false;
        }

        Update.onProgress([](size_t current, size_t total) {
            OTA_Utils::onOTAProgress(current, total);
        });

        githubUpdateState = "installing";
        githubUpdateMessage = "Writing and validating the new firmware";
        const size_t written = Update.writeStream(http.getStream());
        const bool complete = Update.end(true);
        http.end();

        if (!complete || (contentLength > 0 && written != static_cast<size_t>(contentLength))) {
            githubUpdateState = "error";
            githubUpdateMessage = "Firmware validation failed: " + String(Update.errorString());
            onOTAEnd(false);
            return false;
        }

        githubUpdateState = "success";
        githubUpdateMessage = "Firmware installed; rebooting";
        onOTAEnd(true);
        delay(1500);
        ESP.restart();
        return true;
    }

}
