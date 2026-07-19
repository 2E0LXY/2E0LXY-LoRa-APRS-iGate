# Regional profiles

The firmware uses one board-specific binary worldwide. Region, radio channel,
timezone and display units are configuration data rather than separate firmware
forks.

## Safe setup order

1. Select the physical radio hardware band fitted to the board.
2. Apply a regional preset or select **Custom / locally coordinated**.
3. Confirm the RX and TX channel with the local LoRa APRS network.
4. Set the licensed callsign, APRS-IS credentials and station location.
5. Leave LoRa TX and digipeating disabled until the channel, path and legal
   power have been confirmed.
6. Save and inspect **Diagnostics → Regional radio profile**.

Existing UK installations migrate to the UK profile without changing their
stored frequency, power, APRS-IS, Wi-Fi, callsign, beacon or digipeater
settings. A new installation starts with LoRa TX, RX beaconing and digipeating
disabled.

The machine-readable pack is
[`regional-profiles.json`](../data_embed/regional-profiles.json). It has a
schema version, review date and source link so changes can be reviewed
independently from firmware code.

## Included presets

- **United Kingdom:** 439.9125 MHz, SF12, 125 kHz, CR 4/5,
  `www.aprsnet.uk`, Europe/London automatic daylight saving.
- **IARU Region 1 common:** 433.775 MHz, SF12, 125 kHz, CR 4/5,
  `rotate.aprs2.net`. A 433.900 MHz TX split is not enabled automatically and
  must be locally coordinated.
- **Custom:** preserves operator-entered values and requires explicit
  confirmation.

No universal North American or worldwide APRS frequency is supplied. Those
installations must use the applicable national band plan and local frequency
coordinator. Do not treat LoRaWAN 868/915 MHz channel plans as LoRa APRS
profiles.
