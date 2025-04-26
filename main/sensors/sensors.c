#include "sensors.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "cJSON.h"

#include <string.h>

extern float calculate_voltage(uint16_t adc_raw);
extern float calculate_current(uint16_t adc_raw);
extern float hall_sensor_init(void);
extern float hall_sensor_get_velocity(void);

#define PIN_CS1 10
#define PIN_CS2 0
#define PIN_CS3 21
#define PIN_CS4 22

static const char *TAG = "SENSORS";

static spi_device_handle_t spi_dev1;
static spi_device_handle_t spi_dev2;
static spi_device_handle_t spi_dev3;
static spi_device_handle_t spi_dev4;

void sensors_init(void)
{
    spi_bus_config_t buscfg = {
        .miso_io_num = 19,
        .mosi_io_num = 23,
        .sclk_io_num = 18,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = -1, // obsługujemy CS ręcznie
        .queue_size = 1
    };

    ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &devcfg, &spi_dev1));
    ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &devcfg, &spi_dev2));
    ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &devcfg, &spi_dev3));
    ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &devcfg, &spi_dev4));

    gpio_set_direction(PIN_CS1, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_CS2, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_CS3, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_CS4, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_CS1, 1);
    gpio_set_level(PIN_CS2, 1);
    gpio_set_level(PIN_CS3, 1);
    gpio_set_level(PIN_CS4, 1);
    hall_sensor_init();
}

static uint16_t read_adc(spi_device_handle_t dev, uint8_t cs_pin, uint8_t channel)
{
    if (channel > 1) return 0;

    uint8_t tx_data[3] = {
        0x01,
        (uint8_t)(0x80 | (channel << 6)),
        0x00
    };
    uint8_t rx_data[3] = {0};

    spi_transaction_t t = {
        .length = 8 * 3,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data
    };

    gpio_set_level(cs_pin, 0);
    ESP_ERROR_CHECK(spi_device_transmit(dev, &t));
    gpio_set_level(cs_pin, 1);

    return ((rx_data[1] & 0x0F) << 8 | rx_data[2]);
}



mesurements_t sensors_read_all(void)
{
    mesurements_t m;

    uint16_t adc_v1 = read_adc(spi_dev1, PIN_CS1, 0);
    uint16_t adc_v2 = read_adc(spi_dev2, PIN_CS2, 0);
    uint16_t adc_v3 = read_adc(spi_dev3, PIN_CS3, 0);
    uint16_t adc_v4 = read_adc(spi_dev4, PIN_CS4, 0);   
    uint16_t adc_c1 = read_adc(spi_dev1, PIN_CS1, 1);
    uint16_t adc_c2 = read_adc(spi_dev2, PIN_CS2, 1);
    uint16_t adc_c3 = read_adc(spi_dev3, PIN_CS3, 1);
    uint16_t adc_c4 = read_adc(spi_dev4, PIN_CS4, 1); 

    m.voltage1 = calculate_voltage(adc_v1);
    m.voltage2 = calculate_voltage(adc_v2);
    m.voltage3 = calculate_voltage(adc_v3);
    m.voltage4 = calculate_voltage(adc_v4);   
    m.current1 = calculate_current(adc_c1);
    m.current2 = calculate_current(adc_c2);
    m.current3 = calculate_current(adc_c3);
    m.current4 = calculate_current(adc_c4);
    m.velocity = hall_sensor_get_velocity();

    return m;
}

char* measurements_to_json(const mesurements_t *m)
{
    cJSON *root = cJSON_CreateObject();
    char buf[16];

    snprintf(buf, sizeof(buf), "%.2f", m->voltage1);
    cJSON_AddStringToObject(root, "v1", buf);

    snprintf(buf, sizeof(buf), "%.2f", m->voltage2);
    cJSON_AddStringToObject(root, "v2", buf);

    snprintf(buf, sizeof(buf), "%.2f", m->voltage3);
    cJSON_AddStringToObject(root, "v3", buf);

    snprintf(buf, sizeof(buf), "%.2f", m->voltage4);
    cJSON_AddStringToObject(root, "v4", buf);

    snprintf(buf, sizeof(buf), "%.2f", m->current1);
    cJSON_AddStringToObject(root, "c1", buf);

    snprintf(buf, sizeof(buf), "%.2f", m->current2);
    cJSON_AddStringToObject(root, "c2", buf);

    snprintf(buf, sizeof(buf), "%.2f", m->current3);
    cJSON_AddStringToObject(root, "c3", buf);

    snprintf(buf, sizeof(buf), "%.2f", m->current4);
    cJSON_AddStringToObject(root, "c4", buf);

    snprintf(buf, sizeof(buf), "%.2f", m->velocity);
    cJSON_AddStringToObject(root, "s", buf);

    cJSON_AddNumberToObject(root, "timestamp", esp_timer_get_time() / 1000);

    char *json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return json_string;
}