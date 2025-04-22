#include "driver/gpio.h"
#include "esp_timer.h"
#include <stdint.h>
#include "esp_attr.h"

#define HALL_SENSOR_PIN GPIO_NUM_27 // Pin, do którego podłączony jest czujnik Halla
#define DEBOUNCE_TIME_US 10000 // Odrzucanie zaklocen - min. 10ms miedzy impulsami

static volatile uint32_t pulse_count = 0; // Licznik impulsów
static volatile int32_t last_pulse_time = 0; // Czas ostatniego impulsu

static void IRAM_ATTR hall_isr_handler(void* arg)
{
    int64_t current_time = esp_timer_get_time();
    if (current_time - last_pulse_time > DEBOUNCE_TIME_US) {
        pulse_count++;
        last_pulse_time = current_time;
    }
}

void hall_sensor_init(void) 
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE, // Przerwanie na zboczu
        .mode = GPIO_MODE_INPUT, // Ustawienie pinu jako wejście
        .pin_bit_mask = (1ULL << HALL_SENSOR_PIN), // Ustawienie pinu czujnika Halla
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Wyłączenie rezystora pull-down
        .pull_up_en = GPIO_PULLUP_ENABLE // Włączenie rezystora pull-up
    };
    gpio_config(&io_conf); // Konfiguracja pinu

    gpio_install_isr_service(0); // Instalacja usługi przerwań
    gpio_isr_handler_add(HALL_SENSOR_PIN, hall_isr_handler, NULL); // Dodanie obsługi przerwania
}

float hall_sensor_get_velocity(void)
{
    static uint32_t last_count = 0; // Ostatni licznik impulsów
    static int64_t last_time = 0; // Ostatni czas

    uint32_t current_count = pulse_count; // Aktualny licznik impulsów
    int64_t current_time = esp_timer_get_time(); // Aktualny czas

    float delta_count = (float)(current_count - last_count); // Różnica liczników impulsów
    float delta_time = (float)(current_time - last_time) / 1000000.0f; // Różnica czasu w sekundach

    last_count = current_count; // Aktualizacja ostatniego licznika
    last_time = current_time; // Aktualizacja ostatniego czasu

    if (delta_time == 0) return 0.0f; // Uniknięcie dzielenia przez zero
    return (delta_count / delta_time) * 60.0f; // Prędkość w RPM
}