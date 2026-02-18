/******************************************************************************
 * Copyright © 2025 HUB I4.0 - Universidade do Estado do Amazonas
 * Modificado por Tiago Oliveira - Universidade Federal do Pará
 * Arquitetura Dual Core + ESP-IDF MQTT + Round-Robin TWR
 *****************************************************************************/

#include "Defines.h" // LEIA ESSE ARQUIVO PRIMEIRO
#include <Preferences.h>
#include <ArduinoJson.h>

Preferences preferences;

// ============================================================================
// CONTEXT NAMESPACES
// ============================================================================

namespace network_ctx
{   
    //buffers para conexão wifi
    char current_ssid[32];
    char current_pass[64];
    
    // Buffers de fallback
    char backup_ssid[32];
    char backup_pass[64];
    
    //booleano para fallback
    bool b_giveup_new_wifi = false;

    // Controle de estado
    uint8_t wifi_retries = 0;
    bool b_trying_new_wifi = false;
    volatile bool b_connect_to_new_wifi = false;
}

namespace mqtt_ctx{
    // Buffer para tópico de configuração
    char mqtt_config_topic[64];

    // Handle cliente MQTT nativo
    esp_mqtt_client_handle_t handle_mqtt_client = NULL;
    
    // Buffers para identificação
    char mqtt_client_id[32];

    volatile bool b_mqtt_connected = false;
    bool b_mqtt_initialized = false;
}

namespace rtos_ctx
{
    // Handles FreeRTOS
    QueueHandle_t uwbQueue;
    TaskHandle_t handle_task_dw1000;
    TaskHandle_t handle_task_network;
}

// ============================================================================
// ESTRUTURAS DE DADOS PARA ENVIO DE RANGE (Core 1 -> Core 0)
// ============================================================================
typedef struct
{
    uint16_t anchor_id;
    uint16_t tag_id;
    float distance;
    int16_t ax;
    int16_t ay;
    int16_t az;
    float rp_power;
    float fp_power;
    float eta;
    float quality;
} range_pkg;

// ============================================================================
// PROTÓTIPOS DE FUNÇÃO
// ============================================================================
void task_dw1000_routine(void *parameter);
void task_network_routine(void *parameter);

void new_range_callback(DW1000Device *device);
void new_device_callback(DW1000Device *device);
void inactive_device_callback(DW1000Device *device);

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

void start_mqtt();
void manage_wifi_connection();
void manage_mqtt_connection();
void retrive_and_publish_range();


// ============================================================================
// SETUP
// ============================================================================
void setup()
{
    Serial.begin(SERIAL_BAUD);
    delay(1000);

    Serial.println("--- Inicializando Ancora UWB (No Strings) ---");

    // --- CRIAÇÃO DA FILA ---
    rtos_ctx::uwbQueue = xQueueCreate(1, sizeof(range_pkg));
    if (rtos_ctx::uwbQueue == NULL)
    {
        Serial.println("Erro: Falha ao criar fila");
    }

    // --- PREENCHIMENTO DOS BUFFERS ---
    snprintf(mqtt_ctx::mqtt_client_id, sizeof(mqtt_ctx::mqtt_client_id), "ESP32_Anchor_%X", DW1000_ANCHOR_SHORT_ADDRESS);
    snprintf(mqtt_ctx::mqtt_config_topic, sizeof(mqtt_ctx::mqtt_config_topic), "uwb/ancora%d/config", ANCHOR_NUMBER);

    // --- INICIALIZAÇÃO DO DW1000 ---
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

    // Inicialização conforme Defines.h
    DW1000Ranging.init(BoardType::DW1000_BOARD_TYPE,
                       DW1000_ANCHOR_SHORT_ADDRESS,
                       DW1000_ANCHOR_MAC_ADDRESS,
                       true,
                       (byte *)MODE, // Cast necessário pois MODE é constexpr byte[]
                       DW1000_PIN_RST,
                       DW1000_PIN_SS,
                       DW1000_PIN_IRQ);

    DW1000Ranging.attachNewRange(new_range_callback);
    DW1000Ranging.attachNewDevice(new_device_callback);
    DW1000Ranging.attachInactiveDevice(inactive_device_callback);

    uint16_t delayAntenna = getAntennaDelayForAnchor(ANCHOR_NUMBER);
    DW1000.setAntennaDelay(delayAntenna);

    Serial.printf("Ancora ID: %X | Delay Antena: %d\n", DW1000_ANCHOR_SHORT_ADDRESS, delayAntenna);

    // --- PINAGEM DAS TASK's ---
    xTaskCreatePinnedToCore(task_network_routine, "NetTask", 4096, NULL, 1, &rtos_ctx::handle_task_network, 0);
    xTaskCreatePinnedToCore(task_dw1000_routine, "UwbTask", 4096, NULL, 5, &rtos_ctx::handle_task_dw1000, 1);
}

void loop()
{
    vTaskDelete(NULL);
}

// ============================================================================
// TASKS
// ============================================================================

// --- TASK UWB (CORE 1) ---
void task_dw1000_routine(void *parameter)
{
    Serial.println("[UWB] Task iniciada no Core 1");
    for (;;)
    {
        DW1000Ranging.loop();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void manage_wifi_connection()
{
    // Verifica se deve tentar conectar à outra rede
    if (network_ctx::b_connect_to_new_wifi)
    {
        Serial.printf("[Net] Desconectando para testar nova rede: %s\n", network_ctx::current_ssid);
        WiFi.disconnect(true);

        network_ctx::b_connect_to_new_wifi = false;
        mqtt_ctx::b_mqtt_connected = false;

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        network_ctx::wifi_retries++;
        Serial.printf("[Net] Conectando ao WiFi: %s (Tentativa %d)\n", network_ctx::current_ssid, network_ctx::wifi_retries);
        WiFi.begin(network_ctx::current_ssid, network_ctx::current_pass);

        // Margem para tentar conectar ao wifi
        unsigned long margin_to_connect_wifi = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - margin_to_connect_wifi < 10000)
        {
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("[Net] WiFi Conectado com sucesso.");
            network_ctx::wifi_retries = 0; // reseta tentativas

            if (network_ctx::b_trying_new_wifi)
            {
                preferences.begin(NVS_WIFI_NAMESPACE, NVS_READ_WRITE);
                preferences.putString(NVS_WIFI_SSID, network_ctx::current_ssid);
                preferences.putString(NVS_WIFI_PASS, network_ctx::current_pass);
                preferences.end();
                network_ctx::b_trying_new_wifi = false;
            }
        }
        else
        {
            Serial.println("[Net] Falha na conexão WiFi.");

            if (network_ctx::wifi_retries >= 3)
            {
                network_ctx::b_giveup_new_wifi = true;
            }

            if (network_ctx::b_trying_new_wifi && network_ctx::b_giveup_new_wifi)
            {
                Serial.println("[Net] CRÍTICO: Falha em 3 tentativas. Retornando para a rede anterior!");

                // Restaura os dados do backup
                strlcpy(network_ctx::current_ssid, network_ctx::backup_ssid, sizeof(network_ctx::current_ssid));
                strlcpy(network_ctx::current_pass, network_ctx::backup_pass, sizeof(network_ctx::current_pass));

                // Aborta o teste e zera o contador para reconectar na rede antiga
                network_ctx::b_trying_new_wifi = false;
                network_ctx::b_giveup_new_wifi = false;
                network_ctx::wifi_retries = 0;
            }
        }
    }
}

void start_mqtt()
{
    esp_mqtt_client_config_t mqtt_cfg = {};

    mqtt_cfg.broker.address.uri = MQTT_BROKER;
    mqtt_cfg.credentials.client_id = mqtt_ctx::mqtt_client_id;

    // Atributo confuso, setar em falso nega a desabilitação da reconexão automática
    // Então está setado para reconectar
    mqtt_cfg.network.disable_auto_reconnect = false;

    mqtt_ctx::handle_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_ctx::handle_mqtt_client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    esp_mqtt_client_start(mqtt_ctx::handle_mqtt_client);
}

void manage_mqtt_connection()
{
    if (WiFi.status() == WL_CONNECTED)
    {

        if (!mqtt_ctx::b_mqtt_initialized)
        {
            mqtt_ctx::b_mqtt_initialized = true;
            start_mqtt();
        }
    }
}

void retrive_and_publish_range()
{   
    range_pkg received_range_pkg;
    char jsonBuffer[MAX_BUFFER_SIZE];

    if (xQueueReceive(rtos_ctx::uwbQueue, &received_range_pkg, 10 / portTICK_PERIOD_MS) == pdPASS)
    {
        if (mqtt_ctx::b_mqtt_connected)
        {
            int len = snprintf(jsonBuffer, sizeof(jsonBuffer),
                               "{\"id_ancora\":%d,\"id_tag\":%d,\"distancia\":%.2f,"
                               "\"ax\":%d,\"ay\":%d,\"az\":%d,"
                               "\"fp\":%.2f,\"rx\":%.2f,\"eta\":%.2f,\"quality\":%.2f}",
                               received_range_pkg.anchor_id, received_range_pkg.tag_id, received_range_pkg.distance,
                               received_range_pkg.ax, received_range_pkg.ay, received_range_pkg.az,
                               received_range_pkg.fp_power, received_range_pkg.rp_power,
                               received_range_pkg.eta, received_range_pkg.quality);

            esp_mqtt_client_publish(mqtt_ctx::handle_mqtt_client, MQTT_TOPIC, jsonBuffer, len, 0, 0);
        }
    }
}

// --- TASK NETWORK (CORE 0) ---
void task_network_routine(void *parameter)
{
    Serial.println("[Net] Task iniciada no Core 0");

    preferences.begin(NVS_WIFI_NAMESPACE, true);
    size_t ssid_len = preferences.getString(NVS_WIFI_SSID, network_ctx::current_ssid, sizeof(network_ctx::current_ssid));
    size_t pass_len = preferences.getString(NVS_WIFI_PASS, network_ctx::current_pass, sizeof(network_ctx::current_pass));
    preferences.end();
    

    if (ssid_len == 0 || pass_len == 0)
    {
        strlcpy(network_ctx::current_ssid, WIFI_SSID, sizeof(network_ctx::current_ssid));
        strlcpy(network_ctx::current_pass, WIFI_PASSWORD, sizeof(network_ctx::current_pass));
        Serial.println("[Net] Usando credenciais padrão do Defines.h");
    }
    else
    {
        Serial.println("[Net] Usando credenciais salvas na NVS");
    }
    
    // --- LOOP DA TASK ---
    for (;;)
    {
        manage_wifi_connection();

        manage_mqtt_connection();

        retrive_and_publish_range();

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// ============================================================================
// CALLBACKS DW1000
// ============================================================================

void new_range_callback(DW1000Device *device)
{
    float dist = device->getRange();
    
    if (dist < MIN_DISTANCE_METERS || dist > MAX_DISTANCE_METERS)
    {
        return;
    }
    
    range_pkg data;

    data.anchor_id = DW1000_ANCHOR_SHORT_ADDRESS;
    data.tag_id = device->getShortAddress();
    data.distance = dist;
    data.rp_power = device->getRXPower();
    data.fp_power = device->getFPPower();
    data.quality = device->getQuality();

    // Nota: Assume que a biblioteca DW1000 modificada expõe mpu_ax/ay/az
    data.ax = (int16_t)device->mpu_ax;
    data.ay = (int16_t)device->mpu_ay;
    data.az = (int16_t)device->mpu_az;


    // Cálculo seguro de ETA
    if (data.fp_power != 0.0f)
    {
        data.eta = data.rp_power / data.fp_power;
    }
    else
    {
        data.eta = 0.0f;
    }


    xQueueOverwrite(rtos_ctx::uwbQueue, &data);
}

void new_device_callback(DW1000Device *device)
{
    Serial.printf("Device Detectado: %X\n", device->getShortAddress());
}

void inactive_device_callback(DW1000Device *device)
{
    Serial.printf("Device Inativo: %X\n", device->getShortAddress());
}


// ============================================================================
// CALLBACKS MQTT
// ============================================================================
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch ((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:
        mqtt_ctx::b_mqtt_connected = true;
        Serial.println("MQTT: Conectado");
        esp_mqtt_client_subscribe(mqtt_ctx::handle_mqtt_client, mqtt_ctx::mqtt_config_topic, 0);
        break;
        
        case MQTT_EVENT_DISCONNECTED:
        Serial.println("MQTT: Desconectado");
        mqtt_ctx::b_mqtt_connected = false;
        break;
        
        case MQTT_EVENT_DATA:
        if (strncmp(event->topic, mqtt_ctx::mqtt_config_topic, event->topic_len) == 0)
        {
            
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, event->data, event->data_len);
            
            if (!error)
            {
                const char *new_ssid = doc[NVS_WIFI_SSID];
                const char *new_pass = doc[NVS_WIFI_PASS];
                
                if (new_ssid != nullptr && new_pass != nullptr)
                {
                    // 1. Salva as credenciais em uso no buffer de backup
                    strlcpy(network_ctx::backup_ssid, network_ctx::current_ssid, sizeof(network_ctx::backup_ssid));
                    strlcpy(network_ctx::backup_pass, network_ctx::current_pass, sizeof(network_ctx::backup_pass));
                    
                    // 2. Atualiza as credenciais alvo
                    strlcpy(network_ctx::current_ssid, new_ssid, sizeof(network_ctx::current_ssid));
                    strlcpy(network_ctx::current_pass, new_pass, sizeof(network_ctx::current_pass));
                    
                    // 3. Prepara a máquina de estados para validação
                    network_ctx::wifi_retries = 0;
                    network_ctx::b_trying_new_wifi = true;
                    network_ctx::b_connect_to_new_wifi = true;
                    network_ctx::b_giveup_new_wifi = false;
                    Serial.println("[MQTT] Comando de rede recebido. Testando conexão...");
                }
            }
            else
            {
                Serial.println("[MQTT] Erro no parsing do JSON.");
            }
        }
        break;
        
        case MQTT_EVENT_ERROR:
        Serial.println("MQTT: Erro interno");
        break;
        
        default:
        break;
    }
}


// ============================================================================
// MAIN
// ============================================================================
extern "C" void app_main()
{

    initArduino();

    setup();

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}