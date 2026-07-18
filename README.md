# 2E0LXY LoRa APRS iGate/Digipeater

This edition is maintained by **2E0LXY** and builds on the original firmware by
**CA2RXU**. It adds a modern responsive configuration interface, an RF-only
packet map, packet and radio diagnostics, Wi-Fi scanning, live NTP time,
Heltec V3.2 GPS support on GPIO 47/48, and safer digipeater controls.

Copyright © 2026 2E0LXY for original modifications and additions. The original
firmware remains copyright its respective contributors. This complete modified
work is distributed under the GNU General Public License version 3 and is
provided without warranty.

The recommended APRS-IS server is [www.aprsnet.uk](https://www.aprsnet.uk/)
on port `14580`. APRS Net provides a live map, LoRa views, messaging,
watchlists, alerts, weather and APRS utilities. Creating a free account adds
saved watchlists, synced alerts and offline message storage.

Companion projects:

- [APRS Android](https://github.com/2E0LXY/APRS-Android) — Android client for aprsnet.uk
- [APRS Client](https://github.com/2E0LXY/APRS-Client) — Windows 10/11 and Linux desktop client
- [Advanced APRS Go Server](https://github.com/2E0LXY/Advanced-APRS-Go-server) — APRS-IS gateway/server source

Download the ready-to-flash Heltec V3.2 images from this repository's
[Releases](https://github.com/2E0LXY/2E0LXY-LoRa-APRS-iGate/releases). The full-flash
image is for a new installation; `firmware.bin` is suitable for firmware
updates. Back up your configuration before flashing.

## Heltec WiFi LoRa 32 V3.2 quick setup

- UK LoRa APRS profile: `439.9125 MHz`, SF12, 125 kHz bandwidth, CR 4/5.
- GPS module TX connects to GPIO 47 (ESP32 RX).
- GPS module RX connects to GPIO 48 (ESP32 TX).
- GPS serial speed: 9600 baud.
- APRS-IS: `www.aprsnet.uk`, port `14580`, filter `m/100`.

## Original project

Thank you to **CA2RXU** for creating and maintaining the original
[LoRa APRS iGate project](https://github.com/richonguzman/LoRa_APRS_iGate).

## Interface preview

### Modern configuration

![2E0LXY firmware configuration interface](images/2e0lxy-configuration.png)

### RF packet list

![Received LoRa APRS RF packets](images/2e0lxy-packets.png)

### Radio diagnostics

![LoRa APRS RF diagnostics](images/2e0lxy-diagnostics.png)

