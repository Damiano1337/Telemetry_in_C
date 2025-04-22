#include "sd_logger.h"
#include "esp_log.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include <stdio.h>
#include <string.h>

#define MOUNT_POINT "/sdcard"
#define FILE_PATH "/sdcard/pomiary1.txt"

static const char *TAG = "SD_LOGGER";

static sdmmc_card_t *card;
static FILE *log_file = NULL;

#define BUFFER_SIZE 8192
static char buffer[BUFFER_SIZE];
static size_t buffer_index = 0;
static SemaphoreHandle_t buffer_mutex;

esp_err_t sd_logger_init(void)
{
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 3,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4;

    gpio_set_pull_mode(15, GPIO_PULLUP_ONLY); // CMD
    gpio_set_pull_mode(2,  GPIO_PULLUP_ONLY); // D0
    gpio_set_pull_mode(4,  GPIO_PULLUP_ONLY); // D1
    gpio_set_pull_mode(12, GPIO_PULLUP_ONLY); // D2
    gpio_set_pull_mode(13, GPIO_PULLUP_ONLY); // D3
    gpio_set_pull_mode(14, GPIO_PULLUP_ONLY); // CLK

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Karta SD: ID = %s, Tryb magistrali: %d-bit", card->cid.name, slot_config.width);


    log_file = fopen(FILE_PATH, "a");
    if (!log_file) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }

    buffer_mutex = xSemaphoreCreateMutex();
    if (buffer_mutex == NULL) {
        ESP_LOGE(TAG, "Nie udało się utworzyć mutexa!");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "SD card mounted and file ready");
    return ESP_OK;
}

void sd_logger_append(const char *json_line)
{
    if (!json_line) return;
    size_t len = strlen(json_line);
    if (len + 2 > BUFFER_SIZE) return; // pojedyncza linia za duża

    xSemaphoreTake(buffer_mutex, portMAX_DELAY);

    if (buffer_index + len + 2 < BUFFER_SIZE) {
        memcpy(&buffer[buffer_index], json_line, len);
        buffer_index += len;
        buffer[buffer_index++] = '\n';
        buffer[buffer_index] = '\0';
    }

    xSemaphoreGive(buffer_mutex);
}

void sd_logger_flush(void)
{
    xSemaphoreTake(buffer_mutex, portMAX_DELAY);

    if (buffer_index > 0) {
        FILE *f = fopen(FILE_PATH, "a");
        if (f) {
            fwrite(buffer, 1, buffer_index, f);
            fclose(f); 
            //ESP_LOGI("SD_LOGGER", "Zapisano %d bajtów do pliku", buffer_index);
        } else {
            ESP_LOGE("SD_LOGGER", "Nie udało się otworzyć pliku!");
        }
        buffer_index = 0;
    }

    xSemaphoreGive(buffer_mutex);
}
