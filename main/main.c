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

static mesurements_t mqtt_measurement;
mesurements_t ap_measurement;

void pomiar_task(void *pv)
{
    while (1) {
        mesurements_t m = sensors_read_all();

        mqtt_measurement = m;
        //ap_measurement = m;

        char *json = measurements_to_json(&m);
        if (json) {
            sd_logger_append(json);
            printf("%s\n", json);
            free(json);
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void zapis_task(void *pv)
{
    while (1) {
        sd_logger_flush();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void mqtt_publish_task(void *pv)
{
    while (mqtt_get_client() == NULL) {
        ESP_LOGW(TAG, "MQTT client not ready, waiting...");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    while (1) {
        mesurements_t m = mqtt_measurement;
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
    nvs_flash_init();
    sensors_init();

    if (sd_logger_init() != ESP_OK) {
        ESP_LOGE(TAG, "Nie udało się zainicjować karty SD");
    }

    //wifi_init_apsta();
    //mount_spiffs();
    //start_http_server();

    //mqtt_app_start();

    xTaskCreate(pomiar_task, "PomiarTask", 4096, NULL, 5, NULL);
    xTaskCreate(zapis_task, "ZapisTask", 4096, NULL, 4, NULL);
    //xTaskCreate(mqtt_publish_task, "MQTTPublishTask", 4096, NULL, 3, NULL);

}
