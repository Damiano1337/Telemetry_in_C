#include <stdint.h>

#define V_REF 5.0f //Napiecie referencyjne
#define CURRENT_SCALING 1.06f // Korekta wartosci natezenia
#define ZERO_CURRENT_VOLTAGE 2.449f // Napięcie odpowiadające zerowemu prądowi
#define SENSITIVITY_ACS758 0.040f // Wartość czułości czujnika ACS758 w V/A

float calculate_current(int16_t adc_raw)
{
    float voltage = (adc_raw * V_REF) / 4095.0f; // 12-bit ADC
    voltage -= ZERO_CURRENT_VOLTAGE; // Odejmuje napięcie odpowiadające zerowemu prądowi
    voltage *= CURRENT_SCALING; 
    return voltage / SENSITIVITY_ACS758; // Prąd w amperach
}