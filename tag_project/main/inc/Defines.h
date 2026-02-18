/******************************************************************************
 * Copyright © 2025 HUB I4.0 - Universidade do Estado do Amazonas
 * All rights reserved
 *****************************************************************************/

/*
Arquivo de definições gerais do projeto de TAG UWB
*/

#pragma once

#include <Arduino.h>
#include <SPI.h>
#include "DW1000.h"
#include "DW1000Ranging.h"

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
// DW1000 PINS (ESP32 DW1000 UWB)
// ============================================================================
#define DW1000_PIN_RST 27    // Reset pin
#define DW1000_PIN_IRQ 34    // Interrupt Request pin
#define DW1000_PIN_SS 4      // Chip Select pin (SPI)

// ============================================================================
// TAG SELECTION - CONFIGURE THE TAG NUMBER HERE
// ============================================================================
#define DW1000_BOARD_TYPE TAG // TAG
#define TAG_NUMBER 1


// ============================================================================
// SHORT ADDRESSES AND MAC ADDRESSES DEFINITION PER TAG
// ============================================================================
#if TAG_NUMBER == 1
    #define DW1000_TAG_SHORT_ADDRESS 125              // 0x7D:0x00
    #define DW1000_TAG_MAC_ADDRESS "22:EA:82:60:3B:9C"

#elif TAG_NUMBER == 2
    #define DW1000_TAG_SHORT_ADDRESS 126              // 0x7E:0x00
    #define DW1000_TAG_MAC_ADDRESS "22:EA:82:60:3B:9D"

#elif TAG_NUMBER == 3
    #define DW1000_TAG_SHORT_ADDRESS 127              // 0x7F:0x00
    #define DW1000_TAG_MAC_ADDRESS "22:EA:82:60:3B:9E"

#elif TAG_NUMBER == 4
    #define DW1000_TAG_SHORT_ADDRESS 128              // 0x80:0x00
    #define DW1000_TAG_MAC_ADDRESS "22:EA:82:60:3B:9F"

#elif TAG_NUMBER == 5
    #define DW1000_TAG_SHORT_ADDRESS 0x2EC8           // 0x81:0x00
    #define DW1000_TAG_MAC_ADDRESS "34:27:FB:18:2E:C8"

#else
    #error "Invalid TAG_NUMBER."
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


