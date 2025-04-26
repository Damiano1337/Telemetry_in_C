#include "access_point.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "sensors.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#define WIFI_SSID "Hydrive_AP"
#define WIFI_PASS "12345678"
#define WIFI_CHANNEL 1
#define MAX_CONN    4

static const char *TAG = "access_point";
extern mesurements_t ap_measurement;  // zadeklaruj globalną zmienną

// Inicjalizacja WiFi w trybie AP
void wifi_init_apsta(void)
{
    ESP_LOGI(TAG, "Initializing WiFi in AP + STA mode...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t ap_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = WIFI_CHANNEL,
            .password = WIFI_PASS,
            .max_connection = MAX_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        }
    };

    if (strlen(WIFI_PASS) == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    wifi_config_t sta_config = {
        .sta = {
            .ssid = "ESP32test",
            .password = "maslo123",
        }
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "Access Point started: SSID:%s password:%s", WIFI_SSID, WIFI_PASS);
    ESP_LOGI(TAG, "Connecting to STA: SSID:%s", sta_config.sta.ssid);
}


// Obsługa strony głównej
static esp_err_t index_handler(httpd_req_t *req)
{
    FILE* f = fopen("/spiffs/index.html", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open index.html");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    char* html = malloc(size + 1);
    if (html == NULL) {
        fclose(f);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    fread(html, 1, size, f);
    html[size] = '\0';
    fclose(f);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, size);
    free(html);

    return ESP_OK;
}

// Endpoint z danymi telemetrycznymi
static esp_err_t api_data_handler(httpd_req_t *req)
{
    char* json = measurements_to_json(&ap_measurement);  // użyj globalnych danych

    if (json) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json, strlen(json));
        free(json);
    } else {
        httpd_resp_send_500(req);
    }

    return ESP_OK;
}

// Start serwera HTTP
void start_http_server(void)
{
    ESP_LOGI(TAG, "Starting HTTP Server");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t index_uri = {
            .uri      = "/",
            .method   = HTTP_GET,
            .handler  = index_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &index_uri);

        httpd_uri_t api_uri = {
            .uri      = "/api/data",
            .method   = HTTP_GET,
            .handler  = api_data_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &api_uri);
    }
}

// Montowanie SPIFFS
void mount_spiffs(void)
{
    ESP_LOGI(TAG, "Mounting SPIFFS...");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

    size_t total = 0, used = 0;
    ESP_ERROR_CHECK(esp_spiffs_info(NULL, &total, &used));
    ESP_LOGI(TAG, "SPIFFS Total: %d, Used: %d", total, used);
}

