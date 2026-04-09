/******************************************************************************
 * Copyright © 2025 HUB I4.0 - Universidade do Estado do Amazonas
 * Modificado por Tiago Oliveira - Universidade Federal do Pará
 * Arquitetura Modular Dual-Plane (Controle e Dados) + Dual Core + Round-Robin TWR
 *****************************************************************************/

#include "Defines.h" 
#include "types.h"
#include <ArduinoJson.h>

// Inclusão da Arquitetura de Rede
#include "network_manager.h"
#include "json_serializer.h"
#include "udp_data_transport.h"
#include "mqtt_control_transport.h"
#include "esp_timer.h" 

// ============================================================================
// CONTEXT NAMESPACES (Reduzidos e Focados)
// ============================================================================
namespace rtos_ctx {
    QueueHandle_t uwbQueue = nullptr;
    TaskHandle_t handle_task_dw1000 = nullptr;
}

namespace ota_ctx {
    bool b_start_update = false;
    char ota_url[128] = {0};
    char ota_new_version[32] = {0};
}

// ============================================================================
// PROTÓTIPOS
// ============================================================================
void task_dw1000_routine(void *parameter);
void new_range_callback(DW1000Device *device);
void new_device_callback(DW1000Device *device);
void inactive_device_callback(DW1000Device *device);

void process_control_command(const uint8_t* payload, size_t len);
void on_network_state_changed(net_state_t new_state);

constexpr uint32_t calculate_hash(const char * command_payload);
void enable_ota_routine(const JsonDocument& doc);
void change_network(const JsonDocument& doc);
void restart_device();
void update_firmware();

// ============================================================================
// HASHING COMPILE-TIME
// ============================================================================
constexpr uint32_t calculate_hash(const char * command_payload) {
    uint32_t hash = 0x811c9dc5;
    uint32_t hash_multiplier = 0x01000193;
    const char *p_str = command_payload;

    while (*p_str != '\0') {
        hash = hash ^ *p_str;
        hash = hash * hash_multiplier;
        p_str++;
    }
    return hash;
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(SERIAL_BAUD);
    vTaskDelay(pdMS_TO_TICKS(1000));

    Serial.println("--- Inicializando Ancora UWB Modular ---");

    // 1. Criação da Fila de Comunicação Inter-Core (Core 1 -> Core 0)
    rtos_ctx::uwbQueue = xQueueCreate(1, sizeof(range_pkg_t));
    if (rtos_ctx::uwbQueue == nullptr) {
        Serial.println("[CRÍTICO] Falha ao criar fila UWB.");
        esp_restart();
    }

    // 2. Injeção de Dependências e Inicialização da Rede
    // Registamos o callback de comandos diretamente no transporte de controlo
    if (transport_mqtt.set_receive_callback) {
        transport_mqtt.set_receive_callback(process_control_command);
    }

    // Inicializamos o Orquestrador passando as três instâncias concretas
    network_manager_init(&serializer_json, &transport_udp, &transport_mqtt, on_network_state_changed);
    network_manager_start_task(rtos_ctx::uwbQueue);

    // 3. Inicialização do Hardware DW1000
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

    DW1000Ranging.init(BoardType::DW1000_BOARD_TYPE,
                       DW1000_ANCHOR_SHORT_ADDRESS,
                       DW1000_ANCHOR_MAC_ADDRESS,
                       true,
                       const_cast<byte*>(MODE),
                       DW1000_PIN_RST,
                       DW1000_PIN_SS,
                       DW1000_PIN_IRQ);

    DW1000Ranging.attachNewRange(new_range_callback);
    DW1000Ranging.attachNewDevice(new_device_callback);
    DW1000Ranging.attachInactiveDevice(inactive_device_callback);

    uint16_t delayAntenna = getAntennaDelayForAnchor(ANCHOR_NUMBER);
    DW1000.setAntennaDelay(delayAntenna);

    Serial.printf("Ancora ID: %X (%d) | Delay Antena: %d\n", DW1000_ANCHOR_SHORT_ADDRESS, ANCHOR_NUMBER ,delayAntenna);

    // 4. Início da Task de Ranging (Core 1)
    xTaskCreatePinnedToCore(task_dw1000_routine, "UwbTask", 4096, nullptr, 5, &rtos_ctx::handle_task_dw1000, 1);
}

void loop() {
    // Loop do Arduino desativado. Tudo roda em Tasks do FreeRTOS.
    vTaskDelete(nullptr);
}

// ============================================================================
// TASK DW1000 (CORE 1)
// ============================================================================
void task_dw1000_routine(void *parameter) {
    Serial.println("[UWB] Task iniciada no Core 1");
    for (;;) {
        DW1000Ranging.loop();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// ============================================================================
// CALLBACKS DW1000
// ============================================================================
void new_range_callback(DW1000Device *device) {
    float dist = device->getRange();
    
    if (dist < MIN_DISTANCE_METERS || dist > MAX_DISTANCE_METERS) {
        return;
    }
    
    range_pkg_t data = {}; // Inicialização a zero (Prevenção de lixo na memória)

    data.anchor_id = DW1000_ANCHOR_SHORT_ADDRESS;
    data.tag_id    = device->getShortAddress();
    data.distance  = dist;
    data.rp_power  = device->getRXPower();
    data.fp_power  = device->getFPPower();
    data.quality   = device->getQuality();

    data.ax = static_cast<int16_t>(device->mpu_ax);
    data.ay = static_cast<int16_t>(device->mpu_ay);
    data.az = static_cast<int16_t>(device->mpu_az);

    if (data.fp_power != 0.0f) {
        data.eta = data.rp_power / data.fp_power;
    } else {
        data.eta = 0.0f;
    }

    data.timestamp_us = esp_timer_get_time();

    // Sobrescreve o pacote antigo se a rede estiver ocupada (Non-blocking)
    xQueueOverwrite(rtos_ctx::uwbQueue, &data);
}

void new_device_callback(DW1000Device *device) {
    Serial.printf("[UWB] Device Detectado: %X\n", device->getShortAddress());
}

void inactive_device_callback(DW1000Device *device) {
    Serial.printf("[UWB] Device Inativo: %X\n", device->getShortAddress());
}

// ============================================================================
// GESTÃO DE COMANDOS (PLANO DE CONTROLO)
// ============================================================================
void on_network_state_changed(net_state_t new_state) {
    // Apenas monitoramento. O orquestrador gere as reconexões internamente.
    Serial.printf("[SYSTEM] Novo estado de rede: %d\n", new_state);
}

void process_control_command(const uint8_t* payload, size_t len) {
    // Alocação estática (Stack) do ArduinoJson com tamanho seguro
    #if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument doc; 
    #else
        StaticJsonDocument<256> doc;
    #endif

    DeserializationError error = deserializeJson(doc, payload, len);

    if (error) {
        Serial.printf("[MQTT] Erro no parsing JSON: %s\n", error.c_str());
        return;
    }

    const char *command_payload = doc["command"];
    if (command_payload == nullptr) {
        Serial.println("[MQTT] Comando invalido (nulo).");
        return;
    }
    
    uint32_t hashed_command = calculate_hash(command_payload);

    switch (hashed_command) {
        case calculate_hash("FIRMWARE_UPDATE"):
            enable_ota_routine(doc);
            break;
        
        case calculate_hash("CHANGE_NETWORK"):
            change_network(doc);
            break;

        case calculate_hash("RESTART_DEVICE"):
            restart_device();
            break;

        default:
            Serial.println("[MQTT] Comando desconhecido.");
            break;
    }
}

// ============================================================================
// LÓGICA DE APLICAÇÃO (AÇÕES DE CONTROLO)
// ============================================================================
void enable_ota_routine(const JsonDocument& doc) {
    const char* url = doc["update"]["url"];
    const char* version = doc["update"]["version"];

    if (!url || !version) return;

    if (strncmp(CURRENT_VERSION, version, sizeof(CURRENT_VERSION)) == 0) {
        Serial.printf("[OTA] Versao %s enviada ja e atual.\n", version);
        return;
    }

    strlcpy(ota_ctx::ota_url, url, sizeof(ota_ctx::ota_url));
    strlcpy(ota_ctx::ota_new_version, version, sizeof(ota_ctx::ota_new_version));
    
    ota_ctx::b_start_update = true;
    update_firmware(); // Dispara imediatamente
}

void change_network(const JsonDocument& doc) {
    const char * new_ssid = doc[NVS_WIFI_SSID];
    const char * new_pass = doc[NVS_WIFI_PASS];

    if (new_ssid != nullptr && new_pass != nullptr) {
        // Chamada à API segura do nosso orquestrador
        // O orquestrador cuidará do fallback e da gravação na NVS se falhar
        network_manager_change_wifi(new_ssid, new_pass);
    }
}

void restart_device() {
    Serial.println("[SYSTEM] Reiniciando por comando remoto...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
}

void update_firmware() {
    Serial.printf("[OTA] Iniciando rotina de atualizacao para a versao: %s\n", ota_ctx::ota_new_version);
    // TODO: Implementar lógica de esp_https_ota
    ota_ctx::b_start_update = false;
}


extern "C" void app_main() {
    initArduino();

    setup();

    // 3. Destrói a Task inicial (Core 0). 
    // Como a nossa arquitetura agora é totalmente orientada a eventos assíncronos e 
    // Tasks do FreeRTOS independentes, manter o loop() nativo apenas desperdiçaria RAM.
    vTaskDelete(nullptr);
}
