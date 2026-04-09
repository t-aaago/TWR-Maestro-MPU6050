#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <stdbool.h>
#include "serializer.h"
#include "data_transport.h"
#include "control_transport.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NET_STATE_DISCONNECTED,
    NET_STATE_WIFI_CONNECTING,
    NET_STATE_WIFI_NO_IP,
    NET_STATE_WIFI_READY,
    NET_STATE_FULLY_READY,
    NET_STATE_FALLBACK
} net_state_t;

typedef void (*network_event_cb)(net_state_t new_state);
    void network_manager_init(const serializer_t* data_serializer, 
                          const data_transport_t* data_transport,
                          const control_transport_t* control_transport,
                          network_event_cb event_callback);

// Assinatura corrigida para receber a fila
void network_manager_start_task(QueueHandle_t uwb_queue);

bool network_manager_change_wifi(const char* new_ssid, const char* new_pass);

net_state_t network_manager_get_state(void);

bool network_manager_is_ready_to_send(void);

#ifdef __cplusplus
}
#endif

#endif // NETWORK_MANAGER_H