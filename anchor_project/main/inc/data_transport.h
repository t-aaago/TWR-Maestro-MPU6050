#ifndef DATA_TRANSPORT_H
#define DATA_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef bool (*transport_connect_fn)(void);
typedef void (*transport_disconnect_fn)(void);
typedef bool (*transport_send_fn)(const uint8_t* payload, size_t len);

typedef struct {
    transport_connect_fn connect;
    transport_disconnect_fn disconnect; // Nova linha
    transport_send_fn send;
} data_transport_t;

#endif // DATA_TRANSPORT_H