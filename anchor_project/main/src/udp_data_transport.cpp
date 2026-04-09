#include "udp_data_transport.h"
#include "Defines.h" // Importa TARGET_IP e TARGET_PORT
#include <lwip/sockets.h>
#include <esp_log.h>


namespace {
    constexpr int32_t INVALID_SOCKET = -1;
    const char* const TAG = "UDP_TRANS";

    int32_t current_socket = INVALID_SOCKET;
    struct sockaddr_in dest_addr = {};
}

// ============================================================================
// IMPLEMENTAÇÃO DOS CONTRATOS
// ============================================================================

static bool udp_connect_impl(void) {
    if (current_socket != INVALID_SOCKET) {
        return true; // Evita reinicialização se já estiver aberto
    }

    // Alocação de socket LwIP nativo para datagramas (UDP)
    current_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (current_socket < 0) {
        ESP_LOGE(TAG, "Falha na criacao do socket UDP. errno: %d", errno);
        current_socket = INVALID_SOCKET;
        return false;
    }

    // TARGET_IP e TARGET_PORT são resolvidos em tempo de compilação a partir do Defines.h
    dest_addr.sin_addr.s_addr = inet_addr(TARGET_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(TARGET_PORT);

    ESP_LOGI(TAG, "Socket UDP inicializado. Target: %s:%u", TARGET_IP, TARGET_PORT);
    return true;
}

static void udp_disconnect_impl(void) {
    if (current_socket != INVALID_SOCKET) {
        // Interrompe operações de leitura/escrita ativas antes de fechar
        shutdown(current_socket, 0); 
        close(current_socket);       
        current_socket = INVALID_SOCKET;
        ESP_LOGI(TAG, "Socket UDP encerrado (Memory Leak prevenido).");
    }
}

static bool udp_send_impl(const uint8_t* payload, size_t len) {
    // Validação estrita de ponteiros e estado
    if ((current_socket == INVALID_SOCKET) || (payload == nullptr) || (len == 0U)) {
        return false;
    }

    // Conversões estritas de tipo de ponteiro e tamanho
    const struct sockaddr* p_dest_addr = reinterpret_cast<const struct sockaddr*>(&dest_addr);
    const socklen_t addr_len = static_cast<socklen_t>(sizeof(dest_addr));

    // Despacho imediato para a camada de enlace
    ssize_t err = sendto(current_socket, payload, len, 0, p_dest_addr, addr_len);

    if (err < 0) {
        ESP_LOGE(TAG, "Falha na transmissao UDP. errno: %d", errno);
        return false;
    }

    return true;
}

/*
 * Mapeamento estrito da interface C com exportação forçada.
 * O modificador extern "C" é vital aqui para que o Linker encontre a variável no main.cpp.
 */
extern "C" const data_transport_t transport_udp = {
    udp_connect_impl,
    udp_disconnect_impl,
    udp_send_impl
};