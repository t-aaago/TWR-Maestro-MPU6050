#include "json_serializer.h"
#include <ArduinoJson.h>

/*
 * Implementação estática (file scope) da função de serialização.
 * Não é visível fora deste ficheiro, garantindo encapsulamento estrito.
 */
static size_t json_serialize_impl(const range_pkg_t* data, uint8_t* out_buffer, size_t max_len) {
    
    // 1. Proteção contra Null Pointer Dereference e parâmetros inválidos
    if (data == nullptr || out_buffer == nullptr || max_len == 0U) {
        return 0U; // Zero bytes escritos. O orquestrador abortará o envio.
    }

    // 2. Alocação Estática na Stack
    // O objeto JsonDocument gere a memória na stack local da Task (Core 0).
    // O uso de objetos String() é estritamente evitado para prevenir fragmentação no Heap (Memory Leaks).
    JsonDocument doc;

    // 3. Mapeamento direto de tipos primitivos
    doc["id_ancora"] = data->anchor_id;
    doc["id_tag"]    = data->tag_id;
    doc["distancia"] = data->distance;
    doc["ax"]        = data->ax;
    doc["ay"]        = data->ay;
    doc["az"]        = data->az;
    doc["ts_us"]     = data->timestamp_us;
    doc["fp"]        = data->fp_power;
    doc["rx"]        = data->rp_power;
    doc["eta"]       = data->eta;
    doc["quality"]   = data->quality;

    // 4. Serialização com proteção de buffer (Buffer Overflow Protection)
    // O casting para (char*) é necessário pois a API espera arrays de caracteres, 
    // mas a integridade dos dados (8-bits) é mantida.
    size_t bytes_written = serializeJson(doc, (char*)out_buffer, max_len);

    // 5. Validação de truncamento
    // Se o número de bytes escritos for igual ou superior ao tamanho máximo do buffer,
    // significa que o payload foi truncado e o JSON resultante é inválido.
    if (bytes_written == 0U || bytes_written >= max_len) {
        return 0U;
    }

    return bytes_written;
}

/*
 * Mapeamento da função para a interface (struct) em C.
 */
extern "C" const serializer_t serializer_json = {
    json_serialize_impl
};