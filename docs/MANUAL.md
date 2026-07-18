# 2E0LXY LoRa APRS iGate and Digipeater Manual

Version 1.1.4 - Heltec WiFi LoRa 32 V3.2 - July 2026

## 1. Purpose

This firmware turns a Heltec WiFi LoRa 32 V3.2 into a UK LoRa APRS
receive iGate, APRS-IS connected station and configurable WIDE1/WIDE2
digipeater. It includes a modern browser interface, RF packet map,
diagnostics, Wi-Fi scanning, GPS location and GitHub OTA updates.

## 2. Safety and licence

- An amateur-radio licence and valid callsign are required for RF transmission.
- Check the current UK band plan and local coordination before transmitting.
- Use the minimum RF power that provides reliable local coverage.
- Back up configuration before updating firmware.
- Keep USB power connected throughout installation.
- The firmware is GPLv3 software supplied without warranty.

## 3. Hardware

### Supported release target

The supplied binaries target the Heltec WiFi LoRa 32 V3.2 with ESP32-S3,
SX1262 LoRa radio, 8 MB flash and onboard OLED.

### GPS wiring

| GPS module | Heltec V3.2 |
| --- | --- |
| TX | GPIO 47 (ESP32 RX) |
| RX | GPIO 48 (ESP32 TX) |
| GND | GND |
| Power | Use the voltage required by the GPS module |

The firmware starts at 9600 baud and automatically checks 38400, 115200
and 4800 baud until it receives a checksum-valid NMEA sentence. Confirm
the module voltage before applying power.

## 4. Installation

### Browser installer

1. Open the 2E0LXY GitHub Pages installer in desktop Chrome or Edge.
2. Connect the Heltec directly by USB.
3. Select **Connect and install**.
4. Choose the Heltec serial device.
5. Confirm installation and wait for verification and reboot.

The full installer can erase stored settings. Existing stations should
download a configuration backup first.

### Manual USB flash

Download `2E0LXY-Heltec-V3.2-full-flash.bin` from GitHub Releases and
write it to address `0x0` as an ESP32-S3 image.

### OTA update

Open **Update** in the device interface. The page compares the installed
version with the latest 2E0LXY GitHub release. Installation is enabled
only when a newer compatible Heltec V3.2 OTA image is available.

## 5. First connection

After boot, the iGate tries each saved Wi-Fi network. If none connects,
its recovery access point becomes available. Connect to it and open
`http://192.168.4.1/`. Change all default and recovery passwords before
placing the device in service.

## 6. Recommended UK LoRa APRS profile

| Parameter | Value |
| --- | --- |
| RX frequency | 439.9125 MHz |
| TX frequency | 439.9125 MHz |
| Spreading factor | SF12 |
| Signal bandwidth | 125 kHz |
| Coding rate | 4/5 |
| TX power | Use the minimum needed |

Both ends of an RF link must use matching modulation parameters.

## 7. Station configuration

- **Callsign - SSID:** licensed callsign and station SSID.
- **Tactical callsign:** optional short identity.
- **Beacon comment:** concise public station description.
- **Beacon path:** path for this station's own RF beacon.
- **Symbol:** map symbol for an iGate or digipeater.
- **Latitude/longitude:** fallback only when GPS is not used.
- **GPS beacon:** uses the attached receiver after a valid fix.
- **Position ambiguity:** reduces public coordinate precision.

The live **GPS Receiver** panel separately reports hardware/UART detection
and position-fix validity. It shows satellites, HDOP, coordinates, altitude,
speed, course, GPS UTC time, NMEA checksum totals and data age. Saving the
GPS enable switch restarts the iGate so the UART can be changed safely.

For this third iGate, `2E0LXY-9` is used. Avoid SSID `-10` where it is
already assigned to another station.

## 8. Wi-Fi

Select **Scan nearby Wi-Fi**, choose a network, enter its passphrase and
save. Multiple networks may be stored. The startup delay allows a router
or mobile hotspot time to become available after a power failure.

Never publish a configuration backup: it contains network and management
credentials.

## 9. APRS-IS

Recommended settings:

| Setting | Value |
| --- | --- |
| Server | `www.aprsnet.uk` |
| Port | `14580` |
| Filter | `m/100` |
| Passcode | Callsign-specific APRS-IS passcode |

APRS Net UK provides a live APRS map, LoRa views, messaging, weather,
watchlists, alerts and utilities. A free member account adds persistent
watchlists, synced alerts and offline message storage.

Messages or objects should be gated from APRS-IS to RF only when useful
locally and permitted by the routing safeguards.

## 10. Digipeater

- **Off:** no RF packet repeating.
- **WIDE1 fill-in:** local fill-in operation.
- **WIDE2 + WIDE1:** wider digipeater behaviour.
- **Backup digipeater:** falls back to local repeating if Wi-Fi or
  APRS-IS is unavailable.
- **Eco mode:** reduces power consumption but can disable parts of the
  display, networking or web interface.

Use a conservative beacon interval and path. Excessive repeating creates
collisions and reduces channel capacity.

## 11. RF packets and map

The **Packets** page lists recently received RF packets with local NTP
time, RSSI and SNR. The **RF Map** shows only packets received or
transmitted by this radio and only when a supported APRS position is
present. Internet-only APRS-IS traffic is excluded.

## 12. Diagnostics

The diagnostics page reports:

- Valid RF packets.
- Duplicates suppressed.
- CRC failures and other radio errors.
- Successful transmissions and TX errors.
- Free heap and Wi-Fi RSSI.
- Heard stations with packet count, RSSI, SNR and frequency error.

CRC-failed frames are counted but never repaired, gated or repeated.

## 13. Time

The NTP section displays the actual device time, synchronization state,
server and GMT correction. Use GMT offset `0` in UK winter and `1`
during British Summer Time. Confirm the displayed time after changing
the offset.

## 14. Backup, restore and security

- Download a configuration backup before major changes.
- Store backups privately because they contain passwords.
- Enable web authentication and use a unique password.
- Protect OTA credentials.
- Limit remote management to trusted callsigns and RF-only operation
  where appropriate.
- Do not expose the HTTP interface directly to the public internet.

## 15. Firmware updates

The GitHub updater performs these checks:

1. Read the latest release from the standalone 2E0LXY repository.
2. Confirm that a Heltec V3.2 OTA image is attached.
3. Compare semantic versions.
4. Require operator confirmation.
5. Download and write the firmware.
6. Verify the write and reboot.

Manual ElegantOTA upload remains available as a recovery route.

## 16. Tracker interoperability

The receiver is intended for standard LoRa APRS frames and Base91 APRS
positions using matching radio settings. Battery telemetry, GPS
positions and normal APRS paths are processed by the existing APRS
packet pipeline.

Tracker SmartBeaconing, motion turn detection, tracker push-buttons and
GPS power toggling are transmitter functions and do not belong in a
fixed iGate. Experimental APRS434 18-byte address-compressed frames are
not claimed as compatible until decoding and gating are validated with
captured test vectors.

## 17. Troubleshooting

### Web page is unavailable

Confirm the displayed IP address, Wi-Fi connection and eco mode. Ping the
device, reconnect by USB and inspect serial output at 115200 baud.

### APRS-IS will not connect

Check DNS, server, port, passcode and filter. Confirm the callsign is
valid and the device has internet access.

### No RF packets

Check antenna connection, RX enabled state, frequency and LoRa
parameters. Compare diagnostics counters and verify that another station
uses the same profile.

### GPS does not fix

Confirm TX-to-RX crossover, GPIO 47/48, automatic baud scanning, antenna view of the
sky and adequate module power. A cold start may take several minutes.

### OTA fails

Keep power connected, verify internet access and retry manual firmware
upload. Do not upload the full-flash image through the OTA form.

## 18. Project links

- Source: https://github.com/2E0LXY/2E0LXY-LoRa-APRS-iGate
- Releases: https://github.com/2E0LXY/2E0LXY-LoRa-APRS-iGate/releases
- Installer: https://2e0lxy.github.io/2E0LXY-LoRa-APRS-iGate/
- APRS Net UK: https://www.aprsnet.uk/

## 19. Acknowledgement

Copyright © 2026 2E0LXY for original modifications and additions.

Thank you to CA2RXU for creating the original LoRa APRS iGate project.
Original contributor notices remain in the source where required.
