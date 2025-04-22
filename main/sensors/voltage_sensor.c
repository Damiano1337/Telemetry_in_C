#include <stdint.h>

#define V_REF 5.0f //Napiecie referencyjne
#define VOLTAGE_SCALING 0.96f // Korekta wartosci napiecia
#define SCALE_VOLTAGE ((10.0f + 1.2f)/ 1.2f) // Dzielnik napiecia

float calculate_voltage(int32_t adc_raw)
{
    float voltage = (adc_raw * V_REF) / 4095.0f; // 12-bit ADC
    voltage *= VOLTAGE_SCALING; 
    return voltage * SCALE_VOLTAGE; 
}