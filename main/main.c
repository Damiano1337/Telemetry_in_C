#include "wifi/access_point.h"
#include "esp_spiffs.h"
#include "sensors.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sd_logger.h"
#include "mqtt/mqtt_com.h"
#include "nvs_flash.h"
#include "mqtt_client.h"

static const char *TAG = "MAIN";

static mesurements_t last_measurement;

//static esp_mqtt_client_handle_t mqtt_client = NULL;

// Task 1: Pomiar co 1ms
void pomiar_task(void *pv)
{
    while (1) {
        mesurements_t m = sensors_read_all();
        last_measurement = m;

        char *json = measurements_to_json(&m);
        if (json) {
            sd_logger_append(json);
            free(json);
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// Task 2: Zapis co 50ms
void zapis_task(void *pv)
{
    while (1) {
        sd_logger_flush();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Task 3: Publikacja MQTT co 100ms
void mqtt_publish_task(void *pv)
{
    while (mqtt_get_client() == NULL) {
        ESP_LOGW(TAG, "MQTT client not ready, waiting...");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    while (1) {
        mesurements_t m;
            m = last_measurement;

        char *json = measurements_to_json(&m);
        if (json) {
            esp_mqtt_client_publish(mqtt_get_client(), mqtt_get_topic(), json, 0, 1, 0);
            free(json);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{
    
    //wifi_init_softap();
    //mount_spiffs();
    //start_http_server();

    nvs_flash_init();
    sensors_init();

    if (sd_logger_init() != ESP_OK) {
        ESP_LOGE(TAG, "Nie udało się zainicjować karty SD");
    }

    mqtt_app_start();

    xTaskCreate(pomiar_task, "PomiarTask", 4096, NULL, 5, NULL);
    xTaskCreate(zapis_task, "ZapisTask", 4096, NULL, 4, NULL);
    xTaskCreate(mqtt_publish_task, "MQTTPublishTask", 4096, NULL, 3, NULL);
}
