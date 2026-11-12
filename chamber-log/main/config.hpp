#pragma once
#include "hal/adc_types.h"

#define WIFI_NAME "DukeOpen"
#define WIFI_PASSWORD ""

#define HTTP_ENDPOINT                                                          \
  "https://n8n.derock.dev/webhook/8d5e7771-8bd3-4d71-8ac8-5746f56ada5a"

// Must use ADC1, ADC2 is unstable when wifi is in use.
#define VOLTAGE_ADC_CHANNEL ADC_CHANNEL_7 // GPIO35 for Setpoint (0-10V)
#define CURRENT_ADC_CHANNEL ADC_CHANNEL_6 // GPIO34 for Real (4-20mA)

#define R1_OHMS 22000.0f           // Voltage divider top resistor
#define R2_OHMS 10000.0f           // Voltage divider bottom resistor
#define SHUNT_RESISTOR_OHMS 100.0f // Shunt resistor for current measurement

// Calibration constants for voltage and current measurements
// measure at 4mA and 20mA, then record the mV values (non converted, raw adc)

#define V_LOW_MV 547    // The measured mV (raw adc) at 4mA
#define V_HIGH_MV 2462  // The measured mV (raw adc) at 20mA
#define I_LOW_UA 4000   // 4mA
#define I_HIGH_UA 20000 // 20mA

#define SETPOINT_RAW_HIGH 4090 // Raw ADC reading at 10,000mV
#define SETPOINT_RAW_LOW 597   // Raw ADC reading at 2,000mV
#define SETPOINT_MV_HIGH 10000 // 10,000mV
#define SETPOINT_MV_LOW 2000   // 2,000mV
