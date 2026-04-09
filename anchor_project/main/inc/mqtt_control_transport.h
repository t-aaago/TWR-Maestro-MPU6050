#ifndef MQTT_CONTROL_TRANSPORT_H
#define MQTT_CONTROL_TRANSPORT_H

#include "control_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Instância concreta do contrato control_transport_t para o protocolo MQTT.
 */
extern const control_transport_t transport_mqtt;

#ifdef __cplusplus
}
#endif

#endif // MQTT_CONTROL_TRANSPORT_H