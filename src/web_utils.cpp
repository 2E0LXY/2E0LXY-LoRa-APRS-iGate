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

#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <algorithm>
#include "board_pinout.h"
#include "configuration.h"
#include "gps_utils.h"
#include "lora_utils.h"
#include "ntp_utils.h"
#include "ota_utils.h"
#include "web_utils.h"
#include "display.h"
#include "utils.h"


extern Configuration               Config;
extern uint32_t                    lastBeaconTx;
extern std::vector<ReceivedPacket> receivedPackets;
extern String                      versionDate;
#ifdef HAS_GPS
extern TinyGPSPlus                 gps;
#endif

extern const char web_index_html[] asm("_binary_data_embed_index_html_gz_start");
extern const char web_index_html_end[] asm("_binary_data_embed_index_html_gz_end");
extern const size_t web_index_html_len = web_index_html_end - web_index_html;

extern const char web_style_css[] asm("_binary_data_embed_style_css_gz_start");
extern const char web_style_css_end[] asm("_binary_data_embed_style_css_gz_end");
extern const size_t web_style_css_len = web_style_css_end - web_style_css;

extern const char web_script_js[] asm("_binary_data_embed_script_js_gz_start");
extern const char web_script_js_end[] asm("_binary_data_embed_script_js_gz_end");
extern const size_t web_script_js_len = web_script_js_end - web_script_js;

extern const char web_packet_map_html[] asm("_binary_data_embed_packet_map_html_gz_start");
extern const char web_packet_map_html_end[] asm("_binary_data_embed_packet_map_html_gz_end");
extern const size_t web_packet_map_html_len = web_packet_map_html_end - web_packet_map_html;

extern const char web_packet_map_js[] asm("_binary_data_embed_packet_map_js_gz_start");
extern const char web_packet_map_js_end[] asm("_binary_data_embed_packet_map_js_gz_end");
extern const size_t web_packet_map_js_len = web_packet_map_js_end - web_packet_map_js;

extern const char web_diagnostics_html[] asm("_binary_data_embed_diagnostics_html_gz_start");
extern const char web_diagnostics_html_end[] asm("_binary_data_embed_diagnostics_html_gz_end");
extern const size_t web_diagnostics_html_len = web_diagnostics_html_end - web_diagnostics_html;

extern const char web_diagnostics_js[] asm("_binary_data_embed_diagnostics_js_gz_start");
extern const char web_diagnostics_js_end[] asm("_binary_data_embed_diagnostics_js_gz_end");
extern const size_t web_diagnostics_js_len = web_diagnostics_js_end - web_diagnostics_js;

extern const char web_bootstrap_css[] asm("_binary_data_embed_bootstrap_css_gz_start");
extern const char web_bootstrap_css_end[] asm("_binary_data_embed_bootstrap_css_gz_end");
extern const size_t web_bootstrap_css_len = web_bootstrap_css_end - web_bootstrap_css;

extern const char web_bootstrap_js[] asm("_binary_data_embed_bootstrap_js_gz_start");
extern const char web_bootstrap_js_end[] asm("_binary_data_embed_bootstrap_js_gz_end");
extern const size_t web_bootstrap_js_len = web_bootstrap_js_end - web_bootstrap_js;

// Declare external symbols for the embedded image data
extern const unsigned char favicon_data[] asm("_binary_data_embed_favicon_png_gz_start");
extern const unsigned char favicon_data_end[] asm("_binary_data_embed_favicon_png_gz_end");
extern const size_t favicon_data_len = favicon_data_end - favicon_data;


namespace WEB_Utils {

    AsyncWebServer server(80);

    const char firmwareUpdatePage[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>2E0LXY Firmware Update</title><link rel="stylesheet" href="/style.css">
<style>
body{margin:0;background:#061321;color:#e8f3ff;font-family:system-ui,sans-serif}.update-shell{max-width:880px;margin:0 auto;padding:24px}
.update-nav{display:flex;gap:10px;flex-wrap:wrap;margin-bottom:28px}.update-nav a{padding:11px 16px;border:1px solid #284b66;border-radius:10px;color:#dcecff;text-decoration:none}.update-nav a.active{border-color:#2c8cff;background:#0d3158}
.update-card{padding:28px;border:1px solid #284b66;border-radius:16px;background:#0a1c2c}.version-grid{display:grid;grid-template-columns:1fr 1fr;gap:14px;margin:20px 0}.version-box{padding:16px;border:1px solid #24435d;border-radius:12px;background:#0c2437}.version-box small,.muted{display:block;color:#9bb9d2}.version-box strong{display:block;margin-top:5px;font-size:1.25rem}
.status{margin:18px 0;padding:14px;border-radius:10px;background:#102b40}.status.good{color:#58d68d}.status.available{color:#ffd166}.status.error{color:#ff7b86}
.actions{display:flex;gap:12px;flex-wrap:wrap}.actions button,.actions a{padding:12px 17px;border:1px solid #2c8cff;border-radius:9px;background:#1476d4;color:white;font-weight:700;text-decoration:none;cursor:pointer}.actions button:disabled{opacity:.45;cursor:not-allowed}.actions a{background:transparent}
.warning{margin-top:22px;padding:14px;border-left:3px solid #ffd166;background:#17293a;color:#cfdfed}@media(max-width:650px){.version-grid{grid-template-columns:1fr}}
</style></head><body><main class="update-shell">
<nav class="update-nav"><a href="/">Configuration</a><a href="/received-packets">Packets</a><a href="/packet-map">RF Map</a><a href="/diagnostics">Diagnostics</a><a class="active" href="/firmware-update">Update</a></nav>
<section class="update-card"><h1>Firmware update</h1><p class="muted">Checks the official 2E0LXY GitHub release and installs the Heltec V3.2 OTA image.</p>
<div class="version-grid"><div class="version-box"><small>Installed</small><strong id="current">CURRENT_VERSION</strong></div><div class="version-box"><small>Latest release</small><strong id="latest">Checking…</strong></div></div>
<div id="status" class="status">Contacting GitHub…</div><div class="actions"><button id="install" disabled>Install latest firmware</button><button id="check">Check again</button><a href="/update">Manual firmware upload</a></div>
<div class="warning"><strong>Before updating:</strong> keep the iGate powered and connected to Wi-Fi. Configuration is retained. The device verifies the firmware write before rebooting.</div>
</section></main><script>
const current='v1.1.0',api='https://api.github.com/repos/2E0LXY/2E0LXY-LoRa-APRS-iGate/releases/latest';let asset=null;
const el=id=>document.getElementById(id),parts=v=>v.replace(/^[^0-9]*/,'').split('.').map(n=>parseInt(n)||0);
function newer(a,b){const x=parts(a),y=parts(b);for(let i=0;i<3;i++){if((x[i]||0)!==(y[i]||0))return(x[i]||0)>(y[i]||0)}return false}
async function check(){asset=null;el('install').disabled=true;el('latest').textContent='Checking…';el('status').className='status';el('status').textContent='Contacting GitHub…';
try{const r=await fetch(api,{cache:'no-store',headers:{Accept:'application/vnd.github+json'}});if(!r.ok)throw Error('GitHub returned '+r.status);const d=await r.json();el('latest').textContent=d.tag_name;
asset=(d.assets||[]).find(a=>/Heltec-V3\.2-firmware\.bin$/i.test(a.name));if(!asset)throw Error('Compatible Heltec V3.2 OTA image is not attached to this release');
if(newer(d.tag_name,current)){el('status').className='status available';el('status').textContent='Update available: '+d.name;el('install').disabled=false}else{el('status').className='status good';el('status').textContent='This iGate is already running the latest release.'}}
catch(e){el('status').className='status error';el('status').textContent='Update check failed: '+e.message}}
async function install(){if(!asset||!confirm('Install '+el('latest').textContent+' now? Do not disconnect power.'))return;el('install').disabled=true;
try{el('status').className='status available';el('status').textContent='Downloading firmware from GitHub…';const d=await fetch(asset.browser_download_url);if(!d.ok)throw Error('download failed ('+d.status+')');const blob=await d.blob();
el('status').textContent='Preparing device…';let r=await fetch('/ota/start?mode=firmware');if(!r.ok)throw Error('device rejected update start');
const form=new FormData();form.append('firmware',blob,asset.name);el('status').textContent='Installing firmware… Keep power connected.';r=await fetch('/ota/upload',{method:'POST',body:form});if(!r.ok)throw Error(await r.text());
el('status').className='status good';el('status').textContent='Update installed. The iGate is rebooting…';setTimeout(()=>location.href='/',12000)}
catch(e){el('status').className='status error';el('status').textContent='Update failed: '+e.message;el('install').disabled=false}}
el('check').onclick=check;el('install').onclick=install;check();
</script></body></html>)HTML";

    void handleFirmwareUpdate(AsyncWebServerRequest *request) {
        if(Config.webadmin.active && !request->authenticate(Config.webadmin.username.c_str(), Config.webadmin.password.c_str()))
            return request->requestAuthentication();
        String page = FPSTR(firmwareUpdatePage);
        page.replace("CURRENT_VERSION", versionDate);
        request->send(200, "text/html", page);
    }

    void handleNotFound(AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(404, "text/plain", "Not found");
        response->addHeader("Cache-Control", "max-age=3600");
        request->send(response);
    }

    void handleStatus(AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "OK");
    }

    void handleTimeJson(AsyncWebServerRequest *request) {
        if(Config.webadmin.active && !request->authenticate(Config.webadmin.username.c_str(), Config.webadmin.password.c_str()))
            return request->requestAuthentication();

        JsonDocument data;
        data["dateTime"] = NTP_Utils::getDateTime();
        data["time"] = NTP_Utils::getFormatedTime();
        data["synchronized"] = NTP_Utils::isTimeSet();
        data["server"] = Config.ntp.server;
        data["gmtCorrection"] = Config.ntp.gmtCorrection;

        String buffer;
        serializeJson(data, buffer);
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", buffer);
        response->addHeader("Cache-Control", "no-store");
        request->send(response);
    }

    void handleGpsJson(AsyncWebServerRequest *request) {
        if(Config.webadmin.active && !request->authenticate(Config.webadmin.username.c_str(), Config.webadmin.password.c_str()))
            return request->requestAuthentication();

        JsonDocument data;
        #ifdef HAS_GPS
            GPS_Utils::getData();
            const bool receiving = GPS_Utils::hasReceivedData();
            const uint32_t byteAge = GPS_Utils::lastByteAgeMs();
            const bool streamCurrent = receiving && byteAge < 5000;

            data["supported"] = true;
            data["enabled"] = Config.beacon.gpsActive;
            data["detected"] = receiving;
            data["streamCurrent"] = streamCurrent;
            data["fixValid"] = gps.location.isValid() && gps.location.age() < 10000;
            data["latitude"] = gps.location.isValid() ? gps.location.lat() : 0.0;
            data["longitude"] = gps.location.isValid() ? gps.location.lng() : 0.0;
            data["altitudeMetres"] = gps.altitude.isValid() ? gps.altitude.meters() : 0.0;
            data["speedKmph"] = gps.speed.isValid() ? gps.speed.kmph() : 0.0;
            data["courseDegrees"] = gps.course.isValid() ? gps.course.deg() : 0.0;
            data["satellites"] = gps.satellites.isValid() ? gps.satellites.value() : 0;
            data["hdop"] = gps.hdop.isValid() ? gps.hdop.hdop() : 0.0;
            data["locationAgeMs"] = gps.location.isValid() ? gps.location.age() : UINT32_MAX;
            data["lastByteAgeMs"] = receiving ? byteAge : UINT32_MAX;
            data["charactersProcessed"] = gps.charsProcessed();
            data["sentencesPassed"] = gps.passedChecksum();
            data["sentencesFailed"] = gps.failedChecksum();
            data["utcValid"] = gps.date.isValid() && gps.time.isValid();
            if (gps.date.isValid() && gps.time.isValid()) {
                char utc[32];
                snprintf(utc, sizeof(utc), "%04d-%02d-%02d %02d:%02d:%02d UTC",
                    gps.date.year(), gps.date.month(), gps.date.day(),
                    gps.time.hour(), gps.time.minute(), gps.time.second());
                data["utc"] = utc;
            } else {
                data["utc"] = "Waiting for GPS time";
            }
            #ifdef GPS_BAUDRATE
                data["baud"] = GPS_BAUDRATE;
            #else
                data["baud"] = 9600;
            #endif
            data["rxPin"] = GPS_TX;
            data["txPin"] = GPS_RX;
        #else
            data["supported"] = false;
            data["enabled"] = false;
            data["detected"] = false;
            data["streamCurrent"] = false;
            data["fixValid"] = false;
            data["utc"] = "GPS is not compiled for this board";
        #endif

        String buffer;
        serializeJson(data, buffer);
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", buffer);
        response->addHeader("Cache-Control", "no-store");
        request->send(response);
    }

    void handleHome(AsyncWebServerRequest *request) {
        if(Config.webadmin.active && !request->authenticate(Config.webadmin.username.c_str(), Config.webadmin.password.c_str()))
            return request->requestAuthentication();

        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", (const uint8_t*)web_index_html, web_index_html_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    }

    void handleFavicon(AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "image/x-icon", (const uint8_t*)favicon_data, favicon_data_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    }

    void handleReadConfiguration(AsyncWebServerRequest *request) {
        if(Config.webadmin.active && !request->authenticate(Config.webadmin.username.c_str(), Config.webadmin.password.c_str()))
            return request->requestAuthentication();

        File file = SPIFFS.open("/igate_conf.json");

        String fileContent;
        while(file.available()){
            fileContent += String((char)file.read());
        }

        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", fileContent);
        response->addHeader("Cache-Control", "no-store");
        request->send(response);
    }

    void handleReceivedPackets(AsyncWebServerRequest *request) {
        if(Config.webadmin.active && !request->authenticate(Config.webadmin.username.c_str(), Config.webadmin.password.c_str()))
            return request->requestAuthentication();

        JsonDocument data;

        for (int i = 0; i < receivedPackets.size(); i++) {
            data[i]["packetTime"] = receivedPackets[i].packetTime;
            data[i]["direction"]  = receivedPackets[i].direction;
            data[i]["packet"]     = receivedPackets[i].packet;
            data[i]["RSSI"]       = receivedPackets[i].RSSI;
            data[i]["SNR"]        = receivedPackets[i].SNR;
        }

        String buffer;

        serializeJson(data, buffer);

        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", buffer);
        response->addHeader("Cache-Control", "no-store");
        request->send(response);
    }

    void handlePacketMap(AsyncWebServerRequest *request) {
        if(Config.webadmin.active && !request->authenticate(Config.webadmin.username.c_str(), Config.webadmin.password.c_str()))
            return request->requestAuthentication();

        AsyncWebServerResponse *response = request->beginResponse(
            200, "text/html", (const uint8_t*)web_packet_map_html, web_packet_map_html_len
        );
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    }

    void handlePacketMapScript(AsyncWebServerRequest *request) {
        if(Config.webadmin.active && !request->authenticate(Config.webadmin.username.c_str(), Config.webadmin.password.c_str()))
            return request->requestAuthentication();

        AsyncWebServerResponse *response = request->beginResponse(
            200, "text/javascript", (const uint8_t*)web_packet_map_js, web_packet_map_js_len
        );
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", "no-cache");
        request->send(response);
    }

    void handleDiagnostics(AsyncWebServerRequest *request) {
        if(Config.webadmin.active && !request->authenticate(Config.webadmin.username.c_str(), Config.webadmin.password.c_str()))
            return request->requestAuthentication();
        AsyncWebServerResponse *response = request->beginResponse(
            200, "text/html", (const uint8_t*)web_diagnostics_html, web_diagnostics_html_len
        );
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", "no-cache");
        request->send(response);
    }

    void handleDiagnosticsScript(AsyncWebServerRequest *request) {
        if(Config.webadmin.active && !request->authenticate(Config.webadmin.username.c_str(), Config.webadmin.password.c_str()))
            return request->requestAuthentication();
        AsyncWebServerResponse *response = request->beginResponse(
            200, "text/javascript", (const uint8_t*)web_diagnostics_js, web_diagnostics_js_len
        );
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", "no-cache");
        request->send(response);
    }

    void handleDiagnosticsJson(AsyncWebServerRequest *request) {
        if(Config.webadmin.active && !request->authenticate(Config.webadmin.username.c_str(), Config.webadmin.password.c_str()))
            return request->requestAuthentication();
        JsonDocument data;
        const RadioDiagnostics& stats = LoRa_Utils::diagnostics();
        data["uptimeSeconds"] = millis() / 1000;
        data["freeHeap"] = ESP.getFreeHeap();
        data["wifiRssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
        data["radio"]["rxValid"] = stats.rxValid;
        data["radio"]["rxDuplicates"] = stats.rxDuplicates;
        data["radio"]["rxCrcErrors"] = stats.rxCrcErrors;
        data["radio"]["rxOtherErrors"] = stats.rxOtherErrors;
        data["radio"]["txSuccess"] = stats.txSuccess;
        data["radio"]["txErrors"] = stats.txErrors;
        data["radio"]["recoveryAttempts"] = stats.recoveryAttempts;
        data["radio"]["recoveryAccepted"] = stats.recoveryAccepted;
        data["radio"]["profileSwitches"] = stats.profileSwitches;
        data["profileScanner"]["enabled"] = Config.loramodule.scanActive;
        data["profileScanner"]["secondaryActive"] = LoRa_Utils::secondaryProfileActive();
        data["profileScanner"]["frequency"] = Config.loramodule.scanFreq;
        data["profileScanner"]["spreadingFactor"] = Config.loramodule.scanSpreadingFactor;
        data["profileScanner"]["codingRate"] = Config.loramodule.scanCodingRate4;
        data["profileScanner"]["bandwidth"] = Config.loramodule.scanSignalBandwidth;
        data["profileScanner"]["interval"] = Config.loramodule.scanInterval;
        data["profileScanner"]["dwell"] = Config.loramodule.scanDwell;
        JsonArray stations = data["heardStations"].to<JsonArray>();
        for (const HeardStationDiagnostic& station : LoRa_Utils::heardStations()) {
            JsonObject item = stations.add<JsonObject>();
            item["callsign"] = station.callsign;
            item["packets"] = station.packets;
            item["lastRssi"] = station.lastRssi;
            item["minRssi"] = station.minRssi;
            item["maxRssi"] = station.maxRssi;
            item["avgRssi"] = station.avgRssi;
            item["lastSnr"] = station.lastSnr;
            item["minSnr"] = station.minSnr;
            item["maxSnr"] = station.maxSnr;
            item["avgSnr"] = station.avgSnr;
            item["lastFreqError"] = station.lastFreqError;
            item["lastHeard"] = station.lastHeard;
        }
        String buffer;
        serializeJson(data, buffer);
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", buffer);
        response->addHeader("Cache-Control", "no-store");
        request->send(response);
    }

    void handleWiFiScan(AsyncWebServerRequest *request) {
        if(Config.webadmin.active && !request->authenticate(Config.webadmin.username.c_str(), Config.webadmin.password.c_str()))
            return request->requestAuthentication();

        int found = WiFi.scanComplete();
        if (found == WIFI_SCAN_FAILED) {
            WiFi.scanNetworks(true, true);
            AsyncWebServerResponse *response = request->beginResponse(202, "application/json", "{\"status\":\"scanning\"}");
            response->addHeader("Cache-Control", "no-store");
            request->send(response);
            return;
        }
        if (found == WIFI_SCAN_RUNNING) {
            AsyncWebServerResponse *response = request->beginResponse(202, "application/json", "{\"status\":\"scanning\"}");
            response->addHeader("Cache-Control", "no-store");
            request->send(response);
            return;
        }
        JsonDocument data;
        JsonArray networks = data.to<JsonArray>();
        std::vector<String> seen;
        for (int i = 0; i < found; i++) {
            String ssid = WiFi.SSID(i);
            if (ssid.length() == 0 || std::find(seen.begin(), seen.end(), ssid) != seen.end()) continue;
            seen.push_back(ssid);
            JsonObject network = networks.add<JsonObject>();
            network["ssid"] = ssid;
            network["rssi"] = WiFi.RSSI(i);
            network["channel"] = WiFi.channel(i);
            network["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
        }
        WiFi.scanDelete();
        String buffer;
        serializeJson(data, buffer);
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", buffer);
        response->addHeader("Cache-Control", "no-store");
        request->send(response);
    }

    void handleWriteConfiguration(AsyncWebServerRequest *request) {
        Serial.println("Got new Configuration Data from www");

        auto getParamStringSafe = [&](const String& name, const String& defaultValue = "") -> String {
            if (request->hasParam(name, true)) {
                return request->getParam(name, true)->value();
            }
            return defaultValue;
        };

        auto getParamIntSafe = [&](const String& name, int defaultValue = 0) -> int {
            if (request->hasParam(name, true)) {
                return request->getParam(name, true)->value().toInt();
            }
            return defaultValue;
        };

        auto getParamFloatSafe = [&](const String& name, float defaultValue = 0.0) -> float {
            if (request->hasParam(name, true)) {
                return request->getParam(name, true)->value().toFloat();
            }
            return defaultValue;
        };

        auto getParamDoubleSafe = [&](const String& name, double defaultValue = 0.0) -> double {
            if (request->hasParam(name, true)) {
                return request->getParam(name, true)->value().toDouble();
            }
            return defaultValue;
        };

        int networks = getParamIntSafe("wifi.APs");

        Config.wifiAPs = {};

        for (int i = 0; i < networks; i++) {
            WiFi_AP wifiap;
            wifiap.ssid                   = getParamStringSafe("wifi.AP." + String(i) + ".ssid");
            wifiap.password               = getParamStringSafe("wifi.AP." + String(i) + ".password");

            Config.wifiAPs.push_back(wifiap);
        }

        Config.startupDelay                 = getParamIntSafe("startupDelay", Config.startupDelay);

        Config.callsign                     = getParamStringSafe("callsign", Config.callsign);
        Config.tacticalCallsign             = getParamStringSafe("tacticalCallsign", Config.tacticalCallsign);
        Config.wifiAutoAP.enabled           = request->hasParam("wifi.autoAP.enabled", true);
        Config.wifiAutoAP.password          = getParamStringSafe("wifi.autoAP.password", Config.wifiAutoAP.password);
        Config.wifiAutoAP.timeout           = getParamIntSafe("wifi.autoAP.timeout", Config.wifiAutoAP.timeout);

        Config.aprs_is.active               = request->hasParam("aprs_is.active", true);
        if (Config.aprs_is.active) {
            Config.aprs_is.messagesToRF     = request->hasParam("aprs_is.messagesToRF", true);
            Config.aprs_is.objectsToRF      = request->hasParam("aprs_is.objectsToRF", true);
            Config.aprs_is.server           = getParamStringSafe("aprs_is.server", Config.aprs_is.server);
            Config.aprs_is.passcode         = getParamStringSafe("aprs_is.passcode", Config.aprs_is.passcode);
            Config.aprs_is.port             = getParamIntSafe("aprs_is.port", Config.aprs_is.port);
            Config.aprs_is.filter           = getParamStringSafe("aprs_is.filter", Config.aprs_is.filter);
        }

        Config.beacon.interval              = getParamIntSafe("beacon.interval", Config.beacon.interval);
        Config.beacon.sendViaAPRSIS         = request->hasParam("beacon.sendViaAPRSIS", true);
        Config.beacon.sendViaRF             = request->hasParam("beacon.sendViaRF", true);
        Config.beacon.beaconFreq            = getParamIntSafe("beacon.beaconFreq", Config.beacon.beaconFreq);
        Config.beacon.latitude              = getParamDoubleSafe("beacon.latitude", Config.beacon.latitude);
        Config.beacon.longitude             = getParamDoubleSafe("beacon.longitude", Config.beacon.longitude);
        Config.beacon.comment               = getParamStringSafe("beacon.comment", Config.beacon.comment);
        Config.beacon.overlay               = getParamStringSafe("beacon.overlay", Config.beacon.overlay);
        Config.beacon.symbol                = getParamStringSafe("beacon.symbol", Config.beacon.symbol);
        Config.beacon.path                  = getParamStringSafe("beacon.path", Config.beacon.path);

        Config.beacon.statusActive          = request->hasParam("beacon.statusActive", true);
        if (Config.beacon.statusActive) {
            Config.beacon.statusPacket      = getParamStringSafe("beacon.statusPacket", Config.beacon.statusPacket);
        }

        #if defined(HAS_GPS)
            Config.beacon.gpsActive         = request->hasParam("beacon.gpsActive", true);
        #else
            Config.beacon.gpsActive         = false;
        #endif
        Config.beacon.ambiguityLevel        = getParamIntSafe("beacon.ambiguityLevel", Config.beacon.ambiguityLevel);

        Config.personalNote                 = getParamStringSafe("personalNote", Config.personalNote);

        Config.blacklist                    = getParamStringSafe("blacklist", Config.blacklist);

        Config.digi.mode                    = getParamIntSafe("digi.mode", Config.digi.mode);
        Config.digi.ecoMode                 = getParamIntSafe("digi.ecoMode", Config.digi.ecoMode);
        Config.digi.backupDigiMode          = request->hasParam("digi.backupDigiMode", true);

        Config.loramodule.rxActive          = request->hasParam("lora.rxActive", true);
        Config.loramodule.rxFreq            = getParamIntSafe("lora.rxFreq", Config.loramodule.rxFreq);
        Config.loramodule.rxSpreadingFactor = getParamIntSafe("lora.rxSpreadingFactor", Config.loramodule.rxSpreadingFactor);
        Config.loramodule.rxCodingRate4     = getParamIntSafe("lora.rxCodingRate4", Config.loramodule.rxCodingRate4);
        Config.loramodule.rxSignalBandwidth = getParamIntSafe("lora.rxSignalBandwidth", Config.loramodule.rxSignalBandwidth);
        Config.loramodule.txActive          = request->hasParam("lora.txActive", true);
        Config.loramodule.txFreq            = getParamIntSafe("lora.txFreq", Config.loramodule.txFreq);
        Config.loramodule.txSpreadingFactor = getParamIntSafe("lora.txSpreadingFactor", Config.loramodule.txSpreadingFactor);
        Config.loramodule.txCodingRate4     = getParamIntSafe("lora.txCodingRate4", Config.loramodule.txCodingRate4);
        Config.loramodule.txSignalBandwidth = getParamIntSafe("lora.txSignalBandwidth", Config.loramodule.txSignalBandwidth);
        Config.loramodule.power             = getParamIntSafe("lora.power", Config.loramodule.power);

        Config.display.alwaysOn             = request->hasParam("display.alwaysOn", true);
        if (!Config.display.alwaysOn) {
            Config.display.timeout          = getParamIntSafe("display.timeout", Config.display.timeout);
        }
        Config.display.turn180              = request->hasParam("display.turn180", true);

        Config.battery.sendInternalVoltage          = request->hasParam("battery.sendInternalVoltage", true);
        Config.battery.monitorInternalVoltage       = request->hasParam("battery.monitorInternalVoltage", true);
        if (Config.battery.monitorInternalVoltage) {
            Config.battery.internalSleepVoltage     = getParamFloatSafe("battery.internalSleepVoltage", Config.battery.internalSleepVoltage);
        }

        Config.battery.sendExternalVoltage          = request->hasParam("battery.sendExternalVoltage", true);
        if (Config.battery.sendExternalVoltage) {
            Config.battery.useExternalI2CSensor     = request->hasParam("battery.useExternalI2CSensor", true);
        }
        if (Config.battery.sendExternalVoltage) {
            Config.battery.externalVoltagePin       = getParamIntSafe("battery.externalVoltagePin", Config.battery.externalVoltagePin);
            Config.battery.voltageDividerR1         = getParamFloatSafe("battery.voltageDividerR1", Config.battery.voltageDividerR1);
            Config.battery.voltageDividerR2         = getParamFloatSafe("battery.voltageDividerR2", Config.battery.voltageDividerR2);
        }
        Config.battery.monitorExternalVoltage       = request->hasParam("battery.monitorExternalVoltage", true);
        if (Config.battery.monitorExternalVoltage) {
            Config.battery.externalSleepVoltage     = getParamFloatSafe("battery.externalSleepVoltage", Config.battery.externalSleepVoltage);
        }
        Config.battery.sendVoltageAsTelemetry       = request->hasParam("battery.sendVoltageAsTelemetry", true);

        Config.wxsensor.active                      = request->hasParam("wxsensor.active", true);
        if (Config.wxsensor.active) {
            Config.wxsensor.heightCorrection        = getParamIntSafe("wxsensor.heightCorrection", Config.wxsensor.heightCorrection);
            Config.wxsensor.temperatureCorrection   = getParamFloatSafe("wxsensor.temperatureCorrection", Config.wxsensor.temperatureCorrection);
            Config.beacon.symbol = "_";
        }

        Config.syslog.active                    = request->hasParam("syslog.active", true);
        if (Config.syslog.active) {
            Config.syslog.server                = getParamStringSafe("syslog.server", Config.syslog.server);
            Config.syslog.port                  = getParamIntSafe("syslog.port", Config.syslog.port);
            Config.syslog.logBeaconOverTCPIP    = request->hasParam("syslog.logBeaconOverTCPIP", true);
        }

        Config.tnc.enableServer             = request->hasParam("tnc.enableServer", true);
        Config.tnc.enableSerial             = request->hasParam("tnc.enableSerial", true);
        Config.tnc.acceptOwn                = request->hasParam("tnc.acceptOwn", true);
        Config.tnc.aprsBridgeActive         = request->hasParam("tnc.aprsBridgeActive", true);

        Config.mqtt.active                  = request->hasParam("mqtt.active", true);
        if (Config.mqtt.active) {
            Config.mqtt.server              = getParamStringSafe("mqtt.server", Config.mqtt.server);
            Config.mqtt.topic               = getParamStringSafe("mqtt.topic", Config.mqtt.topic);
            Config.mqtt.username            = getParamStringSafe("mqtt.username", Config.mqtt.username);
            Config.mqtt.password            = getParamStringSafe("mqtt.password", Config.mqtt.password);
            Config.mqtt.port                = getParamIntSafe("mqtt.port", Config.mqtt.port);
            Config.mqtt.beaconOverMqtt      = request->hasParam("mqtt.beaconOverMqtt", true);
        }

        Config.rebootMode                   = request->hasParam("other.rebootMode", true);
        if (Config.rebootMode) {
            Config.rebootModeTime           = getParamIntSafe("other.rebootModeTime", Config.rebootModeTime);
        }

        Config.ota.username                 = getParamStringSafe("ota.username", Config.ota.username);
        Config.ota.password                 = getParamStringSafe("ota.password", Config.ota.password);

        Config.webadmin.active              = request->hasParam("webadmin.active", true);
        if (Config.webadmin.active) {
            Config.webadmin.username        = getParamStringSafe("webadmin.username", Config.webadmin.username);
            Config.webadmin.password        = getParamStringSafe("webadmin.password", Config.webadmin.password);
        }

        Config.remoteManagement.managers    = getParamStringSafe("remoteManagement.managers", Config.remoteManagement.managers);
        Config.remoteManagement.rfOnly      = request->hasParam("remoteManagement.rfOnly", true);

        Config.ntp.server                   = getParamStringSafe("ntp.server", Config.ntp.server);
        Config.ntp.gmtCorrection            = getParamFloatSafe("ntp.gmtCorrection", Config.ntp.gmtCorrection);

        Config.rememberStationTime          = getParamIntSafe("other.rememberStationTime", Config.rememberStationTime);

        bool saveSuccess = Config.writeFile();

        if (saveSuccess) {
            Serial.println("Configuration saved successfully");
            AsyncWebServerResponse *response = request->beginResponse(302, "text/html", "");
            response->addHeader("Location", "/?success=1");
            request->send(response);

            displayToggle(false);
            delay(500);
            ESP.restart();
        } else {
            Serial.println("Error saving configuration!");
            String errorPage = "<!DOCTYPE html><html><head><title>Error</title></head><body>";
            errorPage += "<h1>Configuration Error:</h1>";
            errorPage += "<p>Couldn't save new configuration. Please try again.</p>";
            errorPage += "<a href='/'>Back</a></body></html>";

            AsyncWebServerResponse *response = request->beginResponse(500, "text/html", errorPage);
            request->send(response);
        }
    }

    void handleAction(AsyncWebServerRequest *request) {
        if(Config.webadmin.active && !request->authenticate(Config.webadmin.username.c_str(), Config.webadmin.password.c_str()))
            return request->requestAuthentication();

        if (!request->hasParam("type", false)) {
            request->send(400, "text/plain", "Missing action type");
            return;
        }

        String type = request->getParam("type", false)->value();

        if (type == "send-beacon") {
            lastBeaconTx = 0;

            request->send(200, "text/plain", "Beacon will be sent in a while");
        } else if (type == "reboot") {
            displayToggle(false);
            ESP.restart();
        } else {
            request->send(404, "text/plain", "Not Found");
        }
    }

    void handleStyle(AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/css", (const uint8_t*)web_style_css, web_style_css_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    }

    void handleScript(AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/javascript", (const uint8_t*)web_script_js, web_script_js_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    }

    void handleBootstrapStyle(AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/css", (const uint8_t*)web_bootstrap_css, web_bootstrap_css_len);
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", "max-age=3600");
        request->send(response);
    }

    void handleBootstrapScript(AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/javascript", (const uint8_t*)web_bootstrap_js, web_bootstrap_js_len);
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", "max-age=3600");
        request->send(response);
    }

    void setup() {
        if (Config.digi.ecoMode == 0) {
            server.on("/", HTTP_GET, handleHome);
            server.on("/received-packets", HTTP_GET, handleHome);
            server.on("/status", HTTP_GET, handleStatus);
            server.on("/time.json", HTTP_GET, handleTimeJson);
            server.on("/gps.json", HTTP_GET, handleGpsJson);
            server.on("/received-packets.json", HTTP_GET, handleReceivedPackets);
            server.on("/packet-map", HTTP_GET, handlePacketMap);
            server.on("/packet-map.js", HTTP_GET, handlePacketMapScript);
            server.on("/diagnostics", HTTP_GET, handleDiagnostics);
            server.on("/diagnostics.js", HTTP_GET, handleDiagnosticsScript);
            server.on("/diagnostics.json", HTTP_GET, handleDiagnosticsJson);
            server.on("/firmware-update", HTTP_GET, handleFirmwareUpdate);
            server.on("/wifi-scan.json", HTTP_GET, handleWiFiScan);
            server.on("/configuration.json", HTTP_GET, handleReadConfiguration);
            server.on("/configuration.json", HTTP_POST, handleWriteConfiguration);
            server.on("/action", HTTP_POST, handleAction);
            server.on("/style.css", HTTP_GET, handleStyle);
            server.on("/script.js", HTTP_GET, handleScript);
            server.on("/bootstrap.css", HTTP_GET, handleBootstrapStyle);
            server.on("/bootstrap.js", HTTP_GET, handleBootstrapScript);
            server.on("/favicon.png", HTTP_GET, handleFavicon);

            OTA_Utils::setup(&server); // Include OTA Updater for WebServer

            server.onNotFound(handleNotFound);

            server.begin();
        }
    }

}
