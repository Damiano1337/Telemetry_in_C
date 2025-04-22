#ifndef MQTT_COM_H
#define MQTT_COM_H

#include "mqtt_client.h"

void mqtt_app_start(void);
esp_mqtt_client_handle_t mqtt_get_client(void);
const char* mqtt_get_topic(void);

#endif // MQTT_COM_H
