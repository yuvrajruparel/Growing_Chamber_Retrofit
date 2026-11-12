#include "include/humidity.hpp"
#include "config.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

// ADC Attenuation - Provides a full-scale voltage of ~2.5V, which is safe for
// our max inputs
#define ADC_ATTENUATION ADC_ATTEN_DB_12
#define ADC_MIN 0
#define ADC_MAX 4095

static const char *TAG = "HUMIDITY_SENSOR";
static adc_oneshot_unit_handle_t adc1_handle;

/**
 * Maps a value from one range to another.
 */
float map(float x, float in_min, float in_max, float out_min, float out_max) {
  if (in_max == in_min) {
    return out_min;
  }

  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/**
 * Initializes the ADC units and channels for voltage and current measurement.
 */
void humidity::adcs_init() {
  adc_oneshot_unit_init_cfg_t init_config1 = {
      .unit_id = ADC_UNIT_1,
      .ulp_mode = ADC_ULP_MODE_DISABLE,
  };

  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

  adc_oneshot_chan_cfg_t config = {
      .atten = ADC_ATTENUATION,
      .bitwidth = ADC_BITWIDTH_DEFAULT, // 12 bits on ESP32
  };

  ESP_ERROR_CHECK(
      adc_oneshot_config_channel(adc1_handle, VOLTAGE_ADC_CHANNEL, &config));
  ESP_ERROR_CHECK(
      adc_oneshot_config_channel(adc1_handle, CURRENT_ADC_CHANNEL, &config));

  ESP_LOGI(
      TAG,
      "ADC1 initialized successfully for voltage and current measurement.");
}

esp_err_t humidity::read_difference_mv(float *voltage_mv, int *adc_raw) {
  int adc_reading = 0;

  ESP_RETURN_ON_ERROR(
      adc_oneshot_read(adc1_handle, VOLTAGE_ADC_CHANNEL, &adc_reading), TAG,
      "Failed to read ADC");

  if (adc_raw != nullptr)
    *adc_raw = adc_reading;

  *voltage_mv = map((float)adc_reading, SETPOINT_RAW_LOW, SETPOINT_RAW_HIGH,
                    SETPOINT_MV_LOW, SETPOINT_MV_HIGH);

  return ESP_OK;
}

esp_err_t humidity::read_real_ua(float *current_ua, int *adc_raw) {
  int adc_reading = 0;

  ESP_RETURN_ON_ERROR(
      adc_oneshot_read(adc1_handle, CURRENT_ADC_CHANNEL, &adc_reading), TAG,
      "Failed to read ADC");

  if (adc_raw != nullptr)
    *adc_raw = adc_reading;

  *current_ua =
      map((float)adc_reading, V_LOW_MV, V_HIGH_MV, I_LOW_UA, I_HIGH_UA);

  return ESP_OK;
}
