#ifndef UDP_DATA_TRANSPORT_H
#define UDP_DATA_TRANSPORT_H

#include "data_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Instância concreta do contrato data_transport_t para o protocolo UDP.
 */
extern const data_transport_t transport_udp;

#ifdef __cplusplus
}
#endif

#endif // UDP_DATA_TRANSPORT_H