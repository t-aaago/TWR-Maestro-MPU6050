#include "unity.h"
#include "json_serializer.h"
#include "types.h"
#include <cstring>

// ============================================================================
// TESTES DO MÓDULO DE SERIALIZAÇÃO JSON
// ============================================================================

TEST_CASE("JSON Serializer - Formata pacote valido", "[serializer]") {
    range_pkg_t pkg = {};
    pkg.anchor_id = 0x3014U;
    pkg.tag_id = 0x1234U;
    pkg.distance = 5.43f;
    pkg.ax = 100;
    pkg.ay = -50;
    pkg.az = 10;
    pkg.rp_power = -85.5f;
    pkg.fp_power = -86.0f;
    pkg.eta = 0.99f;
    pkg.quality = 100.0f;

    uint8_t buffer[256] = {};
    size_t len = serializer_json.serialize(&pkg, buffer, sizeof(buffer));

    // Validações do Teste Unitário
    TEST_ASSERT_GREATER_THAN(0, len);
    
    // Converte o buffer binário para string e verifica se o campo de distância existe corretamente
    const char* json_str = reinterpret_cast<const char*>(buffer);
    TEST_ASSERT_NOT_NULL(std::strstr(json_str, "\"distancia\":5.43"));
    TEST_ASSERT_NOT_NULL(std::strstr(json_str, "\"id_ancora\":12308")); // 0x3014 em decimal
}

TEST_CASE("JSON Serializer - Tolera ponteiro de pacote nulo", "[serializer][fault_injection]") {
    uint8_t buffer[256] = {};
    
    // Injeção de Falha 1: Pacote inexistente
    size_t len = serializer_json.serialize(nullptr, buffer, sizeof(buffer));

    // A função tem de devolver 0 de forma graciosa, sem causar Core Panic
    TEST_ASSERT_EQUAL_UINT32(0U, len);
}

TEST_CASE("JSON Serializer - Tolera ponteiro de buffer nulo", "[serializer][fault_injection]") {
    range_pkg_t pkg = {};
    
    // Injeção de Falha 2: Endereço de destino de memória inválido
    size_t len = serializer_json.serialize(&pkg, nullptr, 256U);

    TEST_ASSERT_EQUAL_UINT32(0U, len);
}

TEST_CASE("JSON Serializer - Intercepta e aborta em Buffer Overflow", "[serializer][fault_injection]") {
    range_pkg_t pkg = {};
    uint8_t small_buffer[10] = {}; // Buffer intencionalmente incapaz de armazenar o JSON

    // Injeção de Falha 3: Mentir sobre a capacidade de memória
    size_t len = serializer_json.serialize(&pkg, small_buffer, sizeof(small_buffer));

    // O ArduinoJson tem de detetar a quebra de limite, truncar a operação e devolver 0
    TEST_ASSERT_EQUAL_UINT32(0U, len);
}
void force_link_json() {}