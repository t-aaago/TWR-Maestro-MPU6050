/******************************************************************************
 * Copyright © 2025 HUB I4.0 - Universidade do Estado do Amazonas
 * All rights reserved
 *****************************************************************************/

/*
Arquivo de definições gerais do projeto de âncora UWB TWR
*/

#pragma once

#include "Arduino.h"
#include <SPI.h>
#include <WiFi.h>
#include <cmath>
#include "DW1000Ranging.h"
#include "DW1000.h"
#include "mqtt_client.h"

// ============================================================================
// SERIAL CONFIGURATION
// ============================================================================
#define SERIAL_BAUD 115200

// ============================================================================
// SPI PINS
// ============================================================================
#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define SPI_SS 5

// ============================================================================
// DW1000 PINS (ESP32 DW1000 twr)
// ============================================================================
#define DW1000_PIN_RST 27    // Reset pin
#define DW1000_PIN_IRQ 34    // Interrupt Request pin
#define DW1000_PIN_SS 4      // Chip Select pin (SPI)

// ============================================================================
// ANCHOR SELECTION - CONFIGURE THE ANCHOR NUMBER HERE
// ============================================================================
#define DW1000_BOARD_TYPE ANCHOR  // ANCHOR
#define ANCHOR_NUMBER 1

// ============================================================================
// Wifi Configuration
// ============================================================================
#define WIFI_SSID "PCT-GUAMA"           // switch to your network SSID
#define WIFI_PASSWORD "pct@2016"     // switch to your network password

/* #define WIFI_SSID "Tiago"           // switch to your network SSID
#define WIFI_PASSWORD "12345678" */

// ============================================================================
// MQTT Configuration
// ============================================================================
#define MQTT_BROKER "mqtt://test.mosquitto.org:1883"

#define MQTT_PORT 1883
#if ANCHOR_NUMBER == 1
    #define MQTT_TOPIC "uwb/ancora1/data"
#elif ANCHOR_NUMBER == 2
    #define MQTT_TOPIC "uwb/ancora2/data"
#elif ANCHOR_NUMBER == 3
    #define MQTT_TOPIC "uwb/ancora3/data"
#elif ANCHOR_NUMBER == 4
    #define MQTT_TOPIC "uwb/ancora4/data"
#elif ANCHOR_NUMBER == 5
    #define MQTT_TOPIC "uwb/ancora5/data"
#else
    #error "Invalid ANCHOR_NUMBER."
#endif

#define MAX_BUFFER_SIZE 256

// ============================================================================
// SHORT ADDRESSES AND MAC ADDRESSES DEFINITION PER ANCHOR
// ============================================================================
#if ANCHOR_NUMBER == 1
    #define DW1000_ANCHOR_SHORT_ADDRESS 0x2540              // Âncora 1 LABPRO (MSB)
    #define DW1000_ANCHOR_MAC_ADDRESS "C8:2E:18:FB:25:40"

#elif ANCHOR_NUMBER == 2
    #define DW1000_ANCHOR_SHORT_ADDRESS 0x3734              // Âncora 2 LABPRO (MSB)
    #define DW1000_ANCHOR_MAC_ADDRESS "D4:8C:49:A1:37:34"

#elif ANCHOR_NUMBER == 3
    #define DW1000_ANCHOR_SHORT_ADDRESS 0x2850              // Âncora 3 LABPRO (MSB)
    #define DW1000_ANCHOR_MAC_ADDRESS "C8:2E:18:FB:29:50"
#elif ANCHOR_NUMBER == 4
    #define DW1000_ANCHOR_SHORT_ADDRESS 0x3014              // Âncora 4 LABPRO (MSB)
    #define DW1000_ANCHOR_MAC_ADDRESS "D4:8c:49:A1:30:14"

#elif ANCHOR_NUMBER == 5
    #define DW1000_ANCHOR_SHORT_ADDRESS 0x8CD4              // Âncora 5 Tiago (MSB)
    #define DW1000_ANCHOR_MAC_ADDRESS "2C:34:A1:49:8C:D4"
#else
    #error "Invalid ANCHOR_NUMBER."
#endif

// ============================================================================
// TRANSMISSION MODES AND PARAMETERS
// ============================================================================
// NOTE: Any change on this parameters will affect the transmission time of the packets
// So if you change this parameters you should also change the response times on the DW1000 library 
// (actual response times are based on experience made with these parameters)
#define DW1000_TX_RATE DW1000.TRX_RATE_6800KBPS
//DW1000.TRX_RATE_110KBPS
//DW1000.TRX_RATE_850KBPS
//DW1000.TRX_RATE_6800KBPS
#define DW1000_TX_FREQ DW1000.TX_PULSE_FREQ_16MHZ
//DW1000.TX_PULSE_FREQ_16MHZ
//DW1000.TX_PULSE_FREQ_64MHZ
#define DW1000_TX_PREAMBLE DW1000.TX_PREAMBLE_LEN_64
//DW1000.TX_PREAMBLE_LEN_64
//DW1000.TX_PREAMBLE_LEN_128
//DW1000.TX_PREAMBLE_LEN_256
//DW1000.TX_PREAMBLE_LEN_512
//DW1000.TX_PREAMBLE_LEN_1024
//DW1000.TX_PREAMBLE_LEN_1536
//DW1000.TX_PREAMBLE_LEN_2048
//DW1000.TX_PREAMBLE_LEN_4096
constexpr byte MODE[] = {DW1000_TX_RATE, DW1000_TX_FREQ, DW1000_TX_PREAMBLE};

// ============================================================================
// TIMEOUTS AND DELAYS
// ============================================================================
#define TASK_MIN_DELAY_MS 1        // Minimum delay to prevent watchdog issues

// ============================================================================
// ANTENNA DELAY PER ANCHOR (TO BE CALIBRATED)
// ============================================================================
uint16_t getAntennaDelayForAnchor(int anchorNumber) {
    switch (anchorNumber) {
        case 1:
            return 16660;
        case 2:
            return 16600;
        case 3:
            return 16566;
        case 4:
            return 16656;
        case 5:
            return 16530;
        default:
            return 16530; // Default value
    }
}

//=============================================================================
// lIMITS FOR DISTANCE CALCULATION
// ============================================================================
#define MIN_DISTANCE_METERS 0.0f   // Minimum valid distance in meters
#define MAX_DISTANCE_METERS 50.0f // Maximum valid distance in meters

//=============================================================================
// NVS
// ============================================================================
#define NVS_WIFI_NAMESPACE "wifi_cred"
#define NVS_WIFI_SSID "ssid"
#define NVS_WIFI_PASS "pass"
#define NVS_READ_WRITE false

