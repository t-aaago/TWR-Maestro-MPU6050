#include "unity.h"
#include "network_manager.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "types.h" // Importação necessária para a assinatura correta do mock_serialize

// ============================================================================
// MOCKS E ISOLAMENTO DE ESTADO (Namespace Anónimo)
// ============================================================================
namespace {
    // Rastreadores de execução
    bool data_connect_called = false;
    bool data_disconnect_called = false;
    bool data_send_called = false;

    bool ctrl_connect_called = false;
    bool ctrl_disconnect_called = false;
    bool ctrl_connect_should_fail = false; // Flag para injeção de falha

    net_state_t last_reported_state = NET_STATE_DISCONNECTED;

    // Função de reset para garantir isolamento limpo entre os testes
    void reset_mock_states() {
        data_connect_called = false;
        data_disconnect_called = false;
        data_send_called = false;
        ctrl_connect_called = false;
        ctrl_disconnect_called = false;
        ctrl_connect_should_fail = false;
        last_reported_state = NET_STATE_DISCONNECTED;
    }

    // --- Implementações Falsas (Mocks) ---
    void mock_network_event_cb(net_state_t new_state) {
        last_reported_state = new_state;
    }

    bool mock_data_connect() { 
        data_connect_called = true; 
        return true; 
    }
    
    void mock_data_disconnect() { 
        data_disconnect_called = true; 
    }
    
    bool mock_data_send(const uint8_t* payload, size_t len) { 
        data_send_called = true; 
        return true; 
    }

    bool mock_ctrl_connect() { 
        ctrl_connect_called = true; 
        return !ctrl_connect_should_fail; 
    }
    
    void mock_ctrl_disconnect() { 
        ctrl_disconnect_called = true; 
    }
    
    bool mock_ctrl_subscribe(const char* topic) { 
        return true; 
    }
    
    void mock_ctrl_set_cb(control_command_cb_t cb) { 
        // Não executamos ação neste mock
    }

    // Assinatura estrita exigida pelo contrato C++ (const range_pkg_t*)
    size_t mock_serialize(const range_pkg_t* data, uint8_t* buffer, size_t max_len) {
        return 10U; // Simula 10 bytes escritos de forma arbitrária
    }
}

// ============================================================================
// EXPORTAÇÃO DOS CONTRATOS (C-Linkage exigida pela API)
// ============================================================================
extern "C" {
    const data_transport_t mock_data_transport = {
        mock_data_connect,
        mock_data_disconnect,
        mock_data_send
    };

    const control_transport_t mock_control_transport = {
        mock_ctrl_connect,
        mock_ctrl_disconnect,
        mock_ctrl_subscribe,
        mock_ctrl_set_cb
    };

    const serializer_t mock_serializer = {
        mock_serialize
    };
}

// ============================================================================
// SETUP GLOBAL DOS TESTES DE REDE
// ============================================================================
static void ensure_network_initialized() {
    static bool init_done = false;
    if (!init_done) {
        // Inicializa o loop de eventos base APENAS UMA VEZ
        esp_event_loop_create_default();
        
        // O Init de hardware LwIP e NVS corre apenas na primeira vez
        network_manager_init(&mock_serializer, &mock_data_transport, &mock_control_transport, mock_network_event_cb);
        init_done = true;
    }
}

// ============================================================================
// TESTES UNITÁRIOS DE MÁQUINA DE ESTADOS E TOLERÂNCIA A FALHAS
// ============================================================================

TEST_CASE("NET-01: Estado Inicial da Maquina de Estados", "[network]") {
    reset_mock_states();
    ensure_network_initialized();
    
    // O sistema não pode tentar conectar transportes antes de obter um IP
    // O estado deve ser NET_STATE_DISCONNECTED (valor enumérico 0)
    TEST_ASSERT_EQUAL(0, network_manager_get_state());
    TEST_ASSERT_FALSE(data_connect_called);
    TEST_ASSERT_FALSE(ctrl_connect_called);
}

TEST_CASE("NET-02: Prevencao de Memory Leaks em falha de camada MAC", "[network][fault_injection]") {
    reset_mock_states();
    ensure_network_initialized();

    // 1. Simula um evento de desconexão (como se a antena DW1000/Wi-Fi cortasse o sinal)
    esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, nullptr, 0, portMAX_DELAY);
    
    // Cede processamento à task interna de eventos do ESP-IDF para despachar o evento
    vTaskDelay(pdMS_TO_TICKS(100));

    // 2. Validação Estrita: A máquina de estados TEM de ter invocado o encerramento 
    // de ambos os descritores de ficheiro (Sockets) para prevenir exaustão do Heap.
    TEST_ASSERT_TRUE(data_disconnect_called);
    TEST_ASSERT_TRUE(ctrl_disconnect_called);
}

TEST_CASE("NET-03: Plano de Dados sobrevive a falha do Plano de Controle", "[network][fault_injection]") {
    reset_mock_states();
    ensure_network_initialized();
    
    // Injeção de Falha: O MQTT vai falhar a ligação TCP
    ctrl_connect_should_fail = true; 

    // Simula a obtenção de um IP pela camada DHCP
    ip_event_got_ip_t ip_info = {};
    esp_event_post(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_info, sizeof(ip_event_got_ip_t), portMAX_DELAY);
    
    vTaskDelay(pdMS_TO_TICKS(100));

    // O sistema chamou a conexão de ambos
    TEST_ASSERT_TRUE(data_connect_called);
    TEST_ASSERT_TRUE(ctrl_connect_called);

    // Validação Estrutural: A telemetria UDP DEVE estar operacional, 
    // mesmo com a infraestrutura MQTT inoperante.
    TEST_ASSERT_TRUE(network_manager_is_ready_to_send());
}

// ============================================================================
// GARANTIA DE LIGAÇÃO (LINKAGE)
// ============================================================================
// Força o Linker a não descartar este ficheiro objeto durante a otimização
void force_link_network() {}