#include "network_manager.h"
#include "Defines.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#define MAX_RETRY_COUNT 5 
#define RESTART_TIMEOUT_MS 10000


static const char* TAG = "NET_MGR";

static char current_ssid[64] = {};
static char current_pass[64] = {};
static uint8_t retry_count = 0;


// ============================================================================
// 1. VARIÁVEIS DE ESTADO E INJEÇÃO DE DEPENDÊNCIA (Estáticas / Privadas)
// ============================================================================
static const serializer_t*        p_serializer = NULL;
static const data_transport_t*    p_data_transport = NULL;
static const control_transport_t* p_control_transport = NULL;
static network_event_cb           p_event_callback = NULL;

static net_state_t current_state = NET_STATE_DISCONNECTED;
static QueueHandle_t p_uwb_queue = NULL;       


static void update_state(net_state_t new_state) {
    if (current_state != new_state) {
        current_state = new_state;
        ESP_LOGI(TAG, "Mudanca de Estado de Rede: %d", current_state);
        if (p_event_callback) {
            p_event_callback(current_state);
        }
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        update_state(NET_STATE_WIFI_CONNECTING);
        esp_wifi_connect();
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        update_state(NET_STATE_DISCONNECTED);
        
        if (p_control_transport && p_control_transport->disconnect) p_control_transport->disconnect();
        if (p_data_transport && p_data_transport->disconnect) p_data_transport->disconnect();

        if (retry_count < MAX_RETRY_COUNT) {
            retry_count++;
            ESP_LOGW(TAG, "Wi-Fi desconectado. Tentativa %d/%d...", retry_count, MAX_RETRY_COUNT);
            esp_wifi_connect(); 
        } else {
            ESP_LOGE(TAG, "Falha critica no Wi-Fi. Reiniciando o hardware em 10s...");
            vTaskDelay(pdMS_TO_TICKS(RESTART_TIMEOUT_MS));
            esp_restart(); // Recuperação de falha fatal
        }

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        retry_count = 0; // MANTIDO: Essencial para evitar reboots falsos
        update_state(NET_STATE_WIFI_READY); // MANTIDO: Marca a interface como UP
        ESP_LOGI(TAG, "IP Recebido. Iniciando Transportes...");

        bool b_control_ok = true;
        bool b_data_ok = true;

        // Avalia independentemente
        if (p_control_transport && p_control_transport->connect) {
            b_control_ok = p_control_transport->connect();
        }
        if (p_data_transport && p_data_transport->connect) {
            b_data_ok = p_data_transport->connect();
        }

        // TRATAMENTO DESACOPLADO DE FALHAS
        if (b_control_ok && b_data_ok) {
            update_state(NET_STATE_FULLY_READY);
            ESP_LOGI(TAG, "Planos de Controle e Dados OPERACIONAIS.");
        } else if (b_data_ok) {
            // O UDP (Dados) conectou, mas o MQTT (Controle) falhou.
            // Promovemos o estado para garantir que a telemetria não para.
            update_state(NET_STATE_FULLY_READY); 
            ESP_LOGW(TAG, "Plano de Dados OPERACIONAL. Plano de Controle (MQTT) FALHOU.");
        } else {
            ESP_LOGE(TAG, "Falha critica na inicializacao dos sockets de Transporte (UDP morto).");
        }
    }
}

// ============================================================================
// 3. INICIALIZAÇÃO E EXPOSIÇÃO DA API
// ============================================================================
void network_manager_init(const serializer_t* data_serializer, 
                          const data_transport_t* data_transport,
                          const control_transport_t* control_transport,
                          network_event_cb event_callback) {
    
    p_serializer = data_serializer;
    p_data_transport = data_transport;
    p_control_transport = control_transport;
    p_event_callback = event_callback;

    // Inicializa o subsistema NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Tenta carregar as credenciais salvas na NVS
    nvs_handle_t nvs_handle;
    bool credenciais_nvs_carregadas = false;

    if (nvs_open(NVS_WIFI_NAMESPACE, NVS_READONLY, &nvs_handle) == ESP_OK) {
        size_t ssid_len = sizeof(current_ssid);
        size_t pass_len = sizeof(current_pass);
        
        esp_err_t err_ssid = nvs_get_str(nvs_handle, NVS_WIFI_SSID, current_ssid, &ssid_len);
        esp_err_t err_pass = nvs_get_str(nvs_handle, NVS_WIFI_PASS, current_pass, &pass_len);
        
        if (err_ssid == ESP_OK && err_pass == ESP_OK && ssid_len > 1) {
            credenciais_nvs_carregadas = true;
            ESP_LOGI(TAG, "Credenciais carregadas da NVS com sucesso.");
        }
        nvs_close(nvs_handle);
    }

    // Fallback para Defines.h caso a NVS falhe ou esteja vazia
    if (!credenciais_nvs_carregadas) {
        ESP_LOGW(TAG, "Falha na NVS ou vazia. Usando credenciais padrao do Defines.h");
        strncpy(current_ssid, WIFI_SSID, sizeof(current_ssid) - 1);
        strncpy(current_pass, WIFI_PASSWORD, sizeof(current_pass) - 1);
    }

    // Configuração do LwIP e Event Loop
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, current_ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, current_pass, sizeof(wifi_config.sta.password) - 1);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

net_state_t network_manager_get_state(void) {
    return current_state;
}

bool network_manager_is_ready_to_send(void) {
    return (current_state == NET_STATE_FULLY_READY);
}

// ============================================================================
// 4. A TASK DO PLANO DE DADOS (Core 0)
// ============================================================================
static void task_network_routine(void *parameter) {
    ESP_LOGI(TAG, "Task de Rede iniciada no Core 0");
    
    range_pkg_t pkg;
    uint8_t tx_buffer[256]; // Memória estática, sem malloc

    for (;;) {
        // Aguarda indefinidamente por um pacote UWB na fila (Zero consumo de CPU enquanto vazio)
        if (xQueueReceive(p_uwb_queue, &pkg, portMAX_DELAY) == pdPASS) {
            

            if (network_manager_is_ready_to_send()) {
                
                // Blinda contra falha de injeção de dependência
                if (p_serializer && p_serializer->serialize) {
                    size_t payload_len = p_serializer->serialize(&pkg, tx_buffer, sizeof(tx_buffer));

                    if (payload_len > 0 && p_data_transport && p_data_transport->send) {
                        p_data_transport->send(tx_buffer, payload_len);
                    }
                }
            }
        }
    }
}

// Atualizamos a assinatura para receber a fila criada no main.cpp
void network_manager_start_task(QueueHandle_t uwb_queue) {
    p_uwb_queue = uwb_queue;
    xTaskCreatePinnedToCore(task_network_routine, "NetTask", 4096, NULL, 1, NULL, 0);
}

// ============================================================================
// 5. ATUALIZAÇÃO DE CREDENCIAIS
// ============================================================================
bool network_manager_change_wifi(const char* new_ssid, const char* new_pass) {
    if (!new_ssid || !new_pass) return false;

    ESP_LOGI(TAG, "Comando de troca de Wi-Fi. Aplicando nova config...");
    
    strncpy(current_ssid, new_ssid, sizeof(current_ssid) - 1);
    strncpy(current_pass, new_pass, sizeof(current_pass) - 1);

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, current_ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, current_pass, sizeof(wifi_config.sta.password) - 1);

    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    
    // O disconnect disparará o evento no handler, que usará a nova config para reconectar
    esp_wifi_disconnect(); 

    return true; 
}