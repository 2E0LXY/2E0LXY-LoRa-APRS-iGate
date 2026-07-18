# Tracker interoperability and feature audit

This document records the review of three established LoRa APRS tracker
projects. It distinguishes compatible radio/APRS capabilities from features
that only make sense on a moving battery-powered tracker. No source code was
copied from the reference projects.

Reviewed projects:

- [aprs434/lora.tracker](https://github.com/aprs434/lora.tracker)
- [lora-aprs/LoRa_APRS_Tracker](https://github.com/lora-aprs/LoRa_APRS_Tracker)
- [dl9sau/TTGO-T-Beam-LoRa-APRS](https://github.com/dl9sau/TTGO-T-Beam-LoRa-APRS)

## Compatibility matrix

| Capability | 2E0LXY status | Notes |
| --- | --- | --- |
| Standard APRS position frames | Supported | Received RF frames can be gated, repeated, listed and mapped. |
| Base91 APRS position compression | Supported | Handled by the APRS packet-processing library. |
| Configurable LoRa frequency, SF, bandwidth and coding rate | Supported | RX and TX profiles are independently configurable. |
| GPS position and altitude | Supported | Live GPS can position the iGate and its own beacon. |
| Position ambiguity | Supported | Available for station beacon configuration. |
| Battery and external-voltage telemetry | Supported | Includes configurable telemetry reporting. |
| Wi-Fi setup and scanning | Supported | Multiple saved networks and click-to-select scanning. |
| OLED timeout and reduced-power display operation | Supported | Intended for unattended iGate operation. |
| Manual and scheduled beacon | Supported | Controlled from the web interface. |
| Web OTA and release discovery | Supported | Includes browser upload and confirmation before install. |
| Serial/network KISS and APRS bridge | Supported | Appropriate for a fixed network iGate. |
| Bluetooth KISS | Not implemented | Serial and network transports are used; BLE would add memory and radio-coexistence cost. |
| Tracker SmartBeaconing and turn detection | Not applicable | A fixed iGate should not vary its beacon rate with speed or heading. |
| Physical tracker transmit button | Not applicable | Manual transmit is available in the authenticated web interface. |
| Tracker GPS power cycling | Not applicable | Continuous location availability is preferred for this mains-powered iGate. |
| Tracker automatic shutdown | Not applicable | Conflicts with unattended iGate availability. |
| APRS434 experimental 18-byte address-compressed frames | Not enabled | This is not standard APRS. Test vectors and network agreement are required before safe gating or repeating. |

## Design decision

“All features” does not mean copying tracker behaviour into an iGate. The
2E0LXY firmware implements the interoperable APRS, radio, GPS, telemetry,
networking, display and maintenance capabilities. Moving-tracker controls are
deliberately excluded, and experimental frames are rejected rather than
guessed, repaired, gated or repeated.

Future protocol additions must include reproducible test vectors, CRC rules,
packet-loop protection and validation against real transmitters before being
enabled on RF or APRS-IS.
