#pragma once

#include "esp_err.h"
namespace humidity {

/**
 * @brief Initializes the ADC channels for humidity measurements.
 * This function configures ADC1 channels 6 and 7 for one-shot reads.
 * It must be called once before any read functions.
 */
void adcs_init();

/**
 * @brief Reads the process output from TC2 (configured to be only P in PID, so
 * linear difference). This function reads the analog voltage from the TC2's
 * 0-10V output, which has been scaled by a voltage divider.
 * @return The calculated setpoint signal in millivolts (mV).
 */
esp_err_t read_difference_mv(float *voltage_mv, int *adc_raw);

/**
 * @brief Reads the real humidity value from the current loop output.
 * This function reads the voltage across a shunt resistor to determine
 * the current from the TC2's 4-20mA output.
 * @return The calculated real humidity signal in microamps (uA).
 */
esp_err_t read_real_ua(float *current_ua, int *adc_raw);

} // namespace humidity
