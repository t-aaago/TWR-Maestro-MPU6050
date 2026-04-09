#include "mqtt_control_transport.h"
#include "Defines.h"
#include <mqtt_client.h>
#include <esp_log.h>
#include <cstring>

// Isolamento de estado (Internal Linkage)
namespace {
    const char* const TAG = "MQTT_CTRL";
    
    esp_mqtt_client_handle_t mqtt_client = nullptr;
    control_command_cb_t registered_callback = nullptr;
    bool is_initialized = false;
    bool is_connected = false;

    /*
     * Callback assíncrono executado pela Task interna da biblioteca MQTT.
     */
    void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
        auto* event = static_cast<esp_mqtt_event_handle_t>(event_data);
        
        switch (static_cast<esp_mqtt_event_id_t>(event_id)) {
            case MQTT_EVENT_CONNECTED:
                is_connected = true;
                ESP_LOGI(TAG, "MQTT Conectado ao Broker.");
                // Se um tópico estiver definido no Defines.h, fazemos o Subscribe automático
                if (mqtt_client != nullptr && std::strlen(MQTT_TOPIC) > 0) {
                    esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC, 1);
                    ESP_LOGI(TAG, "Inscrito no topico: %s", MQTT_TOPIC);
                }
                break;
                
            case MQTT_EVENT_DISCONNECTED:
                is_connected = false;
                ESP_LOGW(TAG, "MQTT Desconectado do Broker.");
                break;
                
            case MQTT_EVENT_DATA:
                // Prevenção contra ponteiros nulos e validação estrita do contrato
                if (registered_callback != nullptr && event->data != nullptr && event->data_len > 0) {
                    
                    // Conversão de char* para uint8_t* para manter a pureza binária na interface
                    const auto* payload = reinterpret_cast<const uint8_t*>(event->data);
                    const size_t len = static_cast<size_t>(event->data_len);
                    
                    ESP_LOGI(TAG, "Comando MQTT recebido. Tamanho: %zu bytes", len);
                    registered_callback(payload, len);
                }
                break;
                
            case MQTT_EVENT_ERROR:
                ESP_LOGE(TAG, "Erro critico na camada MQTT/TLS.");
                break;
                
            default:
                break;
        }
    }
}

// ============================================================================
// IMPLEMENTAÇÃO DOS CONTRATOS
// ============================================================================

static bool mqtt_connect_impl(void) {
    if (is_initialized && mqtt_client != nullptr) {
        return true; // Evita inicialização dupla
    }

    // Inicialização da estrutura nativa zerada para evitar lixo de memória
    esp_mqtt_client_config_t mqtt_cfg = {};

    // Compatibilidade reversa: O Arduino Core v3 (ESP-IDF v5) exige a nova estrutura aninhada,
    // enquanto versões anteriores usavam variáveis rasas. Esta macro garante a compilação em ambos.
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    mqtt_cfg.broker.address.uri = SEC_BROKER_URL;
    mqtt_cfg.credentials.username = SEC_BROKER_USR;
    mqtt_cfg.credentials.authentication.password = SEC_BROKER_PASS;
    mqtt_cfg.broker.verification.certificate = SEC_BROKER_CERT; // Descomente para TLS
#else
    mqtt_cfg.uri = SEC_BROKER_URL;
    mqtt_cfg.username = SEC_BROKER_USR;
    mqtt_cfg.password = SEC_BROKER_PASS;
    mqtt_cfg.cert_pem = SEC_BROKER_CERT; // Descomente para TLS
#endif

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == nullptr) {
        ESP_LOGE(TAG, "Falha na alocacao de memoria para o cliente MQTT.");
        return false;
    }

    // Regista o manipulador de eventos e inicia o motor MQTT
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, nullptr);
    esp_err_t err = esp_mqtt_client_start(mqtt_client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar cliente MQTT. Erro: %s", esp_err_to_name(err));
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = nullptr;
        return false;
    }

    is_initialized = true;
    return true;
}

static void mqtt_disconnect_impl(void) {
    if (mqtt_client != nullptr) {
        // Destruição formal: O LwIP encerra o socket TCP e liberta a memória da Task interna.
        esp_mqtt_client_stop(mqtt_client);
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = nullptr;
    }
    is_initialized = false;
    is_connected = false;
    ESP_LOGI(TAG, "Cliente MQTT destruido (Memory Leak evitado).");
}

static bool mqtt_subscribe_impl(const char* topic) {
    if (!is_connected || mqtt_client == nullptr || topic == nullptr) {
        return false;
    }
    int msg_id = esp_mqtt_client_subscribe(mqtt_client, topic, 1);
    return (msg_id != -1);
}

static void mqtt_set_receive_callback_impl(control_command_cb_t callback) {
    // Atualização atómica do ponteiro de função
    registered_callback = callback;
}

extern "C" const control_transport_t transport_mqtt = {
    mqtt_connect_impl,
    mqtt_disconnect_impl,
    mqtt_subscribe_impl,
    mqtt_set_receive_callback_impl
};