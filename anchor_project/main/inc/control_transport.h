#ifndef CONTROL_TRANSPORT_H
#define CONTROL_TRANSPORT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef void (*control_command_cb_t)(const uint8_t* payload, size_t len);

typedef struct {
    bool (*connect)(void);
    void (*disconnect)(void); // Nova linha
    bool (*subscribe_to_commands)(const char* topic_or_endpoint);
    void (*set_receive_callback)(control_command_cb_t callback);
} control_transport_t;

#endif // CONTROL_TRANSPORT_H