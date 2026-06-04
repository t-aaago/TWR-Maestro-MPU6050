/******************************************************************************
 * Copyright © 2025 HUB I4.0 - Universidade do Estado do Amazonas
 * All rights reserved
 *****************************************************************************/

#pragma once

#include "Arduino.h"
#include <SPI.h>
#include <cmath>
#include <cstdint>
#include "DW1000Ranging.h"
#include "DW1000.h"

// Importação das credenciais isoladas
#include "../secrets/control_config.h"
#include "../secrets/data_config.h"

// ============================================================================
// VERSIONAMENTO E SERIAL
// ============================================================================
inline constexpr char CURRENT_VERSION[] = "V0.2.1";
inline constexpr uint32_t SERIAL_BAUD = 115200U;

// ============================================================================
// PINOUT SPI E DW1000 (ESP32)
// ============================================================================
inline constexpr uint8_t SPI_SCK  = 18U;
inline constexpr uint8_t SPI_MISO = 19U;
inline constexpr uint8_t SPI_MOSI = 23U;
inline constexpr uint8_t SPI_SS   = 5U;

inline constexpr uint8_t DW1000_PIN_RST = 27U;
inline constexpr uint8_t DW1000_PIN_IRQ = 34U;
inline constexpr uint8_t DW1000_PIN_SS  = 4U;

// ============================================================================
// SELEÇÃO DE ÂNCORA (Usado para pré-processamento)
// ============================================================================
#define DW1000_BOARD_TYPE ANCHOR

#ifndef ANCHOR_NUMBER
    #define ANCHOR_NUMBER 1 // Valor padrão de fallback
#endif

// ============================================================================
// CONFIGURAÇÃO WI-FI (Fallback)
// ============================================================================
inline constexpr char WIFI_SSID[] = "DASHING";
inline constexpr char WIFI_PASSWORD[] = "Oliveir@s1968";

// ============================================================================
// LIMITES E TIMEOUTS
// ============================================================================
inline constexpr uint32_t TASK_MIN_DELAY_MS = 1U;
inline constexpr float MIN_DISTANCE_METERS = 0.0f;
inline constexpr float MAX_DISTANCE_METERS = 50.0f;
inline constexpr size_t MAX_BUFFER_SIZE = 256U;

// ============================================================================
// ENDEREÇOS E TÓPICOS POR ÂNCORA (Alocação Estática Baseada na Seleção)
// ============================================================================
#if ANCHOR_NUMBER == 1
    inline constexpr char MQTT_TOPIC[] = "uwb/ancora1/comandos";
    inline constexpr uint16_t DW1000_ANCHOR_SHORT_ADDRESS = 0x2540U;
    inline constexpr char DW1000_ANCHOR_MAC_ADDRESS[] = "C8:2E:18:FB:25:40";
#elif ANCHOR_NUMBER == 2
    inline constexpr char MQTT_TOPIC[] = "uwb/ancora2/comandos";
    inline constexpr uint16_t DW1000_ANCHOR_SHORT_ADDRESS = 0x3426U;
    inline constexpr char DW1000_ANCHOR_MAC_ADDRESS[] = "D4:8C:49:A1:34:26";
#elif ANCHOR_NUMBER == 3
    inline constexpr char MQTT_TOPIC[] = "uwb/ancora3/comandos";
    inline constexpr uint16_t DW1000_ANCHOR_SHORT_ADDRESS = 0x3014U;
    inline constexpr char DW1000_ANCHOR_MAC_ADDRESS[] = "d4:8c:49:a1:30:14";
#elif ANCHOR_NUMBER == 4
    inline constexpr char MQTT_TOPIC[] = "uwb/ancora4/comandos";
    inline constexpr uint16_t DW1000_ANCHOR_SHORT_ADDRESS = 0x7db4U;
    inline constexpr char DW1000_ANCHOR_MAC_ADDRESS[] = "e0:5a:1b:1f:7d:b4";
#elif ANCHOR_NUMBER == 5
    inline constexpr char MQTT_TOPIC[] = "uwb/ancora5/comandos";
    inline constexpr uint16_t DW1000_ANCHOR_SHORT_ADDRESS = 0x31b8U;
    inline constexpr char DW1000_ANCHOR_MAC_ADDRESS[] = "d4:8c:49:a1:31:b8";
#elif ANCHOR_NUMBER == 6
    inline constexpr char MQTT_TOPIC[] = "uwb/ancora6/comandos";
    inline constexpr uint16_t DW1000_ANCHOR_SHORT_ADDRESS = 0x31c8U;
    inline constexpr char DW1000_ANCHOR_MAC_ADDRESS[] = "d4:8c:49:a1:31:c8";
#elif ANCHOR_NUMBER == 7
    inline constexpr char MQTT_TOPIC[] = "uwb/ancora7/comandos";
    inline constexpr uint16_t DW1000_ANCHOR_SHORT_ADDRESS = 0x2950U;
    inline constexpr char DW1000_ANCHOR_MAC_ADDRESS[] = "c8:2e:18:fb:29:50";
#elif ANCHOR_NUMBER == 8
    inline constexpr char MQTT_TOPIC[] = "uwb/ancora8/comandos";
    inline constexpr uint16_t DW1000_ANCHOR_SHORT_ADDRESS = 0x2904U;
    inline constexpr char DW1000_ANCHOR_MAC_ADDRESS[] = "c8:2e:18:fb:29:04";
#elif ANCHOR_NUMBER == 9
    inline constexpr char MQTT_TOPIC[] = "uwb/ancora9/comandos";
    inline constexpr uint16_t DW1000_ANCHOR_SHORT_ADDRESS = 0x3674U;
    inline constexpr char DW1000_ANCHOR_MAC_ADDRESS[] = "d4:8c:49:a1:36:74";
#elif ANCHOR_NUMBER == 10
    inline constexpr char MQTT_TOPIC[] = "uwb/ancora10/comandos";
    inline constexpr uint16_t DW1000_ANCHOR_SHORT_ADDRESS = 0x3674U;
    inline constexpr char DW1000_ANCHOR_MAC_ADDRESS[] = "D4:8C:49:A1:36:74";
#elif ANCHOR_NUMBER == 11
    inline constexpr char MQTT_TOPIC[] = "uwb/ancora11/comandos";
    inline constexpr uint16_t DW1000_ANCHOR_SHORT_ADDRESS = 0x2904U;
    inline constexpr char DW1000_ANCHOR_MAC_ADDRESS[] = "C8:2E:18:FB:29:04";
#elif ANCHOR_NUMBER == 12
    inline constexpr char MQTT_TOPIC[] = "uwb/ancora12/comandos";
    inline constexpr uint16_t DW1000_ANCHOR_SHORT_ADDRESS = 0x297CU;
    inline constexpr char DW1000_ANCHOR_MAC_ADDRESS[] = "C8:2E:18:FB:29:7C";
#else
    #error "Invalid ANCHOR_NUMBER."
#endif

// ============================================================================
// PARÂMETROS DE TRANSMISSÃO DW1000
// ============================================================================
// Qualquer alteração nestes parâmetros afetará o tempo de transmissão.
// Certifique-se de ajustar os tempos de resposta na biblioteca se necessário.

// 1. Taxa de Transmissão (Data Rate)
inline constexpr uint8_t DW1000_TX_RATE = DW1000.TRX_RATE_6800KBPS;
// inline constexpr uint8_t DW1000_TX_RATE = DW1000.TRX_RATE_850KBPS;
// inline constexpr uint8_t DW1000_TX_RATE = DW1000.TRX_RATE_110KBPS;

// 2. Frequência de Pulso (PRF)
inline constexpr uint8_t DW1000_TX_FREQ = DW1000.TX_PULSE_FREQ_16MHZ;
// inline constexpr uint8_t DW1000_TX_FREQ = DW1000.TX_PULSE_FREQ_64MHZ;

// 3. Tamanho do Preamble (Preamble Length)
inline constexpr uint8_t DW1000_TX_PREAMBLE = DW1000.TX_PREAMBLE_LEN_64;
// inline constexpr uint8_t DW1000_TX_PREAMBLE = DW1000.TX_PREAMBLE_LEN_128;
// inline constexpr uint8_t DW1000_TX_PREAMBLE = DW1000.TX_PREAMBLE_LEN_256;
// inline constexpr uint8_t DW1000_TX_PREAMBLE = DW1000.TX_PREAMBLE_LEN_512;
// inline constexpr uint8_t DW1000_TX_PREAMBLE = DW1000.TX_PREAMBLE_LEN_1024;
// inline constexpr uint8_t DW1000_TX_PREAMBLE = DW1000.TX_PREAMBLE_LEN_1536;
// inline constexpr uint8_t DW1000_TX_PREAMBLE = DW1000.TX_PREAMBLE_LEN_2048;
// inline constexpr uint8_t DW1000_TX_PREAMBLE = DW1000.TX_PREAMBLE_LEN_4096;

// Array de configuração final passado para o driver UWB
inline constexpr uint8_t MODE[] = {DW1000_TX_RATE, DW1000_TX_FREQ, DW1000_TX_PREAMBLE};

// ============================================================================
// CALIBRAÇÃO DE ANTENA
// ============================================================================
// A palavra 'inline' é crucial aqui para evitar erros de Linker (ODR Violation)
inline uint16_t getAntennaDelayForAnchor(int anchorNumber) {
    switch (anchorNumber) {
        case 1: return 16660U;
        case 2: return 16600U;
        case 3: return 16566U;
        case 4: return 16656U;
        case 5: return 16530U;
        default: return 16530U;
    }
}

// ============================================================================
// ESP-IDF NVS KEYS
// ============================================================================
inline constexpr char NVS_WIFI_NAMESPACE[] = "wifi_cred";
inline constexpr char NVS_WIFI_SSID[] = "ssid";
inline constexpr char NVS_WIFI_PASS[] = "pass";
inline constexpr bool NVS_READ_WRITE = false;