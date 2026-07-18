# 2E0LXY LoRa APRS iGate and Digipeater

Firmware for a Heltec WiFi LoRa 32 V3.2 operating as a UK LoRa APRS
iGate, WIDE1/WIDE2 digipeater and GPS-positioned station.

[Web installer](https://2e0lxy.github.io/2E0LXY-LoRa-APRS-iGate/) |
[Firmware downloads](https://github.com/2E0LXY/2E0LXY-LoRa-APRS-iGate/releases/latest) |
[Manual](docs/2E0LXY-LoRa-APRS-iGate-Manual.pdf) |
[APRS Net UK](https://www.aprsnet.uk/)

## Highlights

- UK 439.9125 MHz LoRa APRS profile: SF12, 125 kHz, CR 4/5.
- Simultaneous receive iGate and configurable WIDE1/WIDE2 digipeater.
- GPS location on GPIO 47 RX and GPIO 48 TX at 9600 baud.
- APRS-IS gating through `www.aprsnet.uk:14580`.
- RF packet list and RF-only map with APRS position decoding.
- RF diagnostics, duplicate suppression and strict CRC rejection.
- Heard-station signal statistics: RSSI, SNR and frequency error.
- Wi-Fi scanner and multiple saved network profiles.
- GitHub release checking with confirmed one-click OTA installation.
- Web authentication, configuration backup/restore and remote controls.
- Battery, external voltage, weather sensors, telemetry, MQTT, syslog
  and KISS/TNC bridge support inherited and maintained in this edition.

## Install

The easiest installation method is the
[2E0LXY browser installer](https://2e0lxy.github.io/2E0LXY-LoRa-APRS-iGate/).
Use Chrome or Edge on a computer connected to the Heltec by USB.

For manual installation, download one of the release assets:

- `2E0LXY-Heltec-V3.2-full-flash.bin` - first USB installation at `0x0`.
- `2E0LXY-Heltec-V3.2-firmware.bin` - OTA update that retains configuration.

Back up the configuration before changing firmware and keep USB power
connected until installation completes.

## Quick configuration

| Setting | Recommended UK value |
| --- | --- |
| Callsign | Your callsign with an appropriate SSID |
| APRS-IS server | `www.aprsnet.uk` |
| APRS-IS port | `14580` |
| APRS-IS filter | `m/100` |
| RX/TX frequency | `439912500` Hz |
| Spreading factor | `SF12` |
| Bandwidth | `125 kHz` |
| Coding rate | `4/5` |
| GPS | GPIO 47 RX, GPIO 48 TX, 9600 baud |

## Interface

### Configuration

![2E0LXY configuration interface](images/2e0lxy-configuration.png)

### RF packets

![Received LoRa APRS packets](images/2e0lxy-packets.png)

### RF diagnostics

![LoRa radio diagnostics](images/2e0lxy-diagnostics.png)

## Tracker interoperability audit

The following projects were reviewed as interoperability references:

- [aprs434/lora.tracker](https://github.com/aprs434/lora.tracker)
- [lora-aprs/LoRa_APRS_Tracker](https://github.com/lora-aprs/LoRa_APRS_Tracker)
- [dl9sau/TTGO-T-Beam-LoRa-APRS](https://github.com/dl9sau/TTGO-T-Beam-LoRa-APRS)

See the [full feature audit](docs/FEATURE-AUDIT.md) for the implementation
boundary and the safety rationale for excluded tracker-only or experimental
features.

| Reference capability | 2E0LXY iGate status |
| --- | --- |
| Standard LoRa APRS position packets | Supported |
| Base91-compressed APRS GPS positions | Supported through APRS packet processing |
| Independent RX/TX frequencies and LoRa parameters | Supported |
| GPS position, altitude and ambiguity | Supported for the iGate's beacon |
| Battery/external-voltage telemetry | Supported |
| Wi-Fi configuration and scanning | Supported |
| OLED timeout and power-saving operation | Supported |
| Manual and scheduled station beacon | Supported from the web interface |
| OTA firmware update | Supported, including GitHub release checking |
| Serial KISS/TNC and APRS bridge | Supported |
| Bluetooth KISS/TNC | Not present on this Heltec build; serial/network TNC is used |
| APRS434 experimental address-compressed 18-byte frames | Not advertised as supported; protocol compatibility must be validated before gating |
| Tracker SmartBeaconing and turn detection | Tracker-only; not applicable to a fixed iGate |
| Tracker button/GPS power controls | Tracker-only; not applicable to a fixed iGate |

The receiver never guesses or “repairs” damaged frames. Unsupported or
CRC-failed packets are counted for diagnostics but are not gated or repeated.

## Related 2E0LXY software

- [APRS Net UK](https://www.aprsnet.uk/) - live map, messaging, LoRa view,
  weather, alerts and APRS utilities.
- [APRS Android](https://github.com/2E0LXY/APRS-Android)
- [APRS Client for Windows and Linux](https://github.com/2E0LXY/APRS-Client)
- [Advanced APRS Go Server](https://github.com/2E0LXY/Advanced-APRS-Go-server)

## Licence

Copyright © 2026 2E0LXY for original modifications and additions.

This modified work is distributed under the GNU General Public License
version 3 and is supplied without warranty. See [LICENSE](LICENSE).

Original contributor notices are retained in the source where required
by the GNU GPL.
