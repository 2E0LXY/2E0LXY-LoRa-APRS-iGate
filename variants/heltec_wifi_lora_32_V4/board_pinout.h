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

#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    //  LoRa Radio
    #define HAS_SX1262
    #define HAS_TCXO
    #define RADIO_SCLK_PIN          9
    #define RADIO_MISO_PIN          11
    #define RADIO_MOSI_PIN          10
    #define RADIO_CS_PIN            8
    #define RADIO_RST_PIN           12
    #define RADIO_DIO1_PIN          14
    #define RADIO_BUSY_PIN          13
    #define RADIO_WAKEUP_PIN        RADIO_DIO1_PIN
    #define GPIO_WAKEUP_PIN         GPIO_SEL_14

    //  I2C
    #define USE_WIRE_WITH_OLED_PINS
    #define SENSOR_I2C_BUS          Wire1
    #define BOARD_I2C_SDA           41
    #define BOARD_I2C_SCL           42

    //  Display
    #define HAS_DISPLAY

    #undef  OLED_SDA
    #undef  OLED_SCL
    #undef  OLED_RST

    #define OLED_SDA                17
    #define OLED_SCL                18
    #define OLED_RST                21
    #define OLED_DISPLAY_HAS_RST_PIN

    //  Aditional Config
    #define INTERNAL_LED_PIN        35
    #define BATTERY_PIN             1

    #define ADC_CTRL_PIN            37
    #define ADC_CTRL_ON_STATE       HIGH
    #define VEXT_CTRL_PIN           36
    #define VEXT_CTRL_ON_STATE      LOW

    //  On-board GPS connector (GPS TX -> MCU GPIO38, GPS RX -> MCU GPIO39)
    #define HAS_GPS
    #define GPS_BAUDRATE            9600
    #define GPS_TX                  38
    #define GPS_RX                  39
    #define GPS_POWER_PIN           34
    #define GPS_POWER_ON_STATE      LOW
    #define GPS_STANDBY_PIN         40
    #define GPS_STANDBY_ON_STATE    HIGH

    //  V4.2 GC1109 / V4.3 KCT8103L RF front end
    #define LORA_PA_POWER           7
    #define LORA_FEM_CSD            2
    #define LORA_GC1109_CPS         46
    #define LORA_KCT8103L_CTX       5


#endif
